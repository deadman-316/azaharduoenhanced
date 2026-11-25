# Building and signing

## Toolchain (as required by upstream 2127.0.1)

| Component | Version |
| --- | --- |
| JDK | 17 |
| Gradle | 8.14.5 (wrapper) |
| Android Gradle Plugin | 8.13.2 |
| Kotlin | 2.0.20 |
| compileSdk / targetSdk / minSdk | 37 / 37 / 29 |
| NDK | 27.3.13750724 |
| CMake | 3.30.3 (minimum 3.25.0) |

## Where builds happen

APKs are built by GitHub Actions (`.github/workflows/duo-android.yml`).

A local build in the development sandbox was attempted first (2026-11-25). The Android SDK
command-line tools, NDK 27.3.13750724 and CMake 3.30.3 installed from `dl.google.com`, and
the changed C++ files compile for arm64. A full APK could not be produced there because the
sandbox network policy refuses `repo.maven.apache.org`, `repo1.maven.org`,
`plugins.gradle.org` and `services.gradle.org` (HTTP 403), and the Kotlin Gradle plugin,
Kotlin standard library, kotlinx-serialization, Coil, ini4j and two Gradle plugins are only
published there.

## Triggers

| Action | Result |
| --- | --- |
| Push to a `duo/**` branch | Signed APK attached to the workflow run (zip artifact, needs GitHub login) |
| "Run workflow" button with a release version such as `0.1.0` | The same, plus a GitHub Release tagged `duo/v0.1.0` with the APK as a direct download |
| "Run workflow" button, version left empty | Manual test build of the selected branch |
| Push a tag `duo/vX.Y.Z` | Same as a release run |

Tag and branch names contain a slash on purpose: upstream's own workflows match `*`, which
does not match names containing `/`, so they stay dormant in this fork.

## Signing

One RSA-4096 key signs every build. The keystore is **not** in the repository. It is supplied
to CI through three repository secrets:

| Secret | Meaning |
| --- | --- |
| `ANDROID_KEYSTORE_B64` | The keystore file, base64 encoded on one line |
| `ANDROID_KEY_ALIAS` | Key alias |
| `ANDROID_KEYSTORE_PASS` | Keystore and key password |

Safeguards:

- The build fails if any secret is missing; it never falls back to a debug key.
- `duo/signing-cert-sha256.txt` holds the SHA-256 fingerprint of the signing certificate
  (public information). CI fails if the built APK is signed by anything else, so every
  milestone can update the previous install.
- The build script never enables shell tracing, decodes the keystore to a temporary
  directory outside the checkout, and deletes it on exit. GitHub masks secret values in logs.
- `.gitignore` excludes `*.jks`, `*.keystore` and `github-secrets.txt`.

Keep a private backup of the keystore and password. If they are lost, a new key means
uninstalling the app (and losing its app-private data) before a new build can be installed.

### Adding the secrets from an Android phone

1. Open `https://github.com/deadman-316/azaharduoenhanced` in a browser (not the GitHub app)
   and sign in. If a menu is missing, switch the browser to "Desktop site".
2. Open the repository's **Settings**, then **Secrets and variables**, then **Actions**.
3. Choose **New repository secret**. Enter the name exactly, paste the value, save.
4. Repeat for all three secrets.
5. Open the **Actions** tab. If GitHub shows a notice that workflows are disabled on this
   fork, enable them.
6. To publish a milestone: in **Actions**, pick **duo-android**, then **Run workflow**, select
   the milestone branch, type the version (for example `0.1.0`) and run. The APK appears
   under **Releases** when the run finishes.

Releases are published by the repository owner pressing that button. The development
sandbox can push branches but is not permitted to create tags or releases.

Values are write-only once saved: GitHub never shows them again, to anyone.

## Shader cache version pin

`CMakeModules/GenerateSCMRev.cmake` hashes a list of source files into the shader cache
version. This fork adds instrumentation (and later cache management) to some of those files
without changing what the shader generators emit, which would otherwise change the hash and
force a regeneration of every imported cache.

- `duo/upstream_shader_cache_version.txt` pins the hash of the pristine upstream tag
  (`6a92a76a07a9589e6174d95aa4b14cf4` for 2127.0.1).
- `duo/upstream_generator_files.sha256` lists the hashed files this fork has **not**
  modified. CI fails if any of them differs from upstream while the pin exists.
- Hashed files the fork **has** modified (`vk_shader_util.cpp`, `vk_shader_disk_cache.*`,
  `vk_pipeline_cache.cpp`) must be reviewed by hand at every change: if a change alters
  generated SPIR-V or the on-disk format, delete the pin (caches then regenerate) or bump the
  relevant struct or file version (caches then reset).
- On every rebase onto a new upstream release: recompute the pin and the checksum list from
  the untouched upstream tag.

The pin matches official Azahar only when the official build is the same upstream version.
Caches from other versions still import; they regenerate SPIR-V on first load, or reset if
upstream changed a configuration struct in between.
