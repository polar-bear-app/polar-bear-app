package app.polarbear.compositor

import android.view.Surface
import app.polarbear.data.TouchEventData

class NativeLib {

    companion object {
        // Used to load the 'compositor' library on application startup.
        init {
            System.loadLibrary("compositor")
        }

        /**
         * Start the Wayland compositor.
         */
        @JvmStatic
        external fun start(): String;

        /**
         * Attach an Android Surface to render to
         */
        @JvmStatic
        external fun setSurface(surface: Surface): Unit;

        /**
         * Send user input events to the compositor.
         */
        @JvmStatic
        external fun sendTouchEvent(event: TouchEventData): Unit;
    }
}