package app.polarbear.compositor

import android.view.InputEvent
import android.view.Surface
import app.polarbear.data.TouchEventData

class NativeLib {

    companion object {
        // Used to load the 'compositor' library on application startup.
        init {
            System.loadLibrary("compositor")
        }
    }

    /**
     * Start the Wayland compositor.
     * @param surface The surface to render to
     * @return The socket name this Wayland server is listening on
     */
    external fun start(surface: Surface): String

    /**
     * Send user input events to the compositor.
     */
    external fun sendTouchEvent(event: TouchEventData): Unit
}