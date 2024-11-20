package app.polarbear.compositor

import android.view.Surface
import app.polarbear.data.TouchEventData

interface NativeEventHandler {
    fun createSurface(id: Int)
    fun ready()
}

class NativeLib(private val eventHandler: NativeEventHandler) {

    companion object {
        // Used to load the 'compositor' library on application startup.
        init {
            System.loadLibrary("compositor")
        }
    }

    @Suppress("UNUSED")
    fun callJVM(request: String, vararg args: String): Any {
        val result = when (request) {
            "ready" -> eventHandler.ready()
            "createSurface" -> eventHandler.createSurface(args[0].toInt())
            else -> ""
        }
        return if (result is String) {
            result
        } else {
            ""
        }
    }

    /**
     * Start the Wayland compositor.
     */
    external fun start(socketName: String): Unit;

    /**
     * MainActivity has created a SurfaceView which is ready to be used
     */
    external fun setSurface(id: Int, surface: Surface): Unit;

    /**
     * Send user input events to the compositor.
     */
    external fun sendTouchEvent(surfaceId: Int, event: TouchEventData): Unit;
}