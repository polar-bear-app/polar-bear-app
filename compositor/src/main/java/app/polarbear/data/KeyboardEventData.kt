package app.polarbear.data

import android.view.KeyEvent

data class KeyboardEventData(
    val action: Int,
    val scancode: Int,
    val metaState: Int,
    val state: Int
)

fun keyboardEventToData(event: KeyEvent): KeyboardEventData {
    val action = event.action
    val scancode = event.scanCode // Linux evdev scancode
    val metaState = event.metaState // Modifier state
    val state: Int = if ((action == KeyEvent.ACTION_DOWN))
        1 // WL_KEYBOARD_KEY_STATE_PRESSED
    else
        0 // WL_KEYBOARD_KEY_STATE_RELEASED

    return KeyboardEventData(action, scancode, metaState, state)
}