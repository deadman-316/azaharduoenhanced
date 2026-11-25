# 2026-11-25: Super Mario 3D Land, first device logs

Evidence: **device test** by the project owner, build `ba4c77e` (0.1.0 code).

## Conditions

- Retroid Pocket Duo (`CQ7790S`, Android 16), Adreno 722.
- Driver: Turnip (Mesa `26.2.99`), pipeline cache UUID `79be319f…`.
- Vulkan, async shader compilation off, disk shader cache on, SPIR-V generator on,
  internal resolution 3x.
- Driver features: extended dynamic state yes, pipeline creation cache control yes,
  fragment shader barycentric no, geometry shaders not in use.
- CPU: 3 cores at 1.84 GHz (capacity 429), 4 at 2.40 GHz (854), plus the prime core.
- Title `0004000000054000`.

## Session 1: cold cache, 338 s of play

| Measure | Value |
| --- | --- |
| Frame interval, median / p95 / p99 / p99.9 | 15.8 / 26.4 / 99.2 / 334.8 ms |
| Frames over 33.4 ms | 447 of 17,260 |
| New shader configurations | 726 |
| New pipeline configurations | 1,300 |
| Pipelines actually compiled by the driver | 713, total 71.0 s, median 0.07 ms, p90 393 ms, max 473 ms |
| Pipelines served by the driver cache on first request | 529 |
| Render thread blocked waiting for a pipeline | 199 times, total 21.3 s, median 48 ms, max 467 ms |
| GLSL compiles | 9, total 0.16 s |
| SPIR-V generation | 664, total 0.14 s |

Findings:

- Pipeline compilation by the driver is the expensive part: about a tenth of compiles take
  around 400 ms each. Shader generation is negligible.
- 62 of the 447 long frames overlap a pipeline compile, and they account for 13.9 s of the
  35.9 s of excess frame time.
- The other 385 long frames do not overlap any compile. 136 frames are almost exactly
  100 ms long, 135 of them after the 250 s mark, alternating with frames of about 36 ms.
  The cause is not known. 0.1.1 adds probes to locate it.

## Sessions 2 to 4: warm cache

Recording stopped about 5 s into each session because loading a save state rebuilds the
renderer (fixed in 0.1.1). Only the load phase is usable:

| Session | Pipelines known | Driver cache hits | Misses | Load time |
| --- | --- | --- | --- | --- |
| 2 | 1,242 | 1,242 | 0 | 110 ms |
| 3 | 1,320 | 1,301 | 19 | 110 ms |
| 4 (after session 3 ended without saving) | 1,320 | 1,301 | 19 | 108 ms |

- With a warm driver cache, all 1,242 known pipelines were recreated in 21 ms in total.
- Session 3 compiled 19 missing pipelines in the background after the game had started
  (729 ms of compile, finished 203 ms after loading returned), then ended without saving
  the driver cache. Session 4 had to compile the same 19 again. This is audit limitations
  1 and 2 observed on the device, at small scale.

## Not yet measured

Warm-cache gameplay frame times, and what happens to the caches when a save state is loaded.
