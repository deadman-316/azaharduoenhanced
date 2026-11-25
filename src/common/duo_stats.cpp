// Copyright Azahar Duo Enhanced contributors
// Licensed under GPLv3 or any later version
// Refer to the LICENSE.txt file included.

#include <algorithm>
#include <array>
#include <atomic>
#include <condition_variable>
#include <ctime>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include <fmt/chrono.h>
#include <fmt/format.h>
#include "common/common_paths.h"
#include "common/duo_stats.h"
#include "common/file_util.h"
#include "common/logging/log.h"
#include "common/scm_rev.h"
#include "common/thread.h"

namespace Common::DuoStats {

namespace {

using namespace std::chrono_literals;

constexpr std::size_t NumEvents = static_cast<std::size_t>(Event::Count);

constexpr std::array<const char*, NumEvents> EVENT_NAMES = {
    "CacheLoadBegin",
    "CacheLoadEnd",
    "CachedPipelinesReady",
    "SpirvCacheLoad",
    "SpirvGenerate",
    "GlslCompile",
    "ShaderModuleCreate",
    "NewShaderConfig",
    "PipelineDriverCacheHit",
    "PipelineDriverCacheMiss",
    "PipelineCompile",
    "PipelineFailed",
    "NewPipelineConfig",
    "PipelineWait",
    "DrawSkipped",
    "DriverCacheSaved",
};

/// Events that represent compilation work or a thread blocked on it.
constexpr bool IsCompileActivity(Event event) {
    switch (event) {
    case Event::SpirvGenerate:
    case Event::GlslCompile:
    case Event::ShaderModuleCreate:
    case Event::PipelineDriverCacheMiss:
    case Event::PipelineCompile:
    case Event::PipelineWait:
        return true;
    default:
        return false;
    }
}

// The 3DS refreshes at ~59.83 Hz, one frame is ~16.71 ms.
constexpr u64 FRAME_NS = 16'713'000;
constexpr u64 SPIKE_MINOR_NS = FRAME_NS * 3 / 2; // > 1.5 frames
constexpr u64 SPIKE_MAJOR_NS = FRAME_NS * 2;     // > 2 frames
// Gaps longer than this are pauses (menu, app in background, first frame of a session), not
// stutter. They are still written to frames.csv but kept out of the statistics.
constexpr u64 DISCONTINUITY_NS = 2'000'000'000;

// Frame interval histogram: 0.25 ms buckets up to 250 ms, last bucket is overflow.
constexpr u64 HIST_BUCKET_NS = 250'000;
constexpr std::size_t HIST_BUCKETS = 1001;

constexpr auto FLUSH_PERIOD = 5s;
constexpr std::size_t FRAME_RESERVE = 2048;
constexpr std::size_t EVENT_RESERVE = 1024;

struct EventRow {
    u64 t_ns;
    u64 id;
    u64 duration_ns;
    u64 value;
    Event event;
    u8 flags;
};

struct FrameRow {
    u64 t_ns;
    u32 interval_us;
    u32 work_us;
    u8 kind; // 0 = system frame, 1 = game frame, 2 = pause/discontinuity (excluded from stats)
};

struct Counter {
    std::atomic<u64> count{};
    std::atomic<u64> total_ns{};
    std::atomic<u64> max_ns{};

    void Add(u64 ns) {
        count.fetch_add(1, std::memory_order_relaxed);
        total_ns.fetch_add(ns, std::memory_order_relaxed);
        u64 prev = max_ns.load(std::memory_order_relaxed);
        while (ns > prev && !max_ns.compare_exchange_weak(prev, ns, std::memory_order_relaxed)) {
        }
    }

    void Reset() {
        count = 0;
        total_ns = 0;
        max_ns = 0;
    }
};

struct State {
    std::atomic_bool active{false};
    std::atomic_bool loading{false};

    Clock::time_point session_start{};
    u64 title_id{};
    std::string dir;

    std::mutex event_mutex;
    std::vector<EventRow> event_pending;

    std::mutex frame_mutex;
    std::vector<FrameRow> frame_pending;

    // [event][0 = gameplay, 1 = during load]
    std::array<std::array<Counter, 2>, NumEvents> counters;

    std::array<std::atomic<u64>, HIST_BUCKETS> histogram{};
    std::atomic<u64> system_frames{};
    std::atomic<u64> system_frame_total_ns{};
    std::atomic<u64> system_frame_max_ns{};
    std::atomic<u64> spikes_minor{};
    std::atomic<u64> spikes_major{};
    std::atomic<u64> spikes_major_with_compile{};
    std::atomic<u64> game_frames{};
    std::atomic<u64> discontinuities{};
    std::atomic<u64> last_compile_activity_ns{};
    std::atomic<u64> last_game_frame_ns{};

    // Writer thread
    std::thread writer;
    std::mutex writer_mutex;
    std::condition_variable writer_cv;
    bool writer_stop{false};
    FileUtil::IOFile events_file;
    FileUtil::IOFile frames_file;
};

State& GetState() {
    // Intentionally leaked so that late callers during shutdown never touch a destroyed object.
    static State* state = new State();
    return *state;
}

u64 NowNs(const State& s) {
    return static_cast<u64>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - s.session_start)
            .count());
}

u64 ToNs(Clock::duration d) {
    const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(d).count();
    return ns > 0 ? static_cast<u64>(ns) : 0;
}

double Ms(u64 ns) {
    return static_cast<double>(ns) / 1'000'000.0;
}

/// Returns the frame interval (ms) below which `fraction` of all frames fall.
double Percentile(const std::array<u64, HIST_BUCKETS>& hist, u64 total, double fraction) {
    if (total == 0) {
        return 0.0;
    }
    const u64 target = static_cast<u64>(static_cast<double>(total) * fraction);
    u64 seen = 0;
    for (std::size_t i = 0; i < HIST_BUCKETS; i++) {
        seen += hist[i];
        if (seen > target) {
            return Ms((i + 1) * HIST_BUCKET_NS);
        }
    }
    return Ms(HIST_BUCKETS * HIST_BUCKET_NS);
}

std::string BuildSummary(State& s) {
    std::string out;
    const double elapsed_s = static_cast<double>(NowNs(s)) / 1e9;

    fmt::format_to(std::back_inserter(out),
                   "Azahar Duo Enhanced - session summary\n"
                   "build: {}\n"
                   "shader_cache_version: {}\n"
                   "title_id: {:016X}\n"
                   "session_seconds: {:.1f}\n\n",
                   Common::g_build_fullname, Common::g_shader_cache_version, s.title_id,
                   elapsed_s);

    std::array<u64, HIST_BUCKETS> hist;
    for (std::size_t i = 0; i < HIST_BUCKETS; i++) {
        hist[i] = s.histogram[i].load(std::memory_order_relaxed);
    }
    const u64 frames = s.system_frames.load();
    const u64 total_ns = s.system_frame_total_ns.load();
    const double mean_ms = frames ? Ms(total_ns) / static_cast<double>(frames) : 0.0;

    fmt::format_to(std::back_inserter(out),
                   "[system frames] (interval between emulated frames, ideal {:.2f} ms)\n"
                   "count: {}\n"
                   "mean_ms: {:.3f}\n"
                   "mean_fps: {:.2f}\n"
                   "median_ms: {:.2f}\n"
                   "p95_ms: {:.2f}\n"
                   "p99_ms: {:.2f}\n"
                   "p99.9_ms: {:.2f}\n"
                   "max_ms: {:.2f}\n"
                   "frames_over_1.5x ({:.1f} ms): {}\n"
                   "frames_over_2x ({:.1f} ms): {}\n"
                   "frames_over_2x_overlapping_compile_activity: {}\n"
                   "game_frames_presented: {}\n"
                   "pauses_excluded (gaps over {:.0f} s): {}\n\n",
                   Ms(FRAME_NS), frames, mean_ms, mean_ms > 0 ? 1000.0 / mean_ms : 0.0,
                   Percentile(hist, frames, 0.50), Percentile(hist, frames, 0.95),
                   Percentile(hist, frames, 0.99), Percentile(hist, frames, 0.999),
                   Ms(s.system_frame_max_ns.load()), Ms(SPIKE_MINOR_NS), s.spikes_minor.load(),
                   Ms(SPIKE_MAJOR_NS), s.spikes_major.load(),
                   s.spikes_major_with_compile.load(), s.game_frames.load(),
                   static_cast<double>(DISCONTINUITY_NS) / 1e9, s.discontinuities.load());

    out += "[events] phase,event,count,total_ms,mean_ms,max_ms\n";
    for (std::size_t phase = 0; phase < 2; phase++) {
        for (std::size_t i = 0; i < NumEvents; i++) {
            const Counter& c = s.counters[i][phase];
            const u64 count = c.count.load();
            if (count == 0) {
                continue;
            }
            const u64 total = c.total_ns.load();
            fmt::format_to(std::back_inserter(out), "{},{},{},{:.3f},{:.3f},{:.3f}\n",
                           phase == 1 ? "load" : "gameplay", EVENT_NAMES[i], count, Ms(total),
                           Ms(total) / static_cast<double>(count), Ms(c.max_ns.load()));
        }
    }

    out += "\nNotes:\n"
           "- 'overlapping_compile_activity' means a compile or pipeline wait finished inside\n"
           "  that frame. It shows correlation, not proof of cause.\n"
           "- Percentiles are read from a histogram with 0.25 ms resolution.\n"
           "- 'load' rows happened while the loading screen was showing.\n"
           "- This file is rewritten every few seconds; if the app was killed it is at most\n"
           "  a few seconds stale.\n";
    return out;
}

void Flush(State& s) {
    std::vector<EventRow> events;
    std::vector<FrameRow> frames;
    events.reserve(EVENT_RESERVE);
    frames.reserve(FRAME_RESERVE);
    {
        std::scoped_lock lock{s.event_mutex};
        events.swap(s.event_pending);
    }
    {
        std::scoped_lock lock{s.frame_mutex};
        frames.swap(s.frame_pending);
    }

    if (!events.empty() && s.events_file.IsGood()) {
        std::string text;
        text.reserve(events.size() * 64);
        for (const EventRow& row : events) {
            fmt::format_to(std::back_inserter(text), "{:.3f},{},{:016X},{:.3f},{},{},{},{}\n",
                           Ms(row.t_ns), EVENT_NAMES[static_cast<std::size_t>(row.event)], row.id,
                           Ms(row.duration_ns), (row.flags & FlagDuringLoad) ? 1 : 0,
                           (row.flags & FlagPreloaded) ? 1 : 0,
                           (row.flags & FlagStageVertex)     ? "vs"
                           : (row.flags & FlagStageFragment) ? "fs"
                           : (row.flags & FlagStageGeometry) ? "gs"
                                                             : "",
                           row.value);
        }
        s.events_file.WriteString(text);
        s.events_file.Flush();
    }

    if (!frames.empty() && s.frames_file.IsGood()) {
        std::string text;
        text.reserve(frames.size() * 32);
        for (const FrameRow& row : frames) {
            fmt::format_to(std::back_inserter(text), "{:.3f},{},{:.3f},{:.3f}\n", Ms(row.t_ns),
                           row.kind == 0 ? "system" : row.kind == 1 ? "game" : "pause",
                           static_cast<double>(row.interval_us) / 1000.0,
                           static_cast<double>(row.work_us) / 1000.0);
        }
        s.frames_file.WriteString(text);
        s.frames_file.Flush();
    }

    FileUtil::WriteStringToFile(true, s.dir + "summary.txt", BuildSummary(s));
}

void WriterLoop(State& s) {
    Common::SetCurrentThreadName("DuoStatsWriter");
    std::unique_lock lock{s.writer_mutex};
    while (!s.writer_stop) {
        s.writer_cv.wait_for(lock, FLUSH_PERIOD, [&s] { return s.writer_stop; });
        lock.unlock();
        Flush(s);
        lock.lock();
    }
}

} // Anonymous namespace

void BeginSession(u64 title_id, const std::string& device_report) {
    EndSession();

    State& s = GetState();
    s.title_id = title_id;
    s.session_start = Clock::now();
    s.loading = false;

    for (auto& per_event : s.counters) {
        for (Counter& c : per_event) {
            c.Reset();
        }
    }
    for (auto& bucket : s.histogram) {
        bucket = 0;
    }
    s.system_frames = 0;
    s.system_frame_total_ns = 0;
    s.system_frame_max_ns = 0;
    s.spikes_minor = 0;
    s.spikes_major = 0;
    s.spikes_major_with_compile = 0;
    s.game_frames = 0;
    s.discontinuities = 0;
    s.last_compile_activity_ns = 0;
    s.last_game_frame_ns = 0;
    {
        std::scoped_lock lock{s.event_mutex};
        s.event_pending.clear();
        s.event_pending.reserve(EVENT_RESERVE);
    }
    {
        std::scoped_lock lock{s.frame_mutex};
        s.frame_pending.clear();
        s.frame_pending.reserve(FRAME_RESERVE);
    }

    const std::time_t now = std::time(nullptr);
    const std::string base = FileUtil::GetUserPath(FileUtil::UserPath::LogDir) + "duo" + DIR_SEP;
    s.dir = fmt::format("{}{:%Y%m%d-%H%M%S}_{:016X}{}", base, *std::localtime(&now), title_id,
                        DIR_SEP);

    if (!FileUtil::CreateFullPath(s.dir)) {
        LOG_ERROR(Common, "DuoStats: could not create {}", s.dir);
        return;
    }

    FileUtil::WriteStringToFile(true, s.dir + "device_report.txt", device_report);

    s.events_file = FileUtil::IOFile(s.dir + "events.csv", "ab");
    s.frames_file = FileUtil::IOFile(s.dir + "frames.csv", "ab");
    if (s.events_file.IsGood()) {
        s.events_file.WriteString("t_ms,event,id,duration_ms,during_load,preloaded,stage,value\n");
    }
    if (s.frames_file.IsGood()) {
        s.frames_file.WriteString("t_ms,kind,interval_ms,work_ms\n");
    }

    {
        std::scoped_lock lock{s.writer_mutex};
        s.writer_stop = false;
    }
    s.writer = std::thread([&s] { WriterLoop(s); });
    s.active = true;

    LOG_INFO(Common, "DuoStats: session started, writing to {}", s.dir);
}

void EndSession() {
    State& s = GetState();
    if (!s.active.exchange(false)) {
        return;
    }

    {
        std::scoped_lock lock{s.writer_mutex};
        s.writer_stop = true;
    }
    s.writer_cv.notify_all();
    if (s.writer.joinable()) {
        s.writer.join();
    }

    // Catch anything recorded while the writer was shutting down.
    Flush(s);
    s.events_file.Close();
    s.frames_file.Close();
}

bool IsActive() {
    return GetState().active.load(std::memory_order_relaxed);
}

void SetLoading(bool loading) {
    GetState().loading.store(loading, std::memory_order_relaxed);
}

void Record(Event event, u64 id, Clock::duration duration, u8 flags, u64 value) {
    State& s = GetState();
    if (!s.active.load(std::memory_order_relaxed)) {
        return;
    }

    const bool loading = s.loading.load(std::memory_order_relaxed);
    if (loading) {
        flags |= FlagDuringLoad;
    }

    const u64 t_ns = NowNs(s);
    const u64 duration_ns = ToNs(duration);

    s.counters[static_cast<std::size_t>(event)][loading ? 1 : 0].Add(duration_ns);
    if (IsCompileActivity(event)) {
        s.last_compile_activity_ns.store(t_ns, std::memory_order_relaxed);
    }

    std::scoped_lock lock{s.event_mutex};
    s.event_pending.push_back(EventRow{
        .t_ns = t_ns,
        .id = id,
        .duration_ns = duration_ns,
        .value = value,
        .event = event,
        .flags = flags,
    });
}

void RecordSystemFrame(Clock::duration interval, Clock::duration work) {
    State& s = GetState();
    if (!s.active.load(std::memory_order_relaxed)) {
        return;
    }

    const u64 t_ns = NowNs(s);
    const u64 interval_ns = ToNs(interval);

    if (interval_ns > DISCONTINUITY_NS) {
        s.discontinuities.fetch_add(1, std::memory_order_relaxed);
        std::scoped_lock lock{s.frame_mutex};
        s.frame_pending.push_back(FrameRow{
            .t_ns = t_ns,
            .interval_us = static_cast<u32>(std::min<u64>(interval_ns / 1000, UINT32_MAX)),
            .work_us = static_cast<u32>(std::min<u64>(ToNs(work) / 1000, UINT32_MAX)),
            .kind = 2,
        });
        return;
    }

    s.system_frames.fetch_add(1, std::memory_order_relaxed);
    s.system_frame_total_ns.fetch_add(interval_ns, std::memory_order_relaxed);
    if (interval_ns > s.system_frame_max_ns.load(std::memory_order_relaxed)) {
        s.system_frame_max_ns.store(interval_ns, std::memory_order_relaxed);
    }
    const std::size_t bucket =
        std::min<std::size_t>(interval_ns / HIST_BUCKET_NS, HIST_BUCKETS - 1);
    s.histogram[bucket].fetch_add(1, std::memory_order_relaxed);

    if (interval_ns > SPIKE_MINOR_NS) {
        s.spikes_minor.fetch_add(1, std::memory_order_relaxed);
    }
    if (interval_ns > SPIKE_MAJOR_NS) {
        s.spikes_major.fetch_add(1, std::memory_order_relaxed);
        const u64 frame_start_ns = t_ns > interval_ns ? t_ns - interval_ns : 0;
        const u64 last_activity = s.last_compile_activity_ns.load(std::memory_order_relaxed);
        if (last_activity != 0 && last_activity >= frame_start_ns) {
            s.spikes_major_with_compile.fetch_add(1, std::memory_order_relaxed);
        }
    }

    std::scoped_lock lock{s.frame_mutex};
    s.frame_pending.push_back(FrameRow{
        .t_ns = t_ns,
        .interval_us = static_cast<u32>(std::min<u64>(interval_ns / 1000, UINT32_MAX)),
        .work_us = static_cast<u32>(std::min<u64>(ToNs(work) / 1000, UINT32_MAX)),
        .kind = 0,
    });
}

void RecordGameFrame() {
    State& s = GetState();
    if (!s.active.load(std::memory_order_relaxed)) {
        return;
    }

    const u64 t_ns = NowNs(s);
    const u64 previous = s.last_game_frame_ns.exchange(t_ns, std::memory_order_relaxed);
    const u64 interval_ns = previous != 0 && t_ns > previous ? t_ns - previous : 0;
    s.game_frames.fetch_add(1, std::memory_order_relaxed);

    std::scoped_lock lock{s.frame_mutex};
    s.frame_pending.push_back(FrameRow{
        .t_ns = t_ns,
        .interval_us = static_cast<u32>(std::min<u64>(interval_ns / 1000, UINT32_MAX)),
        .work_us = 0,
        .kind = 1,
    });
}

} // namespace Common::DuoStats
