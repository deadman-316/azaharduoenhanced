# Benchmark procedure, milestone 0.1

Purpose: find out whether shader and pipeline compilation is what causes stutter in
unmodified upstream behaviour, and record the baseline that 0.2 must beat.

First game: **Metroid: Samus Returns** (your own dump).

## Fixed conditions (same for every run)

- Graphics API: Vulkan. GPU driver: stock (system default).
- Disk shader cache: on. Async shader compilation: off. Other settings at defaults.
- Internal resolution: pick one value and keep it. Note it down.
- Same game version and same save file. Same route (below).
- Handheld charged above 50%, not charging during the run, same fan and performance mode,
  same brightness. Let it cool for 5 minutes between runs.
- No other apps in the foreground or downloading.

## Route (about 5 minutes)

1. Launch the game from the library and wait for the title screen.
2. Load the same save each time.
3. Play the same stretch each time: move through at least three rooms, fight at least two
   enemies, use a melee counter, open the map and the pause menu once each.
4. Stop at the same place. Use Azahar's own exit option unless the run says otherwise.

## Runs

| Run | Cache state before launch | How to end it | What it shows |
| --- | --- | --- | --- |
| A. Cold | Delete `shaders/vulkan/` in the AzaharDuo folder | Normal exit | Worst case; how many shaders and pipelines the route needs |
| B. Warm, clean exit | As left by run A | Normal exit | Best case upstream can do today |
| C. Warm, after kill | As left by run B. Play the route, then press Home and swipe the app away instead of exiting. Then launch and play again: this second launch is run C | Normal exit | Whether an unclean exit loses the driver cache (audit limitation 1) |
| D. Seeded | Delete `shaders/vulkan/`, then import your official caches (INSTALL.md, steps 1 to 4 only) | Normal exit | Portable cache without the driver blob (audit limitation 2) |
| E. Fully seeded | As D, plus step 5 of the import | Normal exit | Portable cache plus driver blob |

Runs A to C answer the main question. D and E can follow later.

## What to send back

After each run a new folder appears in `<AzaharDuo folder>/log/duo/`. Zip the folders (a
file manager can do this) and attach them, saying which folder is which run. Add:

- anything you saw or felt: where it stuttered, any missing graphics, any crash;
- the internal resolution used.

## How the results will be read

- `summary.txt`, `[system frames]`: median, 95th, 99th and 99.9th percentile frame interval,
  and how many frames took longer than 1.5 and 2 frame periods.
- `summary.txt`, `[events]`, phase `gameplay`: `PipelineCompile`, `GlslCompile`,
  `SpirvGenerate` and `PipelineWait` counts and total time. In run B these should be near
  zero if the cache works; in run C they show what an unclean exit costs.
- `events.csv`: `CacheLoadEnd` carries, in its `value` column, how many cached pipelines were
  still compiling when the game started. `CachedPipelinesReady` shows how long after that
  they finished.
- `frames.csv` against `events.csv` by timestamp: whether long frames coincide with compile
  events or have another cause.

If long frames in runs B and C do not line up with compile events, shader compilation is not
the main cause and the plan for 0.2 gets revisited before any code is written.
