# Phase 1 audit: shader and pipeline caching in upstream Azahar 2127.0.1

Evidence level for this whole document: **source review**. Effects on frame times are
hypotheses until milestone 0.1 measurements exist.

## Summary

Upstream already keeps, per game, a portable record of shader configurations, SPIR-V and
pipeline state, and rebuilds them at boot. The work is to close gaps in that system, not to
build a cache from scratch.

## Target hardware (full Retroid Pocket Duo, not Lite)

| Item | Value |
| --- | --- |
| SoC | Qualcomm Dragonwing Q-7790 |
| CPU | 8 cores: 1 at 2.8 GHz, 4 at 2.4 GHz, 3 at 1.8 GHz |
| GPU | Adreno 722 |
| RAM / storage | 8 GB / 128 GB plus microSD |
| Screens | 5.5" 1080p AMOLED 120 Hz, 4.5" 960p AMOLED 120 Hz |
| OS | Android 16 |

From public specification sheets (Qualcomm product page, CNX Software, Android Authority).
Unverified until the device report from 0.1 is in: Vulkan extensions exposed by the stock
driver, and whether any Turnip build supports the Adreno 722.

## How it works

**Portable cache**: four append-only files per title in `shaders/vulkan/transferable/`:
`<titleid>_vs.vkch`, `_fs`, `_gs`, `_pl`. zstd-compressed entries with header and footer.
They hold PICA vertex program code, vertex/fragment/geometry shader configurations, the
SPIR-V generated from each, and the static state of every pipeline seen.

**Driver cache**: one `VkPipelineCache` blob per title in `shaders/vulkan/pipeline/`,
named `<titleid>-<vendor><device>.bin`, validated against the driver's pipeline cache UUID.

**Boot**: load the driver blob; create shader modules from stored SPIR-V; for each stored
pipeline ask the driver for it with "fail if compile required"; on a miss (or without that
extension) queue it on worker threads; then start the game.

**Invalidation**: changed config struct layout resets the file. Changed generator sources
(MD5 of a file list in `CMakeModules/GenerateSCMRev.cmake`) or changed driver feature profile
keeps configurations and regenerates SPIR-V. Changed driver UUID drops only the driver blob.

**Runtime**: new shaders go to a worker pool. With async shader compilation off (default) the
draw waits for its pipeline. With it on, draws of more than 6 vertices are skipped until
ready.

## Source files

| File | Role |
| --- | --- |
| `src/video_core/renderer_vulkan/vk_shader_disk_cache.{h,cpp}` | Portable per-title cache |
| `src/video_core/renderer_vulkan/vk_pipeline_cache.{h,cpp}` | Driver blob, worker pools, crash-on-load flag |
| `src/video_core/renderer_vulkan/vk_graphics_pipeline.{h,cpp}` | Pipeline state, hashing, `TryBuild`, `Build` |
| `src/video_core/renderer_vulkan/vk_rasterizer.cpp` | `LoadDefaultDiskResources`; wait-or-skip decision |
| `src/video_core/renderer_vulkan/vk_shader_util.cpp` | GLSL to SPIR-V (glslang), shader module creation |
| `src/video_core/shader/generator/` | Config structs and GLSL/SPIR-V generators |
| `CMakeModules/GenerateSCMRev.cmake` | Shader cache version hash |
| `src/android/app/src/main/jni/native.cpp` | Runs the cache load before the emulation loop |
| `src/android/app/.../utils/DiskShaderCacheProgress.kt` | Loading screen |
| `src/video_core/renderer_opengl/gl_shader_disk_cache.*` | OpenGL equivalent (untouched) |

## Limitations

| # | Limitation | Likely effect |
| --- | --- | --- |
| 1 | Driver blob saved only in the `PipelineCache` destructor and on title switch, written in place | A killed or crashed app loses the session's compiled pipelines |
| 2 | Boot does not wait for queued pipelines | After a driver-cache miss, pipelines compile in the background during play |
| 3 | Progress bar counts entries read, not objects compiled | Loading screen ends while compilation continues |
| 4 | A torn final entry discards the whole file; appends use `fflush`, not `fsync` | One interrupted write can lose a game's record |
| 5 | A config struct layout change drops the file, portable configurations included | An update can silently return a game to a cold cache |
| 6 | Regeneration is single-threaded on the loading thread | Slow boots after a driver or settings change |
| 7 | One SPIR-V set per title, for one driver profile | Alternating drivers regenerates each time |
| 8 | Two worker pools of `hardware_concurrency / 2` threads, no priority or affinity | Runtime compiles compete with emulation |
| 9 | Only two runtime modes: block the draw or skip it | No accuracy-preserving softening |
| 10 | Fragment shaders using custom normal maps are never cached | Recompiled every session |
| 11 | No cache UI | No counts, statistics, per-game delete, import or export |

Leading hypothesis for stutter on revisited content: 1 and 2 together.

## First-playthrough discovery

A 3DS game ships vertex shader programs but no fragment shaders. The fragment stage is
fixed-function hardware configured by registers; the emulator generates a shader per
distinct register combination when a draw uses it. Pipelines add blend state, vertex layout
and target format.

| Method | Verdict |
| --- | --- |
| Static analysis of game assets | Low yield: finds vertex programs, not fragment or pipeline state |
| Enumerating register combinations | Not feasible |
| Importing a portable configuration database | Recommended; coverage equals what the donor played |
| Seeding from the owner's official Azahar caches | Recommended first step |
| Recorded traces, automated collection | Possible later, limited to what is reached |
| Offline or batch preparation of known configurations | Feasible |

The UI must report known, compiled and newly encountered counts separately, never a
percentage of the game.
