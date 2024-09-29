package com.example.wayland_backend

class NativeLib {

    /**
     * A native method that is implemented by the 'wayland_backend' native library,
     * which is packaged with this application.
     */
    external fun executeCommand(command: String): String

    companion object {
        // Used to load the 'wayland_backend' library on application startup.
        init {
            System.loadLibrary("wayland_backend")
        }
    }
}