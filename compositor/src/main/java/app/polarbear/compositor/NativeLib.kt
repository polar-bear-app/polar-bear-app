package app.polarbear.compositor

import android.view.Surface
import app.polarbear.data.TouchEventData

class NativeLib(private val callService: (String, Array<out String>) -> String?) {

    companion object {
        // Used to load the 'compositor' library on application startup.
        init {
            System.loadLibrary("compositor")
        }
    }

    fun callJVM(request: String, vararg args: String): String? = callService(request, args)

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