# Changelog

## 0.1.2 - Frame pacing diagnostics

Diagnostics only. Emulation, rendering and caching logic are unchanged. One difference from
upstream at the Vulkan level: the `VK_GOOGLE_display_timing` extension is enabled when the
driver offers it, and each present call carries an id (with no requested present time) so
the display's own report of when the image was shown can be collected.

### Added
- `present.csv`, one row per presented image and per display report, for each window
  (0 = main, 1 = secondary):
  - `present` rows: time the finished frame waited in the present queue, and the duration
    of image acquire, copy submit and `vkQueuePresentKHR`.
  - `displayed` rows: when the display actually showed that image and the earliest it
    could have, from `VK_GOOGLE_display_timing`.
- `frames.csv` gains per-frame columns saying where the emulation thread's time went:
  frame presentation (`swap`), waiting for a free presentation frame (`frame_wait`), waiting
  for the GPU (`gpu_wait`), waiting for the render worker (`worker_wait`), speed limiter
  sleep (`limiter`), file reads with byte count (`file_read`), texture uploads with count
  (`tex_upload`) and CPU JIT cache invalidations (`jit_invalidate`).
- `summary.txt` totals for the same categories.
- `notes.txt`: what Android reports about every display (active mode and refresh rate,
  supported modes) and what the main and secondary windows requested, captured when the
  emulation screen opens, when it gains focus and when the secondary surface changes.
- `SwapchainCreated` event per window with present mode, image count, size and the
  display's refresh period; `JitCacheInvalidated` event.

### Changed
- `SlowSwapBuffers` now records only presents over 30 ms (10 ms caught every normal frame).

### Not covered
- CPU JIT translation time is not measured directly (it would need changes inside the
  JIT library). It shows up as emulation-thread work not explained by any other category,
  and by comparing a first and a second pass through the same level.
- Android Choreographer and SurfaceFlinger statistics are not read; the display timing
  extension reports the same end result (when each image reached the screen).
- Audio buffer underruns are not recorded yet.

## 0.1.1 - Measurement fixes

Still no change to emulation, rendering or caching behaviour.

### Fixed
- Measurement sessions ended as soon as the renderer was rebuilt, which happens when a save
  state is loaded. Everything after the load went unrecorded (seen in the first device
  logs, 2026-11-25). A session now lasts for the whole emulation run.

### Added
- `RendererDestroyed` marker event.
- `SlowSwapBuffers` and `SlowSchedulerWait` events, recorded only when presenting a frame or
  waiting for the Vulkan scheduler takes longer than 10 ms. Added to trace a recurring
  100 ms stall seen in the first device logs that does not coincide with compilation.

## 0.1.0 - Baseline, instrumentation and diagnostics

Based on upstream Azahar `2127.0.1`. **No emulation, rendering or caching behaviour is
changed in this milestone.** It exists to measure unmodified upstream behaviour.

### Added
- Separate Android identity: package `org.azaharduo.enhanced`, name "Azahar Duo Enhanced".
- ARM64-only build (`arm64-v8a`).
- Measurement sessions (Vulkan renderer). Each game launch writes a folder
  `log/duo/<date-time>_<title id>/` inside the fork's user folder containing:
  - `device_report.txt`: device, SoC, GPU, Vulkan driver and version, pipeline cache UUID,
    features used by the pipeline cache, all device extensions, CPU topology, memory, and the
    shader-related settings in effect.
  - `events.csv`: one row per shader or pipeline event with timestamp and duration. Separate
    event types for: SPIR-V loaded from cache, SPIR-V generation, GLSL compilation, shader
    module creation, driver cache hit, driver cache miss, pipeline compilation, pipeline
    creation failure, render thread blocked on a pipeline, skipped draw, new shader or
    pipeline configuration, driver cache save, and cache load start and end.
  - `frames.csv`: every emulated frame interval and every presented game frame.
  - `summary.txt`: frame-time percentiles, spike counts and per-event totals.
- Low-overhead design: the emulation and render threads only append to memory; a background
  thread writes to disk every 5 seconds, so data survives the app being killed.
- Shader cache version pinned to upstream's value so official Azahar 2127.0.1 caches can be
  copied in and used without regeneration, plus a CI guard that fails the build if shader
  generator sources ever drift from upstream while the pin is in place.
- GitHub Actions workflow `duo-android` producing a signed ARM64 APK, verified for signing
  certificate, package ID and ABI. Tags named `duo/v*` publish a GitHub Release.

### Known limitations
- Measurement sessions are only started by the Vulkan renderer.
- Device temperature and CPU/GPU utilisation are not recorded yet.
- "Overlapping compile activity" in the summary is a correlation, not proof of cause.
