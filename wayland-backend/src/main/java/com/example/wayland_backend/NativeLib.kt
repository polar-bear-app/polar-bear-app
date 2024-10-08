package com.example.wayland_backend

import android.view.Surface

class NativeLib {

    companion object {
        // Used to load the 'wayland_backend' library on application startup.
        init {
            System.loadLibrary("wayland_backend")
        }
    }

    /**
     * A native method that is implemented by the 'wayland_backend' native library,
     * which is packaged with this application.
     */
    external fun start(surface: Surface): String
}