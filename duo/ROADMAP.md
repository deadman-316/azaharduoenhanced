# Roadmap

Order revised by the project owner on 2026-11-25. The owner's primary objective is the smoothest possible gameplay, whatever the cause of
stutter: target is consistent 16.67 ms frame delivery for 60 FPS games and 33.33 ms for
30 FPS games. Primary benchmark game: Super Mario 3D Land.
The original brief is in `duo/docs/roadmap-original.md`.

| Version | Milestone | Status |
| --- | --- | --- |
| 0.1 | Baseline fork, instrumentation and device diagnostics | Done. Device-tested 2026-11-25 (see `duo/benchmarks/`) |
| 0.1.x | Frame pacing investigation (diagnostics only), added 2026-11-25 after device logs showed warm-cache stutter is mostly not compilation | In progress (0.1.2) |
| 0.2 | Full known-shader and pipeline precompilation before gameplay, with the driver cache saved right after preparation | Retained; follows the pacing investigation |
| 0.3 | Cache durability: automatic saving, corruption recovery, driver-specific separation | Planned |
| 0.4 | Parallel shader regeneration and runtime compilation scheduling | Planned |
| 0.5 | Shader cache manager, import/export, cache merging, offline batch preparation | Planned |
| 0.6 | Core CPU and GPU performance | Planned |
| 0.7 | Native dual-screen optimisation | Planned |
| 0.8 | Adaptive resolution and game profiles | Planned |
| 0.9 | Graphics enhancements | Planned |
| 0.10 | In-game menu and quality of life | Planned |
| 0.11 | Optional 3DS-inspired frontend | Planned |
| 1.0 | Stable, extensively tested release | Planned |

## Standing requirements

- Accuracy-first rendering is the default. No skipped objects or effects to improve numbers.
- Vulkan first, OpenGL kept working. Custom (Turnip) driver support retained.
- No root. Separate package ID. The official Azahar install, saves, settings and caches are
  never modified.
- Shader generator output stays identical to upstream wherever possible, so caches remain
  interchangeable. A CI guard fails the build if that stops being true while the cache
  version is still pinned.
- Every milestone: audit, baseline, explain, implement incrementally, test, signed ARM64 APK,
  changelog, device test procedure, then wait for device feedback.

## Requirements carried into 0.2 (full precompilation)

1. Emulation does not start until every valid, reconstructable known pipeline has been
   prepared, or its failure has been handled explicitly.
2. The completion barrier must not deadlock, starve workers or cause an ANR. The UI stays
   responsive and preparation can be cancelled safely.
3. The driver pipeline cache is saved immediately after successful preparation.
4. A "fast start" option keeps upstream behaviour.

## Requirements carried into 0.3 to 0.5

- Periodic driver cache saves off the emulation thread, atomic writes, verified against
  Vulkan's external synchronisation rules for `VkPipelineCache`.
- Merging of compatible portable caches with deduplication and validation, never
  concatenation.
- Standalone preparation without launching gameplay; later, whole-library batch preparation
  while charging, with cancellation and thermal safeguards.
