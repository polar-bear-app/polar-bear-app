package app.polarbear.data

import android.view.MotionEvent
import android.view.View

data class TouchEventData(
    val action: Int,
    val actionType: Int,
    val pointerIndex: Int,
    val x: Float,
    val y: Float
)

fun motionEventToData(event: MotionEvent, view: View): TouchEventData {

    // Get dimensions of the SurfaceView
    val viewWidth = view.width
    val viewHeight = view.height

    // Define your target surface size
    val surfaceWidth = 1024f
    val surfaceHeight = 768f

    // Calculate the scale factors
    val scaleX = surfaceWidth / viewWidth
    val scaleY = surfaceHeight / viewHeight

    // Map the touch event coordinates
    val mappedX = event.x * scaleX
    val mappedY = event.y * scaleY

    val action = event.action
    val actionType = event.actionMasked
    val pointerIndex = event.actionIndex

    return TouchEventData(action, actionType, pointerIndex, mappedX, mappedY)
}