# Development log

## 2026-11-25: Phase 1 audit and milestone 0.1

- Audited upstream `2127.0.1` shader and pipeline caching (`duo/docs/phase1-audit.md`).
- Local APK build attempted; blocked by sandbox network policy (`duo/docs/BUILD.md`).
  Builds moved to GitHub Actions.
- Implemented milestone 0.1 (`duo/CHANGELOG.md`).

### Verification status of 0.1

| Item | Evidence | Result |
| --- | --- | --- |
| Changed and new C++ files (9 translation units) | arm64 compile, NDK 27.3.13750724, upstream flags | Pass |
| CMake configure for Android arm64, shader cache version pin applied | arm64 configure | Pass (pinned `6a92a76a…`, would otherwise be `5435b225…`) |
| Statistics module: multi-threaded recording, periodic flush, CSV and summary output, session restart | host test under ThreadSanitizer with stubbed file I/O | Pass, no data races reported |
| Shader generator guard list matches upstream | host test (`sha256sum --check`) | Pass |
| Full native library link | not done | Pending CI build |
| Kotlin/Gradle build, APK packaging, signing | not done (only resource and Gradle value changes were made) | Pending CI build |
| Anything on the Retroid Pocket Duo | not done | Pending device test |
