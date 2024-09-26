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
import androidx.compose.runtime.MutableState
import androidx.compose.runtime.mutableStateOf
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.viewinterop.AndroidView
import com.example.wayland_backend.NativeLib
import io.waylander.ui.theme.WaylanderTheme
import io.waylander.utils.extractTarAsset
import java.io.File
import kotlin.concurrent.thread

class MainActivity : ComponentActivity() {
    private val progress = mutableStateOf(0)

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()

        setContent {
            WaylanderTheme {
                Scaffold(modifier = Modifier.fillMaxSize()) { innerPadding ->
                    Box(modifier = Modifier.padding(innerPadding)) {
                        WaylandDisplay()
                        Overlay(progress)
                    }
                }
            }
        }

        // Check if destination directory exists
        val archFs = File(this.filesDir, "arch");
        if (archFs.exists() && archFs.listFiles().isNotEmpty()) {
            progress.value = -1
        } else {
            // The file was renamed after packaging into APK
            Thread {
                extractTarAsset(this, "ArchLinuxARM-aarch64-latest.tar", archFs, onProgress = {
                    progress.value = it.toInt()
                })
            }.start()
        }
    }
}

@Composable
fun Overlay(progress: MutableState<Int>) {
    Box(modifier = Modifier.fillMaxSize()) {
        Text(text = if (progress.value < 0) NativeLib().stringFromJNI() else "Extraction Progress: ${progress.value}%", color = Color.White, modifier = Modifier.align(Alignment.Center))
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
