# Installing on the Retroid Pocket Duo

Azahar Duo Enhanced is a separate app (`org.azaharduo.enhanced`). Installing, updating or
removing it does not touch official Azahar.

## Install

1. On the handheld, open the repository's **Releases** page and download the newest
   `azahar-duo-enhanced-<version>-arm64.apk`.
2. Open the file. Allow your browser or file manager to install unknown apps if asked.
3. Launch **Azahar Duo Enhanced**. When asked for a user folder, **create and select a new,
   empty folder**, for example `AzaharDuo` on internal storage.
   **Do not select your existing Azahar folder.** A separate folder is what keeps the
   official install's saves, settings and shader caches untouched.
4. Add your games folder as usual. Pointing both apps at the same games folder is fine; games
   are only read.

Later milestones install over this one and keep its data, because they use the same package
ID and signing key.

## Import your existing shader caches (optional, no PC, no root)

This copies files. Nothing in the official folder is changed.

1. Close Azahar Duo Enhanced completely (swipe it away from recents).
2. Open a file manager and go to your **official** Azahar user folder, then
   `shaders/vulkan/transferable/`. Files are named `<title id>_vs.vkch`, `_fs.vkch`,
   `_gs.vkch` and `_pl.vkch`.
3. **Copy** (not move) the files for the games you want.
4. Paste them into `<your AzaharDuo folder>/shaders/vulkan/transferable/`. Create the
   folders if they do not exist yet.
5. Optional, for the fully seeded configuration: also copy
   `shaders/vulkan/pipeline/<title id>-*.bin` into the same path in the AzaharDuo folder.
   This is the driver's own compiled cache. It is only valid for the same device and the
   same GPU driver, which is the case when both apps use the stock driver on this handheld.

A game's title ID is shown in the name of its session folder under `log/duo/` after you have
launched it once in Azahar Duo Enhanced.

## Where the measurements are

`<your AzaharDuo folder>/log/duo/<date-time>_<title id>/`

## Uninstall

Uninstall the app like any other. The AzaharDuo folder stays until you delete it.
