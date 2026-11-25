// Copyright Azahar Duo Enhanced contributors
// Licensed under GPLv3 or any later version
// Refer to the LICENSE.txt file included.

// Azahar Duo Enhanced: measurement-only instrumentation (milestone 0.1).
// Nothing in this file changes emulator behaviour. Hot-path calls only touch
// memory; all file I/O happens on a dedicated low-frequency writer thread.

#pragma once

#include <chrono>
#include <string>
#include "common/common_types.h"

namespace Common::DuoStats {

enum class Event : u8 {
    /// Marker: disk cache load for a title started / finished.
    CacheLoadBegin,
    CacheLoadEnd,
    /// Marker: every pipeline queued while loading the disk cache has finished building.
    /// value = number of pipelines that were still pending when the load returned.
    CachedPipelinesReady,
    /// A SPIR-V blob was read from the transferable disk cache (no generation needed).
    SpirvCacheLoad,
    /// A fragment shader was generated directly as SPIR-V (duration = generator time).
    SpirvGenerate,
    /// GLSL was compiled to SPIR-V through glslang (duration = glslang time).
    GlslCompile,
    /// vkCreateShaderModule (duration = driver time).
    ShaderModuleCreate,
    /// A shader configuration not present in the disk cache was encountered.
    NewShaderConfig,
    /// Pipeline creation probe with FAIL_ON_PIPELINE_COMPILE_REQUIRED succeeded:
    /// the driver pipeline cache already held it.
    PipelineDriverCacheHit,
    /// The same probe reported that a real compile is required.
    PipelineDriverCacheMiss,
    /// Full vkCreateGraphicsPipelines call without the probe flag (duration = driver time).
    /// May still be served from the driver cache; the duration tells which.
    PipelineCompile,
    /// vkCreateGraphicsPipelines returned an error.
    PipelineFailed,
    /// A pipeline state not present in the disk cache was encountered.
    NewPipelineConfig,
    /// The render (scheduler) thread blocked until a pipeline finished building.
    PipelineWait,
    /// A draw was skipped because its pipeline was not ready (async shader mode only).
    DrawSkipped,
    /// The driver pipeline cache blob was written to disk (value = bytes).
    DriverCacheSaved,
    /// Marker: the renderer's pipeline cache was destroyed (emulation stop, or the renderer
    /// being rebuilt, for example when a save state is loaded).
    RendererDestroyed,
    /// The emulation thread spent more than 10 ms presenting a frame (SwapBuffers).
    SlowSwapBuffers,
    /// A thread spent more than 10 ms waiting for the Vulkan scheduler worker to drain.
    SlowSchedulerWait,

    Count,
};

/// Flags stored with each event.
enum : u8 {
    FlagDuringLoad = 1 << 0,   ///< Happened while the loading screen was showing
    FlagPreloaded = 1 << 1,    ///< Object was reconstructed from the disk cache
    FlagStageVertex = 1 << 2,  ///< Shader stage (if applicable)
    FlagStageFragment = 1 << 3,
    FlagStageGeometry = 1 << 4,
};

using Clock = std::chrono::steady_clock;

/// Starts a measurement session. Creates <user>/log/duo/<timestamp>_<title>/ and writes
/// device_report.txt with the supplied text. Ends any previous session first.
/// A session belongs to one emulation run, not to one renderer: the frontend ends it when
/// emulation stops, so it survives the renderer being rebuilt (save state load).
void BeginSession(u64 title_id, const std::string& device_report);

/// Flushes everything and stops the writer thread. Safe to call when no session is active.
void EndSession();

/// Returns true while a session is active. Cheap (one relaxed atomic load).
bool IsActive();

/// Marks whether the disk cache is currently being loaded (loading screen visible).
void SetLoading(bool loading);

/// Records an event. Thread-safe. No disk I/O.
void Record(Event event, u64 id = 0, Clock::duration duration = {}, u8 flags = 0, u64 value = 0);

/// Records one emulated system frame. interval = time since the previous frame ended,
/// work = time between BeginSystemFrame and EndSystemFrame. Thread-safe. No disk I/O.
void RecordSystemFrame(Clock::duration interval, Clock::duration work);

/// Records that the game presented a new frame. Thread-safe. No disk I/O.
void RecordGameFrame();

/// RAII timer that records an event only when it took longer than the given threshold.
/// For probing hot paths where recording every call would be too much data.
class ScopedSlowEvent {
public:
    explicit ScopedSlowEvent(Event event_, Clock::duration threshold_)
        : event{event_}, threshold{threshold_}, active{IsActive()} {
        if (active) {
            start = Clock::now();
        }
    }
    ~ScopedSlowEvent() {
        if (active) {
            const auto elapsed = Clock::now() - start;
            if (elapsed >= threshold) {
                Record(event, 0, elapsed);
            }
        }
    }
    ScopedSlowEvent(const ScopedSlowEvent&) = delete;
    ScopedSlowEvent& operator=(const ScopedSlowEvent&) = delete;

private:
    Event event;
    Clock::duration threshold;
    bool active;
    Clock::time_point start{};
};

/// RAII timer that records an event with its elapsed time on destruction.
class ScopedEvent {
public:
    explicit ScopedEvent(Event event_, u64 id_ = 0, u8 flags_ = 0)
        : event{event_}, id{id_}, flags{flags_}, active{IsActive()} {
        if (active) {
            start = Clock::now();
        }
    }
    ~ScopedEvent() {
        if (active) {
            Record(event, id, Clock::now() - start, flags);
        }
    }
    ScopedEvent(const ScopedEvent&) = delete;
    ScopedEvent& operator=(const ScopedEvent&) = delete;

private:
    Event event;
    u64 id;
    u8 flags;
    bool active;
    Clock::time_point start{};
};

} // namespace Common::DuoStats
