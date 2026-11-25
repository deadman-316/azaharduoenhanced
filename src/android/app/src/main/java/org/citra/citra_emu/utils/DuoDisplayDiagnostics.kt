// Copyright Azahar Duo Enhanced contributors
// Licensed under GPLv3 or any later version
// Refer to the LICENSE.txt file included.

package org.citra.citra_emu.utils

import android.content.Context
import android.hardware.display.DisplayManager
import android.view.Window
import org.citra.citra_emu.NativeLibrary

/**
 * Azahar Duo Enhanced: reports what Android says about every display (active mode, refresh
 * rate, supported modes) and what a window has requested. Read-only; changes nothing.
 */
object DuoDisplayDiagnostics {
    fun log(context: Context, tag: String, window: Window?) {
        try {
            val dm = context.getSystemService(Context.DISPLAY_SERVICE) as DisplayManager
            val sb = StringBuilder()
            sb.append("[display info] ").append(tag).append('\n')
            for (d in dm.displays) {
                val mode = d.mode
                sb.append("display id=").append(d.displayId)
                    .append(" name=").append(d.name)
                    .append(" state=").append(d.state)
                    .append(" flags=").append(d.flags)
                    .append(" refreshRate=").append(d.refreshRate)
                    .append(" activeMode=").append(mode.modeId)
                    .append(' ').append(mode.physicalWidth).append('x').append(mode.physicalHeight)
                    .append('@').append(mode.refreshRate)
                    .append('\n')
                for (m in d.supportedModes) {
                    sb.append("  supported mode=").append(m.modeId)
                        .append(' ').append(m.physicalWidth).append('x').append(m.physicalHeight)
                        .append('@').append(m.refreshRate)
                        .append('\n')
                }
            }
            if (window != null) {
                val attrs = window.attributes
                sb.append("window onDisplay=").append(window.peekDecorView()?.display?.displayId)
                    .append(" preferredDisplayModeId=").append(attrs.preferredDisplayModeId)
                    .append(" preferredRefreshRate=").append(attrs.preferredRefreshRate)
                    .append('\n')
            }
            NativeLibrary.duoNote(sb.toString())
        } catch (e: Exception) {
            // Diagnostics must never affect the app.
        }
    }
}
