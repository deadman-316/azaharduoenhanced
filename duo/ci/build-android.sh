#!/bin/bash
# Azahar Duo Enhanced: CI build of the signed ARM64 APK.
# Deliberately runs WITHOUT `set -x` so that secrets are never traced into the log.
set -euo pipefail

EXPECTED_PACKAGE="org.azaharduo.enhanced"
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
EXPECTED_CERT="$(tr -d '[:space:]' < "$ROOT/duo/signing-cert-sha256.txt")"

: "${ANDROID_KEYSTORE_B64:?missing}"
: "${ANDROID_KEY_ALIAS:?missing}"
: "${ANDROID_KEYSTORE_PASS:?missing}"

# Shader cache compatibility guard. The shader cache version is pinned to upstream's value
# (duo/upstream_shader_cache_version.txt). That is only valid while the files that decide
# what the shader generators emit are byte-identical to upstream. If any of them changed,
# stop: either revert the change, or delete the pin so caches are correctly invalidated
# and refresh duo/upstream_generator_files.sha256.
if [ -f "$ROOT/duo/upstream_shader_cache_version.txt" ]; then
    (cd "$ROOT" && sha256sum --check --quiet duo/upstream_generator_files.sha256) || {
        echo "::error::Shader generator sources differ from upstream while the cache version is pinned."
        exit 1
    }
    echo "Shader generator sources match upstream; pinned cache version is valid."
fi

# The keystore lives outside the checkout and is removed when the script exits.
KEYSTORE_DIR="$(mktemp -d "${RUNNER_TEMP:-/tmp}/duo-ks.XXXXXX")"
trap 'rm -rf "$KEYSTORE_DIR"' EXIT
export ANDROID_KEYSTORE_FILE="$KEYSTORE_DIR/release.jks"
printf '%s' "$ANDROID_KEYSTORE_B64" | base64 --decode > "$ANDROID_KEYSTORE_FILE"
unset ANDROID_KEYSTORE_B64

# Version name shown in Android's app info.
if [ "${GITHUB_REF_TYPE:-}" = "tag" ]; then
    export GIT_TAG_NAME="duo-${GITHUB_REF_NAME#duo/v}"
else
    export GIT_TAG_NAME="duo-dev-$(git -C "$ROOT" rev-parse --short HEAD)"
fi
echo "Version name: $GIT_TAG_NAME"

if command -v ccache > /dev/null; then
    export NDK_CCACHE="$(command -v ccache)"
fi

echo "JAVA_HOME=${JAVA_HOME:-unset}"
java -version 2>&1

cd "$ROOT/src/android"
chmod +x ./gradlew
./gradlew --no-daemon assembleVanillaRelease

shopt -s nullglob
APKS=(app/build/outputs/apk/vanilla/release/*.apk)
shopt -u nullglob
if [ "${#APKS[@]}" -ne 1 ]; then
    echo "::error::Expected exactly one release APK, found ${#APKS[@]}"
    exit 1
fi
APK="${APKS[0]}"

# ---- Verification: fail the build rather than ship something unexpected ----
BUILD_TOOLS_ALL=("${ANDROID_SDK_ROOT:-$ANDROID_HOME}"/build-tools/*)
BUILD_TOOLS="$(printf '%s\n' "${BUILD_TOOLS_ALL[@]}" | sort -V | tail -n 1)"

echo "== Signature =="
SIGNATURE="$("$BUILD_TOOLS/apksigner" verify --verbose --print-certs "$APK")"
grep -E "^Verifies|Verified using|certificate SHA-256" <<< "$SIGNATURE"
ACTUAL_CERT="$(awk '/certificate SHA-256/ {print $NF; exit}' <<< "$SIGNATURE")"
if [ "$ACTUAL_CERT" != "$EXPECTED_CERT" ]; then
    echo "::error::APK is signed with certificate $ACTUAL_CERT but duo/signing-cert-sha256.txt expects $EXPECTED_CERT"
    exit 1
fi

echo "== Package and ABI =="
BADGING="$("$BUILD_TOOLS/aapt2" dump badging "$APK")"
grep -E "^package:|^native-code:|^application-label:|^sdkVersion|^targetSdkVersion" <<< "$BADGING"
grep -q "^package: name='$EXPECTED_PACKAGE'" <<< "$BADGING" || {
    echo "::error::Unexpected package ID"
    exit 1
}
NATIVE="$(sed -n "s/^native-code: //p" <<< "$BADGING")"
if [ "$NATIVE" != "'arm64-v8a'" ]; then
    echo "::error::Expected only arm64-v8a native code, found: $NATIVE"
    exit 1
fi

mkdir -p "$ROOT/duo-artifacts"
OUT="$ROOT/duo-artifacts/azahar-duo-enhanced-${GIT_TAG_NAME#duo-}-arm64.apk"
cp "$APK" "$OUT"
(cd "$ROOT/duo-artifacts" && sha256sum "$(basename "$OUT")" | tee "$(basename "$OUT").sha256")

if command -v ccache > /dev/null; then
    ccache -s || true
fi
