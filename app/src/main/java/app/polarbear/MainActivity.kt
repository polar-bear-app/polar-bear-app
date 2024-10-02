package app.polarbear

import android.os.Bundle
import android.view.SurfaceHolder
import android.view.SurfaceView
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.material3.TextField
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.focus.FocusRequester
import androidx.compose.ui.focus.focusRequester
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import androidx.compose.ui.viewinterop.AndroidView
import app.polarbear.ui.theme.PolarBearTheme
import app.polarbear.utils.checkAndPacstrap
import app.polarbear.utils.process
import java.io.File
import java.io.OutputStreamWriter

class MainActivity : ComponentActivity() {
    private val stdout = mutableStateOf("")
    private var stdin: OutputStreamWriter? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()

        setContent {
            PolarBearTheme {
                Scaffold(modifier = Modifier.fillMaxSize()) { innerPadding ->
                    Box(modifier = Modifier.padding(innerPadding)) {
                        WaylandDisplay()
                        Overlay()
                    }
                }
            }
        }

        checkAndPacstrap(this, stdout) {
            proot()
        }
    }

    private fun proot() {
        val appInfo = this.applicationInfo
        process(
            listOf(
                appInfo.nativeLibraryDir + "/proot.so",
                "-r", appInfo.dataDir + "/files/archlinux-aarch64",
                "-0",
                "-w", "/",
                "-b", "/dev",
                "-b", "/proc",
                "-b", "/sys",
                "-b", "/proc/net:/proc/net",
                "-b", "/sys/class/net:/sys/class/net",
                "-b", "/dev/net:/dev/net",
                "--link2symlink",
                "--kill-on-exit"
            ), environment = mapOf(
                "PROOT_LOADER" to appInfo.nativeLibraryDir + "/loader.so",
                "PROOT_TMP_DIR" to appInfo.dataDir + "/files/archlinux-aarch64",
            ), output = {
                stdout.value += "${it}\n"
            }, input = {
                this.stdin = it.writer()
                this.stdin?.appendLine("uname -a")
                stdin?.appendLine("echo '# '")
                this.stdin?.flush()
            }
        )
    }

    @Composable
    fun Overlay() {
        // Remember a scroll state
        val scrollState = rememberScrollState()
        val context = LocalContext.current
        val openAlertDialog = remember { mutableStateOf(false) }
        val userInput = remember { mutableStateOf("") }
        val focusRequester = remember { FocusRequester() }


        Box(modifier = Modifier
            .fillMaxSize()
            .clickable {
                openAlertDialog.value = true
            }) {
            if (openAlertDialog.value) {
                AlertDialog(
                    onDismissRequest = { openAlertDialog.value = false },
                    title = { Text("Input Prompt") },
                    text = {
                        TextField(
                            modifier = Modifier
                                .fillMaxWidth()
                                .focusRequester(focusRequester),
                            value = userInput.value,
                            onValueChange = { userInput.value = it },
                            label = { Text("Enter something") }
                        )
                    },
                    confirmButton = {
                        Button(
                            onClick = {
                                openAlertDialog.value = false
                                // Handle the user input here
                                stdin?.appendLine(userInput.value)
                                stdin?.appendLine("echo '# '")
                                stdin?.flush()
                                userInput.value = ""
                            }
                        ) {
                            Text("Flush")
                        }
                    },
                    dismissButton = {
                        Button(onClick = { openAlertDialog.value = false }) {
                            Text("Cancel")
                        }
                    }
                )
                // Automatically focus the TextField after the dialog is shown
                LaunchedEffect(openAlertDialog.value) {
                    if (openAlertDialog.value) {
                        focusRequester.requestFocus()
                    }
                }
            }
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

            // Automatically scroll to the end when the text changes
            LaunchedEffect(stdout.value) {
                scrollState.animateScrollTo(scrollState.maxValue) // Scroll to the end
            }
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
