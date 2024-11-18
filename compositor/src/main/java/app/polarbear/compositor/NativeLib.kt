package app.polarbear.compositor

import android.view.Surface
import app.polarbear.data.TouchEventData
import java.util.Objects

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
        external fun start(socketName: String): Unit;

        /**
         * Send user input events to the compositor.
         */
        @JvmStatic
        external fun sendTouchEvent(event: TouchEventData): Unit;

        @JvmStatic
        fun callJVM(request: String, vararg args: String?): String? {
            return try {
                // Get the method dynamically
                val method = this::class.java.getDeclaredMethod(request, Array<String>::class.java)

                // Make the method accessible
                method.isAccessible = true

                // Call the method dynamically with arguments
                method.invoke(this, args) as String?
            } catch (e: NoSuchMethodException) {
                "Unknown request: $request"
            } catch (e: Exception) {
                "Error invoking request: ${e.message}"
            }
        }

        @JvmStatic
        fun create_surface(vararg args: String?): String? {
            println(args.size)
            return "1"
        }
    }
}