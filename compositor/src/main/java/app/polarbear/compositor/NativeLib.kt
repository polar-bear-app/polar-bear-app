package app.polarbear.compositor

import android.view.Surface

class NativeLib {

    companion object {
        // Used to load the 'compositor' library on application startup.
        init {
            System.loadLibrary("compositor")
        }
    }

    /**
     * A native method that is implemented by the 'compositor' native library,
     * which is packaged with this application.
     */
    external fun start(surface: Surface): String
}