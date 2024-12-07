package app.polarbear.data

import android.view.KeyEvent

data class KeyboardEventData(
    val keyCode: Int,
    val scanCode: Int,
    val state: Int
)

fun keyboardEventToData(event: KeyEvent): KeyboardEventData {
    val state: Int = if ((event.action == KeyEvent.ACTION_DOWN))
        1 // WL_KEYBOARD_KEY_STATE_PRESSED
    else
        0 // WL_KEYBOARD_KEY_STATE_RELEASED

    return KeyboardEventData(event.keyCode, event.scanCode, state)
}