# Azahar Duo Enhanced

A personal, independent fork of the [Azahar](https://github.com/azahar-emu/azahar) Nintendo 3DS
emulator, tuned for the Retroid Pocket Duo (full model: Qualcomm Dragonwing Q-7790, Adreno 722).

- **Not affiliated with or endorsed by the Azahar project.** Do not report issues with this
  fork upstream.
- **Based on** upstream tag `2127.0.1` (commit `fd4c9abd`).
- **Package ID** `org.azaharduo.enhanced`, app name "Azahar Duo Enhanced". It installs
  alongside official Azahar and never reads or writes the official app's data or user folder.
- **Licence:** same as upstream (GPL; see `LICENSE.txt`). Source for every published APK is
  the tagged commit in this repository.
- **AI disclosure:** changes in this fork are written with AI assistance. Upstream's
  `AI-POLICY.md` does not accept such code, so nothing here is, or will be, submitted upstream.

## Where things are

| Path | Contents |
| --- | --- |
| `duo/ROADMAP.md` | Milestones, order and status |
| `duo/CHANGELOG.md` | What changed in each milestone |
| `duo/DEVLOG.md` | Development log and what has actually been verified, and how |
| `duo/docs/phase1-audit.md` | Technical audit of upstream shader and pipeline caching |
| `duo/docs/BUILD.md` | Toolchain, CI build, signing key and repository secrets |
| `duo/docs/INSTALL.md` | Installing on the handheld and importing existing shader caches |
| `duo/docs/BENCHMARK-0.1.md` | Benchmark procedure for milestone 0.1 |
| `duo/docs/roadmap-original.md` | The original project brief, verbatim |
| `duo/benchmarks/` | Benchmark history (results are added as they are measured) |
| `duo/upstream_shader_cache_version.txt` | Pinned shader cache version (see BUILD.md) |

## Evidence labels

Every claim in this project's documents carries one of these labels:

- **source review**: read in the code, not executed.
- **host test**: executed on a Linux x86-64 machine.
- **arm64 compile**: compiled with the Android NDK for arm64, not executed.
- **CI build**: full APK built and verified by GitHub Actions.
- **device test**: run on the Retroid Pocket Duo by the project owner.

Nothing is described as working on the Retroid Pocket Duo without a device test.
