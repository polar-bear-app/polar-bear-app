package io.waylander

import android.os.Bundle
import android.view.SurfaceHolder
import android.view.SurfaceView
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.MutableState
import androidx.compose.runtime.mutableStateOf
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.unit.dp
import androidx.compose.ui.viewinterop.AndroidView
import com.example.wayland_backend.NativeLib
import io.waylander.ui.theme.WaylanderTheme
import io.waylander.utils.extractTarAsset
import java.io.File
import kotlin.concurrent.thread

class MainActivity : ComponentActivity() {
    private val stdout = mutableStateOf("")

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()

        setContent {
            WaylanderTheme {
                Scaffold(modifier = Modifier.fillMaxSize()) { innerPadding ->
                    Box(modifier = Modifier.padding(innerPadding)) {
                        WaylandDisplay()
                        Overlay(stdout)
                    }
                }
            }
        }

        // Check if destination directory exists
        val archFs = File(this.filesDir, "arch");
        if (!archFs.exists() || archFs.listFiles().isNullOrEmpty()) {
            stdout.value = "Arch Linux is not installed! Installing..."
            // The file was renamed after packaging into APK
            thread {
                extractTarAsset(this, "ArchLinuxARM-aarch64-latest.bin", archFs, onProgress = {
                    stdout.value = "${it}"
                })
                proot();
            }
        } else {
            proot();
        }
    }

    private fun proot() {
        val appInfo = this.applicationInfo
        val command = "export PROOT_LOADER=" + appInfo.nativeLibraryDir + "/loader.so PROOT_TMP_DIR=" + appInfo.dataDir + "/files/arch/var/cache && printf '%s\\n' 'pacman --help' | " + appInfo.nativeLibraryDir + "/proot.so -r " + appInfo.dataDir + "/files/arch -0 -w / -b /dev -b /proc -b /sys --link2symlink 2>&1"
//        val command = "ls -la " + appInfo.dataDir + "/files/arch/bin/sh"
        stdout.value = NativeLib().executeCommand(command)
    }
}

@Composable
fun Overlay(stdout: MutableState<String>) {
    // Remember a scroll state
    val scrollState = rememberScrollState()

    Box(modifier = Modifier.fillMaxSize()) {
        // Wrap Text in a Column with verticalScroll
        Column(
            modifier = Modifier
                .fillMaxSize()
                .verticalScroll(scrollState) // Enable vertical scrolling
                .padding(16.dp) // Optional: Add some padding
        ) {
            Text(
                text = stdout.value,
                color = Color.White,
                modifier = Modifier.align(Alignment.Start) // Align text to the start
            )
        }
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
