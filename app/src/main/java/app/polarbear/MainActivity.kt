package app.polarbear

import android.annotation.SuppressLint
import android.content.ComponentName
import android.content.Context
import android.content.Intent
import android.content.ServiceConnection
import android.os.Build
import android.os.Bundle
import android.os.IBinder
import android.view.KeyEvent
import android.view.SurfaceHolder
import android.view.SurfaceView
import android.view.View
import android.view.WindowInsets
import android.view.WindowInsetsController
import android.webkit.WebView
import android.webkit.WebViewClient
import android.widget.FrameLayout
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.animation.core.animateDpAsState
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.offset
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
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.layout.onSizeChanged
import androidx.compose.ui.platform.LocalConfiguration
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.viewinterop.AndroidView
import app.polarbear.data.keyboardEventToData
import app.polarbear.data.motionEventToData
import app.polarbear.ui.theme.PolarBearTheme
import kotlin.math.max
import kotlin.math.min


class MainActivity : ComponentActivity() {
    private val DOCUMENTATION_URL = "https://docusaurus.io/docs"
    private var mainService: MainService? = null
    private var serviceBound by mutableStateOf(false)
    private var leftPanelOffset by mutableStateOf(0f) // This will control the offset of the left panel
    private var rightPanelOffset by mutableStateOf(0f) // This will control the offset of the right panel
    private var rightPanelWidth by mutableStateOf(0.dp) // This will store the width of the panel
    private var leftPanelWidth by mutableStateOf(0.dp) // This will store the width of the panel

    private val serviceConnection = object : ServiceConnection {
        override fun onServiceConnected(name: ComponentName, service: IBinder) {
            val binder = service as MainService.LocalBinder
            mainService = binder.getService()
            serviceBound = true
        }

        override fun onServiceDisconnected(name: ComponentName) {
            serviceBound = false
            mainService = null
        }
    }

    private fun enableFullscreenImmersive() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            window.insetsController?.let { controller ->
                controller.hide(WindowInsets.Type.statusBars() or WindowInsets.Type.navigationBars())
                controller.systemBarsBehavior =
                    WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
            }
        } else {
            // Fallback for older versions
            @Suppress("DEPRECATION") window.decorView.systemUiVisibility =
                (View.SYSTEM_UI_FLAG_FULLSCREEN or View.SYSTEM_UI_FLAG_HIDE_NAVIGATION or View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY)
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        setContent {
            // Set the panel width based on screen width
            val configuration = LocalConfiguration.current
            val maxRightPanelWidth =
                (configuration.screenWidthDp / 2).dp // Maximum width is half of the screen width
            rightPanelWidth =
                (500.dp).coerceAtMost(maxRightPanelWidth) // Set the panel width capped at half the screen width
            leftPanelWidth = configuration.screenWidthDp.dp - rightPanelWidth


            PolarBearApp()
        }

        enableFullscreenImmersive()

        // Handle intent on initial launch
        if (intent.action == MainService.ACTION_LOGS) {
            revealRightPanel() // Call the function to reveal the right panel
        }

        Intent(this, MainService::class.java).also {
            startService(it)
            bindService(it, serviceConnection, Context.BIND_AUTO_CREATE)
        }
    }

    override fun onStop() {
        super.onStop()
        if (serviceBound) {
            unbindService(serviceConnection)
            serviceBound = false
        }
    }

    @Composable
    fun InteractiveLogView() {
        // Remember a scroll state
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
                AlertDialog(onDismissRequest = { openAlertDialog.value = false },
                    title = { Text("Input Prompt") },
                    text = {
                        TextField(modifier = Modifier
                            .fillMaxWidth()
                            .focusRequester(focusRequester),
                            value = userInput.value,
                            onValueChange = { userInput.value = it },
                            label = { Text("Enter something") })
                    },
                    confirmButton = {
                        Button(onClick = {
                            openAlertDialog.value = false
                            // Handle the user input here
                            mainService!!.flush(
                                """
                                        ${userInput.value}
                                        echo '# '
                                    """.trimIndent()
                            )
                            userInput.value = ""
                        }) {
                            Text("Flush")
                        }
                    },
                    dismissButton = {
                        Button(onClick = { openAlertDialog.value = false }) {
                            Text("Cancel")
                        }
                    })
                // Automatically focus the TextField after the dialog is shown
                LaunchedEffect(openAlertDialog.value) {
                    if (openAlertDialog.value) {
                        focusRequester.requestFocus()
                    }
                }
            }
            LogView()
        }
    }

    @Composable
    fun LogView() {
        val logs = remember { mutableStateOf(emptyList<String>()) }
        val scrollState = rememberScrollState()

        LaunchedEffect(serviceBound) {
            if (serviceBound) {
                mainService?.logFlow?.collect { newLogs ->
                    logs.value = newLogs // Update logs
                }
            }
        }

        // Automatically scroll to the end when the text changes
        LaunchedEffect(logs.value) {
            scrollState.animateScrollTo(scrollState.maxValue) // Scroll to the end
        }

        // Wrap Text in a Column with verticalScroll
        Column(
            modifier = Modifier
                .fillMaxSize()
                .verticalScroll(scrollState) // Enable vertical scrolling
                .padding(16.dp) // Optional: Add some padding
        ) {
            Text(
                text = logs.value.joinToString("\n"),
                color = Color.White,
                textAlign = TextAlign.Start,
                fontSize = 12.sp,
                fontFamily = FontFamily.Monospace
            )
        }
    }

    @Composable
    fun OnboardingView() {
        AndroidView(factory = { context ->
            WebView(context).apply {
                webViewClient = WebViewClient() // Ensures links open within the WebView
                settings.javaScriptEnabled = true // Enable JavaScript if required by the website
                loadUrl(DOCUMENTATION_URL)
            }
        })
    }

    private fun revealRightPanel() {
        // Your logic to reveal the right panel
        // You may need to modify the state variable that controls the panel's visibility
        // For example:
        rightPanelOffset = 0f
    }

    override fun onNewIntent(intent: Intent?) {
        super.onNewIntent(intent)
        setIntent(intent) // Set the new intent to access it later

        if (intent?.action == MainService.ACTION_LOGS) {
            revealRightPanel() // Call the function to reveal the right panel
        }
    }


    @Composable
    fun PolarBearApp() {
        val animatedLeftPanelOffset by animateDpAsState(targetValue = leftPanelOffset.dp)
        val animatedRightPanelOffset by animateDpAsState(targetValue = rightPanelOffset.dp)
        PolarBearTheme {
            Scaffold(modifier = Modifier.fillMaxSize()) { innerPadding ->
                Box(modifier = Modifier
                    .padding(innerPadding)
                    .fillMaxSize()
                    .pointerInput(Unit) {
                        // Use awaitPointerEventScope to track pointers
                        awaitPointerEventScope {
                            var initialPositions: List<Offset>? =
                                null // To store the initial positions of the pointers
                            var activePanel: String? =
                                null // To track which panel is being manipulated

                            while (true) {
                                val event = awaitPointerEvent() // Wait for a pointer event

                                if (event.changes.size == 2) { // Check for two fingers
                                    if (initialPositions == null) {
                                        // Record the initial positions of both pointers
                                        initialPositions = event.changes.map { it.position }

                                        // Determine the active panel based on the initial touch positions
                                        val screenWidth =
                                            this@MainActivity.resources.displayMetrics.widthPixels.toFloat()
                                        val initialTouchX = initialPositions[0].x
                                        activePanel =
                                            if (initialTouchX <= leftPanelWidth.toPx()) "left" else if (initialTouchX >= rightPanelWidth.toPx()) "right" else ""
                                    } else {
                                        // Calculate the movement of the pointers from their initial positions
                                        val currentPositions = event.changes.map { it.position }
                                        val movement = currentPositions[0].x - initialPositions[0].x

                                        // Update the appropriate panel offset
                                        if (activePanel == "left") {
                                            this@MainActivity.leftPanelOffset = max(
                                                0f, min(
                                                    leftPanelWidth.toPx(),
                                                    this@MainActivity.leftPanelOffset - movement
                                                )
                                            )
                                        } else if (activePanel == "right") {
                                            this@MainActivity.rightPanelOffset = max(
                                                0f, min(
                                                    rightPanelWidth.toPx(),
                                                    this@MainActivity.rightPanelOffset + movement
                                                )
                                            )
                                        }

                                        // Update the initial positions for continuous dragging
                                        initialPositions = currentPositions
                                    }
                                }

                                // Handle drag end
                                if (event.changes.all { !it.pressed }) {
                                    // Snap the active panel to open or closed based on its offset
                                    if (activePanel == "left") {
                                        this@MainActivity.leftPanelOffset =
                                            if (this@MainActivity.leftPanelOffset > leftPanelWidth.toPx() / 2) leftPanelWidth.toPx() else 0f
                                    } else if (activePanel == "right") {
                                        this@MainActivity.rightPanelOffset =
                                            if (this@MainActivity.rightPanelOffset > rightPanelWidth.toPx() / 2) rightPanelWidth.toPx() else 0f
                                    }

                                    // Reset variables for the next gesture
                                    initialPositions = null
                                    activePanel = null
                                }
                            }
                        }
                    }) {
                    if (serviceBound) {
                        SurfaceGroup()
                    }
                    // Left panel for onboarding information during initial setup
                    Box(
                        modifier = Modifier
                            .fillMaxHeight()
                            .width(leftPanelWidth)
                            .offset(x = -animatedLeftPanelOffset) // Animate the x-axis offset
                            .background(Color.DarkGray)
                            .align(Alignment.CenterStart),
                        contentAlignment = Alignment.Center
                    ) {
                        OnboardingView()
                    }
                    // Right panel for logs
                    Box(
                        modifier = Modifier
                            .fillMaxHeight()
                            .width(rightPanelWidth)
                            .offset(x = animatedRightPanelOffset) // Animate the x-axis offset
                            .background(Color.DarkGray)
                            .align(Alignment.CenterEnd),
                        contentAlignment = Alignment.Center
                    ) {
                        InteractiveLogView()
                    }
                }
            }
        }
    }

    @Composable
    fun SurfaceGroup() {
        val density = LocalDensity.current.density.toInt()

        // Using AndroidView to create a custom FrameLayout container
        AndroidView(modifier = Modifier
            .fillMaxSize()
            .onSizeChanged { size ->
                // Automatically resize the output to match the size of the SurfaceView
                mainService!!.nativeLib.setDisplaySize(
                    size.width, size.height, density
                )
            }, factory = { context ->
            // Create a container FrameLayout to hold multiple SurfaceViews
            FrameLayout(context)
        }, update = { view ->
            view.width
            // Clear any previous views (in case of recomposition)
            // Get a list of IDs currently in the surfaceList
            val currentIds = mainService!!.surfaceList.map { it.id }.toSet()

            // Remove views that are not in the surfaceList
            val childrenToRemove = mutableListOf<View>()
            for (i in 0 until view.childCount) {
                val child = view.getChildAt(i)
                if (child.id !in currentIds) {
                    childrenToRemove.add(child)
                }
            }
            childrenToRemove.forEach { view.removeView(it) }

            // Create and add a SurfaceView for each surface in the surfaceList
            mainService!!.surfaceList.forEach { surface ->
                if (view.findViewById<SurfaceView>(surface.id) == null) {

                    val surfaceView = SurfaceView(view.context).apply {
                        // The wayland client will interact with this surface via this id
                        id = surface.id

                        // Enable touch handling on the SurfaceView
                        setOnTouchListener { view, event ->
                            // Handle touch events here
                            val data = motionEventToData(event, view)
                            mainService!!.nativeLib.sendTouchEvent(surface.id, data)
                            performClick() // click and pointer events should be handled differently
                            true // Return true to indicate the event was handled
                        }

                        holder.addCallback(object : SurfaceHolder.Callback {
                            override fun surfaceCreated(holder: SurfaceHolder) {
                                // Surface is ready for drawing, access the Surface via holder.surface
                                mainService!!.nativeLib.setSurface(surface.id, holder.surface)
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

                    // Capture keyboard events
                    if (view.childCount == 0) {
                        surfaceView.focusable = View.FOCUSABLE;
                        surfaceView.isFocusableInTouchMode = true;
                        surfaceView.requestFocus();
                    }

                    // Add the SurfaceView to the FrameLayout (Stacking them on top of each other)
                    view.addView(surfaceView, 0)
                }
            }
        })
    }

    @SuppressLint("RestrictedApi")
    override fun dispatchKeyEvent(event: KeyEvent): Boolean {
        // Send Wayland key event
        val data = keyboardEventToData(event)
        mainService!!.nativeLib.sendKeyboardEvent(data)
        return true // Consume the event
    }
}
