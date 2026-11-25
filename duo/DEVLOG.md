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
| Full native build and link, Kotlin/Gradle build, APK packaging | CI build (run 45596499481, JDK 17.0.21) | Pass |
| APK signature (v2), certificate fingerprint matches `duo/signing-cert-sha256.txt` | CI build | Pass |
| Package `org.azaharduo.enhanced`, label "Azahar Duo Enhanced", native code `arm64-v8a` only | CI build | Pass |
| Anything on the Retroid Pocket Duo | not done | Pending device test |

### CI notes

- First run failed in 22 s: the runner had no `JAVA_HOME_17_X64`, Gradle ran on another JVM
  and could not resolve the Android Gradle plugin. Fixed by installing JDK 17 with
  `actions/setup-java`.
- Second run passed in about 22 minutes.
