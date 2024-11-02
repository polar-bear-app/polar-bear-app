package app.polarbear

import android.content.ComponentName
import android.content.Context
import android.content.Intent
import android.content.ServiceConnection
import android.os.Bundle
import android.os.IBinder
import android.view.SurfaceHolder
import android.view.SurfaceView
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.animation.core.animateDpAsState
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.gestures.detectHorizontalDragGestures
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.material3.TextField
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.focus.FocusRequester
import androidx.compose.ui.focus.focusRequester
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.unit.dp
import androidx.compose.ui.viewinterop.AndroidView
import app.polarbear.compositor.NativeLib
import app.polarbear.ui.theme.PolarBearTheme
import kotlin.math.max
import kotlin.math.min

class MainActivity : ComponentActivity() {

    private var serviceBound = false
    private var prootService: ProotService? = null

    private val serviceConnection = object : ServiceConnection {
        override fun onServiceConnected(name: ComponentName, service: IBinder) {
            val binder = service as ProotService.LocalBinder
            prootService = binder.getService()
            serviceBound = true
        }

        override fun onServiceDisconnected(name: ComponentName) {
            serviceBound = false
            prootService = null
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        Intent(this, ProotService::class.java).also {
            startService(it)
            bindService(it, serviceConnection, Context.BIND_AUTO_CREATE)
        }

        setContent {
            PolarBearTheme {
                Scaffold(modifier = Modifier.fillMaxSize()) { innerPadding ->
                    Box(modifier = Modifier.padding(innerPadding)) {
                        WaylandDisplay {
                            proot(it)
                        }
                        SwipeableRightPanel() {
                            Overlay()
                        }
                    }
                }
            }
        }
    }

    override fun onStop() {
        super.onStop()
        if (serviceBound) {
            unbindService(serviceConnection)
            serviceBound = false
        }
    }

    private fun proot(command: String) {
        val intent = Intent(this, ProotService::class.java).apply {
            putExtra("command", command)
        }
        startForegroundService(intent)
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
            .background(Color.Black.copy(alpha = 0.5f))
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
                                prootService!!.flush(
                                    """
                                        ${userInput.value}
                                        echo '# '
                                    """.trimIndent()
                                )
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
                if (prootService != null) {
                    Text(
                        text = prootService!!.getLogs().joinToString("\n"),
                        color = Color.White,
                        modifier = Modifier.align(Alignment.Start) // Align text to the start
                    )
                }
            }
        }
    }
}

@Composable
fun WaylandDisplay(render: (String) -> Unit) {

    AndroidView(
        factory = { context ->
            SurfaceView(context).apply {
                holder.addCallback(object : SurfaceHolder.Callback {
                    override fun surfaceCreated(holder: SurfaceHolder) {
                        // Surface is ready for drawing, access the Surface via holder.surface
                        val display = NativeLib().start(holder.surface);
                        render("XDG_RUNTIME_DIR=/tmp WAYLAND_DISPLAY=$display WAYLAND_DEBUG=client dbus-run-session startplasma-wayland")
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


@Composable
fun SwipeableRightPanel(content: @Composable () -> Unit) {
    var panelOffset by remember { mutableStateOf(0f) }
    val animatedPanelWidth by animateDpAsState(targetValue = (panelOffset).dp)

    Box(
        modifier = Modifier
            .fillMaxSize()
            .pointerInput(Unit) {
                detectHorizontalDragGestures(
                    onHorizontalDrag = { change, dragAmount ->
                        change.consume() // Consume the gesture to prevent it from propagating
                        panelOffset = max(
                            0f,
                            min(300f, panelOffset - dragAmount)
                        ) // Limit panel width between 0 and 300
                    },
                    onDragEnd = {
                        // Snap to open or closed based on drag distance
                        panelOffset = if (panelOffset > 150) 300f else 0f
                    }
                )
            }
    ) {

        // Right panel, animated width based on drag distance
        Box(
            modifier = Modifier
                .fillMaxHeight()
                .width(animatedPanelWidth)
                .background(Color.DarkGray)
                .align(Alignment.CenterEnd),
            contentAlignment = Alignment.Center
        ) {
            content()
        }
    }
}