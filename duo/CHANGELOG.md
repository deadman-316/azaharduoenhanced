# Changelog

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
