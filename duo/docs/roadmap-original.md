# Original project brief

Verbatim copy of the brief supplied by the project owner on 2026-11-25.

```text
Azahar Duo Enhanced — Development Roadmap

Project overview

I want to develop a custom fork of the Azahar Nintendo 3DS emulator, specifically optimised for my full-version Retroid Pocket Duo Android handheld.

Official source repository:
https://github.com/azahar-emu/azahar

Project name: Azahar Duo Enhanced

Use the latest stable upstream release as the foundation. Verify the actual hardware specifications of the full Retroid Pocket Duo, not the Lite version.

The objective is to create the smoothest, most polished and best-performing Nintendo 3DS emulation experience possible on this particular handheld.

We are not simply looking to change default settings. I want genuine, measurable improvements to the emulator's underlying code, rendering architecture, shader management, performance and dual-screen integration.

Non-negotiable requirements

- Preserve graphical accuracy and game compatibility.
- Prioritise consistent frame times over maximum FPS.
- Prioritise Vulkan, while retaining OpenGL compatibility.
- No root access required.
- Use a separate Android package ID so the custom build can coexist with official Azahar.
- Do not modify or overwrite my existing Azahar installation, saves, settings or shader caches.
- Retain compatibility with custom Turnip GPU drivers.
- Make changes incrementally with proper version control and rollback points.
- Audit existing functionality before implementing replacements.
- Comply with the upstream open-source licence.
- Never claim that something works on the Retroid Pocket Duo unless it has actually been tested on the device.

Phase 1 — Shader precompilation and stutter elimination

This is the absolute first priority. Do not move on to subsequent phases until this phase has been implemented and tested satisfactorily.

I want to eliminate as much shader and Vulkan pipeline compilation stuttering during gameplay as technically possible.

I am perfectly happy to wait longer before a game launches if it results in a smoother gameplay experience. I am also happy to dedicate several gigabytes of storage to shader and pipeline caches if necessary.

1.1 — Technical audit

Investigate Azahar's existing implementation of:

- Vulkan shader disk caching.
- Vulkan pipeline caching.
- Shader translation and generation.
- Asynchronous shader compilation.
- Pipeline compilation workers.
- Startup shader loading.
- Shader cache persistence.
- GPU driver-specific cache compatibility.
- Cache invalidation.
- Android storage management.
- Synchronisation and blocking operations during rendering.

Identify precisely what Azahar already does, what causes compilation-related stuttering and what can genuinely be improved.

Establish a baseline against the unmodified upstream version.

1.2 — Persistent shader and pipeline storage

Implement or improve a robust persistent caching system that:

- Stores discovered shader configurations on the device.
- Stores compiled shaders and Vulkan pipeline cache data wherever technically supported.
- Maintains separate caches for individual games.
- Preserves caches across emulator restarts and device reboots.
- Saves new cache entries periodically during gameplay.
- Uses safe, atomic cache writes where appropriate.
- Protects against cache corruption following crashes or unexpected shutdowns.
- Separates incompatible caches by GPU, driver, emulator version and relevant graphics settings.
- Retains reusable shader information when compiled driver-specific data becomes incompatible.
- Avoids unnecessary recompilation of previously encountered shaders.

Investigate the feasibility of maintaining both a portable shader configuration database and a device-specific compiled pipeline cache.

1.3 — Pre-game shader and pipeline compilation

Introduce a dedicated startup preparation stage that runs before gameplay begins.

The system should:

1. Identify the selected game.
2. Load its existing shader and pipeline information.
3. Check cache compatibility.
4. Identify known shaders and pipelines requiring compilation.
5. Compile them before emulation begins.
6. Save the resulting cache data.
7. Launch the game once preparation is complete.

Investigate whether all previously observed pipeline configurations can be reconstructed and explicitly prepared, rather than merely loading an opaque Vulkan pipeline cache.

Provide a proper loading screen displaying:

- Game name.
- Preparation stage.
- Shader compilation progress.
- Pipeline compilation progress.
- Number of known shaders.
- Number of cached shaders.
- Number of newly compiled shaders.
- Elapsed time.
- Estimated remaining time, where reliable.

Allow the user to choose between full preparation and a faster startup.

1.4 — First-playthrough shader discovery

Investigate every realistic method of discovering shaders before they are encountered during normal gameplay, including:

- Static analysis of game assets.
- Analysis of PICA200 shader programs.
- Discovery of shader configurations and rendering state combinations.
- Recorded rendering traces.
- Automated shader collection.
- Importing compatible shader configuration databases.
- Optional offline shader preparation.

Do not assume that every possible shader or pipeline can be discovered statically.

Clearly distinguish between shaders that are known, shaders that have been compiled and shader configurations that have not yet been encountered.

The ultimate goal is to minimise first-playthrough stuttering as well as repeat-playthrough stuttering.

1.5 — Runtime compilation optimisation

For previously unknown shaders encountered during gameplay:

- Investigate non-blocking compilation wherever correctness permits.
- Optimise compilation worker scheduling.
- Avoid unnecessarily blocking the emulation or rendering threads.
- Investigate duplicate compilation requests.
- Optimise compilation queue priorities.
- Save newly discovered configurations for subsequent launches.
- Avoid introducing invisible objects, missing effects or graphical corruption.
- Measure CPU contention caused by background compilation.

Provide an accuracy-first mode and, if justified by testing, an optional more aggressive asynchronous mode.

1.6 — Shader cache manager

Create a dedicated shader management section within the emulator.

Include:

- Per-game shader counts.
- Pipeline counts.
- Cache storage usage.
- Cache compatibility status.
- Compilation progress.
- Cache hit and miss statistics.
- Prepare shaders now.
- Rebuild cache.
- Delete individual game caches.
- Import and export compatible shader databases.
- Prepare all games in the library.

Investigate an optional batch compilation mode that can prepare known shaders for multiple games while the handheld is charging and idle.

Do not display a percentage implying that every possible shader in a game has been discovered unless this can genuinely be established.

1.7 — Benchmarking and acceptance criteria

Compare the modified emulator against official Azahar using identical game versions, settings, GPU drivers and device conditions.

Measure:

- Average FPS.
- Emulation speed.
- 1% and 0.1% low FPS.
- Median, 95th and 99th percentile frame times.
- Shader-related frame-time spikes.
- Shader and pipeline compilation events.
- Rendering thread blocking time.
- Cache hit rates.
- Startup preparation time.
- Cache storage requirements.
- CPU and GPU utilisation where available.
- Thermal behaviour.

Test the following scenarios:

- Completely cold shader cache.
- First gameplay session.
- Second gameplay session.
- Emulator restart.
- Device reboot.
- GPU driver change.
- Emulator update.
- Unexpected emulator termination.
- Extended gameplay.

Suggested initial test games:

- Metroid: Samus Returns.
- Luigi's Mansion 2.
- Super Mario 3D Land.
- Mario Kart 7.
- Pokémon Ultra Sun.
- The Legend of Zelda: Ocarina of Time 3D.
- The Legend of Zelda: Majora's Mask 3D.

The primary success criterion is a substantial, measurable reduction in shader-related frame-time spikes, especially during repeat gameplay sessions.

The ideal outcome is no perceptible compilation-related stuttering when revisiting previously encountered content.

Do not sacrifice graphical accuracy to achieve misleading performance improvements.

Phase 2 — Core emulation performance

Once Phase 1 is stable, investigate:

- ARM64 CPU JIT optimisation.
- CPU instruction translation efficiency.
- Memory access optimisation.
- CPU thread scheduling.
- Performance-core utilisation.
- Vulkan command submission overhead.
- GPU synchronisation stalls.
- Pipeline switching overhead.
- Descriptor management.
- Texture uploads and caching.
- Redundant rendering operations.
- Memory allocation overhead.
- Audio processing overhead.
- Frame pacing and presentation timing.
- Thermal throttling during extended sessions.

Use profiling to identify actual bottlenecks before making changes.

Prioritise optimisations that benefit the Retroid Pocket Duo without compromising compatibility.

Phase 3 — Native dual-screen optimisation

Make Azahar feel like a native application designed specifically for the Retroid Pocket Duo.

Investigate and implement:

- Independent rendering and presentation for both displays.
- Reduced dual-display rendering overhead.
- Correct frame synchronisation between displays.
- Independent aspect ratio settings.
- Independent scaling and positioning.
- Pixel-perfect screen alignment.
- Accurate bottom-screen touch mapping.
- Independent screen refresh rates where supported.
- Configurable screen swapping hotkeys.
- Optional bottom-screen disabling.
- Correct sleep and wake behaviour.
- Correct handling of the Android Home button.
- Preservation of screen layouts across game launches.

Investigate the existing issue in Metroid: Samus Returns where the bottom-screen image sometimes appears shifted to the right rather than centred.

Also investigate optional independent software RGB colour correction for each display, without modifying system calibration files or requiring root access.

Colour correction should have negligible performance overhead.

Phase 4 — Adaptive graphics and intelligent performance profiles

Introduce an optional adaptive internal resolution system that:

- Detects sustained GPU bottlenecks.
- Reduces resolution when necessary.
- Restores higher resolution when performance permits.
- Avoids frequent or distracting resolution changes.
- Does not unnecessarily lower resolution during CPU bottlenecks.
- Preserves correct rendering and screen alignment.

Introduce three configurable performance presets:

- Performance.
- Balanced.
- Quality.

Develop individual game profiles covering:

- Internal resolution.
- GPU driver.
- Shader compilation behaviour.
- CPU settings.
- Texture filtering.
- Frame pacing.
- Graphics compatibility settings.
- Screen layout.
- Control mappings.

Allow profiles to be selected automatically when launching a game.

Investigate local performance analysis that can recommend optimal settings based on measured gameplay performance.

Phase 5 — Graphics enhancements

Investigate optional improvements to:

- Texture filtering.
- Texture upscaling.
- HD texture pack loading.
- Texture preloading.
- Anisotropic filtering.
- Anti-aliasing.
- Image sharpening.
- Post-processing shaders.
- Independent screen filtering.
- Widescreen support for compatible games.

All enhancements must be optional.

Prioritise improvements that deliver visible benefits without excessive performance overhead.

Phase 6 — In-game quick settings and performance monitoring

Develop a polished, controller-friendly in-game menu inspired by PPSSPP.

Allow quick access to:

- Internal resolution.
- Adaptive resolution.
- Performance presets.
- Shader settings.
- GPU driver information.
- Screen layouts.
- Screen brightness and colour correction.
- Texture filtering.
- Save and load states.
- Controller mappings.
- Performance monitoring.

Create an optional lightweight performance overlay displaying:

- FPS.
- Emulation speed.
- Frame times.
- CPU and GPU utilisation where available.
- Shader compilation activity.
- Cache hits and misses.
- Device temperature where accessible.
- Internal resolution.
- Memory usage.

The overlay should help identify the cause of performance problems rather than simply displaying FPS.

Phase 7 — Quality-of-life improvements

Investigate and implement:

- Automatic save states on exit.
- Automatic resume.
- Rotating save backups.
- Save-state thumbnails.
- Per-game controller mappings.
- Configurable hotkeys.
- Right-stick camera controls for supported games.
- Configurable vibration.
- Fast-forward controls.
- Improved game library management.
- Save import and export.
- Optional rewind, if performance and memory overhead are acceptable.
- Reliable handling of sleep, wake and interrupted gameplay.

Protect existing in-game saves at all times.

Phase 8 — Nintendo 3DS-inspired frontend

Once the emulator's performance and core functionality are stable, develop an optional dual-screen frontend inspired by the original Nintendo 3DS operating system.

Features should include:

- Game selection on the bottom touchscreen.
- Game artwork and banners on the top screen.
- Full physical controller navigation.
- Customisable themes.
- Game folders.
- Recently played games.
- Favourites.
- Activity log.
- Playtime statistics.
- Optional startup animation.
- Direct boot into the game library.
- Seamless transitions between the frontend and gameplay.

The frontend should be lightweight and should not consume unnecessary resources while games are running.

Development milestones

Version| Milestone
0.1| Baseline fork, Android build and profiling
0.2| Persistent shader and pipeline cache improvements
0.3| Pre-game shader and pipeline compilation
0.4| Shader discovery, runtime optimisation and cache manager
0.5| Core CPU and GPU performance improvements
0.6| Native Retroid Pocket Duo dual-screen optimisation
0.7| Adaptive resolution and intelligent game profiles
0.8| Graphics enhancements
0.9| In-game menu and quality-of-life improvements
0.10| Optional Nintendo 3DS-inspired frontend
1.0| Stable, extensively tested release

Development protocol

For every milestone:

1. Audit the relevant upstream source code.
2. Identify existing functionality.
3. Establish a measurable baseline.
4. Explain the proposed changes.
5. Identify risks and technical limitations.
6. Implement changes incrementally.
7. Run automated tests where possible.
8. Check for graphical and compatibility regressions.
9. Build a signed ARM64 Android APK.
10. Provide a changelog and installation instructions.
11. Provide a specific Retroid Pocket Duo testing procedure.
12. Await my device testing and feedback before progressing.

Maintain a persistent project roadmap, technical documentation, benchmark history and development log in the repository.

Keep the fork maintainable so that future upstream Azahar improvements can be incorporated.

Distinguish clearly between verified results, desktop tests, Android build verification, actual device tests and theoretical performance improvements.

Immediate instruction

Begin with Phase 1 only.

Clone the latest stable Azahar source, verify the Retroid Pocket Duo hardware, audit the existing shader and Vulkan pipeline caching architecture and establish an unmodified baseline.

Investigate the most effective way to precompile all previously discovered shaders and pipelines before a game starts.

I want particular attention paid to the possibility of eliminating first-playthrough stuttering through advanced shader discovery, portable shader configuration databases and offline preparation.

I am willing to accept longer startup times and larger persistent caches in exchange for smoother gameplay.

Report your initial technical findings, relevant source files, existing capabilities, limitations and proposed implementation.

Do not begin unrelated frontend, graphics enhancement or dual-screen development yet.

Our first objective is simple:

The smoothest possible 3DS gameplay on the Retroid Pocket Duo, with minimal shader compilation stuttering.
```

# Milestone 0.1 approval and revised requirements

```text
I have reviewed the Phase 1 audit with ChatGPT and approve proceeding with milestone 0.1.

The findings are very promising, particularly the fact that upstream Azahar begins gameplay before all known pipelines have finished compiling.

I would like to revise the milestone order slightly:

- 0.1: Baseline fork, instrumentation and device diagnostics.
- 0.2: Full known-shader and pipeline precompilation before gameplay.
- 0.3: Persistent cache durability, automatic saving, corruption recovery and driver-specific cache separation.
- 0.4: Parallel shader regeneration and runtime compilation scheduling optimisation.
- 0.5: Shader cache manager, import/export, cache merging and offline batch preparation.

Full pre-game compilation is my highest priority.

Regarding your questions:

Android builds

Please attempt to install the required Android SDK, NDK, JDK, CMake and other dependencies locally first, using the exact versions required by upstream Azahar.

We have successfully built Android APKs in previous projects by downloading the official Android SDK tools.

If local compilation genuinely proves impossible, we can use GitHub Actions as a fallback. Report the specific blockers before switching approaches.

GPU drivers

Use the stock Qualcomm driver as the initial benchmark configuration. Retain custom GPU driver support.

Include a device diagnostics report that identifies the actual GPU, Vulkan driver, supported extensions, pipeline cache UUID, CPU topology and available memory.

We can investigate Turnip compatibility and performance separately once the baseline is established.

Shader cache seeding

Yes, I want to seed the custom emulator using copies of my existing official Azahar shader caches.

Never write to or modify the original Azahar directory.

Provide straightforward instructions for importing my existing caches directly on Android without requiring a PC or root.

Maintain separate cold-cache and seeded-cache benchmark configurations.

Additional technical requirements

1. For full precompilation mode, do not start emulation until all valid, reconstructable known pipelines have completed preparation successfully or failures have been explicitly handled.

2. Ensure that the startup completion barrier cannot introduce deadlocks, worker starvation or an Android application-not-responding condition. Keep the UI responsive and allow preparation to be cancelled safely.

3. Save the newly compiled driver cache immediately after successful startup preparation, rather than waiting until gameplay ends.

4. Distinguish between loading cached SPIR-V, creating shader modules, driver cache hits, actual pipeline compilation and failed pipeline creation.

5. Ensure that instrumentation itself does not introduce significant frame-time overhead. Prefer buffered logging and asynchronous CSV export rather than synchronous per-frame disk writes.

6. For periodic cache persistence, verify Vulkan thread-safety requirements and avoid blocking the emulation thread. Use atomic writes and appropriate durability guarantees.

7. Investigate merging compatible portable shader configuration databases from multiple sources, with deduplication and validation. Do not simply concatenate incompatible cache files.

8. Investigate a standalone shader preparation mode that can compile known shaders and pipelines without launching gameplay.

9. Eventually provide a batch preparation feature that prepares the entire installed game library while the device is charging, with cancellation and thermal safeguards.

10. Keep accuracy-first rendering as the default. Do not skip rendering objects or effects simply to achieve better benchmark results.

11. Preserve shader generator compatibility with upstream wherever possible.

12. Keep the project as an independent fork and respect upstream's AI contribution policy.

For milestone 0.1, please provide:

- A signed ARM64 Android APK.
- A unique package ID and application name.
- A persistent signing key strategy so subsequent builds can update the existing custom installation.
- A device diagnostics export.
- Shader and pipeline compilation statistics.
- Frame-time logging.
- A simple benchmark procedure.
- Build verification results.
- A changelog.

Use Metroid: Samus Returns as our first benchmark game.

I want to establish whether shader compilation is actually responsible for the stuttering, and then demonstrate a measurable improvement with full precompilation enabled.

Please proceed with milestone 0.1 only and report back when the APK is ready for device testing.
```
