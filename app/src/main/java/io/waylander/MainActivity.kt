package io.waylander

import android.os.Bundle
import android.view.SurfaceHolder
import android.view.SurfaceView
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.viewinterop.AndroidView
import com.example.wayland_backend.NativeLib
import io.waylander.ui.theme.WaylanderTheme

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContent {
            WaylanderTheme {
                Scaffold(modifier = Modifier.fillMaxSize()) { innerPadding ->
                    Box(modifier = Modifier.padding(innerPadding)) {
                        WaylandDisplay()
                        Overlay()
                    }
                }
            }
        }
    }
}

@Composable
fun Overlay() {
    Box(modifier = Modifier.fillMaxSize()) {
        Text(text = NativeLib().stringFromJNI(), color = Color.White, modifier = Modifier.align(Alignment.Center))
    }
}

@Composable
fun WaylandDisplay() {
    AndroidView(
        factory = { context ->
            SurfaceView(context).apply {
                holder.addCallback(object : SurfaceHolder.Callback {
                    override fun surfaceCreated(holder: SurfaceHolder) {
                        // Surface is ready for drawing, access the Surface via holder.surface
                    }

                    override fun surfaceChanged(
                        holder: SurfaceHolder, format: Int, width: Int, height: Int
                    ) {
                        // Handle surface changes, such as size or format updates
                    }

                    override fun surfaceDestroyed(holder: SurfaceHolder) {
                        // Cleanup resources when the surface is destroyed
                    }
                })
            }
        },
        modifier = Modifier.fillMaxSize()
    )
}
