package app.polarbear

import android.R
import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.app.Service
import android.content.Intent
import android.os.Binder
import android.os.IBinder
import androidx.compose.runtime.mutableStateListOf
import androidx.core.app.NotificationCompat
import app.polarbear.compositor.NativeEventHandler
import app.polarbear.compositor.NativeLib
import app.polarbear.data.SurfaceData
import app.polarbear.utils.SafeToRetryException
import app.polarbear.utils.process
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.cancel
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.launch
import org.apache.commons.compress.compressors.xz.XZCompressorInputStream
import java.io.File
import java.io.OutputStreamWriter
import java.util.concurrent.CountDownLatch
import kotlin.concurrent.thread
import kotlin.coroutines.resume
import kotlin.coroutines.suspendCoroutine

class MainService : Service(), NativeEventHandler {

    companion object {
        const val DISPLAY = "wayland-pb"
        const val CHANNEL_ID = "ProotChannel"
        const val NOTIFICATION_ID = 1
        const val ACTION_STOP = "ACTION_STOP"
        const val ACTION_LOGS = "ACTION_LOGS"
    }

    private val serviceScope = CoroutineScope(SupervisorJob() + Dispatchers.IO)
    private val _logFlow = MutableStateFlow<List<String>>(emptyList())
    private var stdin: OutputStreamWriter? = null
    private val binder = LocalBinder()
    private var isStarted = false
    private val compositorReadyLatch = CountDownLatch(1)
    private lateinit var fsRoot: File

    val nativeLib = NativeLib(this)
    val surfaceList = mutableStateListOf<SurfaceData>()
    val logFlow: StateFlow<List<String>> = _logFlow // Expose a StateFlow to the outside

    inner class LocalBinder : Binder() {
        fun getService(): MainService = this@MainService
    }

    override fun onBind(intent: Intent?): IBinder {
        return binder
    }

    override fun onCreate() {
        super.onCreate()
        createNotificationChannel()
        startForeground(NOTIFICATION_ID, createNotification())
        fsRoot = File(applicationContext.filesDir, "archlinux-aarch64");
    }

    override fun onDestroy() {
        super.onDestroy()
        isStarted = false
        serviceScope.cancel()
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        if (intent?.action == ACTION_STOP) {
            stopSelf()
        } else if (!isStarted) {
            isStarted = true
            serviceScope.launch {
                boot() // Assuming exceptions are handled within boot()
            }
        }
        return START_NOT_STICKY
    }

    private fun createNotification(): Notification {
        val contentIntent = Intent(this, MainActivity::class.java).apply {
            flags = Intent.FLAG_ACTIVITY_SINGLE_TOP or Intent.FLAG_ACTIVITY_NEW_TASK
        }

        // Stop intent to stop the service and close the app
        val stopIntent = Intent(this, MainService::class.java).apply {
            // No action is needed here since we are directly stopping the service
        }

        val logsIntent = Intent(this, MainActivity::class.java).apply {
            action = ACTION_LOGS // Set action to indicate logs should be shown
            flags =
                Intent.FLAG_ACTIVITY_SINGLE_TOP or Intent.FLAG_ACTIVITY_NEW_TASK // Use the same flags as contentIntent
        }

        val stopPendingIntent = PendingIntent.getService(
            this,
            0,
            stopIntent,
            PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE
        )

        val logsPendingIntent = PendingIntent.getActivity(
            this,
            1,
            logsIntent,
            PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE
        )

        return NotificationCompat.Builder(this, CHANNEL_ID)
            .setContentTitle("Arch Linux is running in the background.")
            .setSmallIcon(R.drawable.ic_menu_info_details)
            .setContentIntent(
                PendingIntent.getActivity(
                    this,
                    0,
                    contentIntent,
                    PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE
                )
            )
            .addAction(R.drawable.ic_menu_close_clear_cancel, "Stop", stopPendingIntent)
            .addAction(R.drawable.ic_menu_info_details, "Logs", logsPendingIntent)
            .build()
    }

    private fun createNotificationChannel() {
        val channel = NotificationChannel(
            CHANNEL_ID,
            "Linux Background Service",
            NotificationManager.IMPORTANCE_DEFAULT
        ).apply {
            description = "Notifications for Arch Linux is running in the background."
        }
        val notificationManager =
            getSystemService(NOTIFICATION_SERVICE) as NotificationManager
        notificationManager.createNotificationChannel(channel)
    }

    /**
     * Write a line of log to the log buffer.
     */
    private fun addLogLine(line: String) {
        val logLines = _logFlow.value.toMutableList()
        // Maintain only the latest 20 lines
        if (logLines.size >= 20) {
            logLines.removeAt(0) // Remove the oldest line
        }
        logLines.add(line) // Add new log line
        _logFlow.value = logLines
    }

    /**
     * Send a command to the Arch Linux system.
     */
    fun flush(command: String) {
        this.stdin?.appendLine(command)
        this.stdin?.flush()
    }

    /**
     * Boot the Arch Linux system.
     */
    private suspend fun boot() {
        try {
            // Step 1. Install Linux FS (if not existed)
            pacstrap()

            // Step 2. Install plasma desktop
            installDependencies()

            // Step 3. Start the Wayland server in C++ code
            thread {
                nativeLib.start(DISPLAY)
            }

            // Step 4. Wait for the compositor finished setup
            suspendCoroutine {
                compositorReadyLatch.await()
                it.resume(Unit)
            }

            // Step 5. Start the Wayland compositor in proot (the compositor must support Wayland backend option).
//            val command =
//                "HOME=/root XDG_RUNTIME_DIR=/tmp WAYLAND_DISPLAY=$display WAYLAND_DEBUG=client dbus-run-session startplasma-wayland"
            val command =
                "HOME=/root XDG_RUNTIME_DIR=/tmp WAYLAND_DISPLAY=$DISPLAY WAYLAND_DEBUG=client weston --renderer=pixman --fullscreen"
            execute(command, returnOnExit = false)
        } catch (error: SafeToRetryException) {
            addLogLine("*** *** ***")
            addLogLine("POLAR BEAR SETUP INTERRUPTED!")
            addLogLine("${error.message}")
            addLogLine("*** *** ***")
        } catch (error: Exception) {
            addLogLine("*** *** ***")
            addLogLine("POLAR BEAR CRASHED!")
            addLogLine("Sorry for the inconvenience. Please understand that the product is on development stage.")
            addLogLine("Please report the crash below to our Github issue tracker:")
            addLogLine("$error")
            addLogLine("Thank you for your support to improve our app.")
            addLogLine("*** *** ***")
        } finally {
            stopSelf()
        }
    }

    /**
     * Start any command in the proot environment. Typically, we will start a Wayland compositor here.
     */
    private suspend fun execute(
        command: String,
        returnOnExit: Boolean = true,
        ignoreLogs: Boolean = false
    ) {
        val prootBin = "${this.applicationInfo.nativeLibraryDir}/proot.so"
        val rootFs = "${this.applicationInfo.dataDir}/files/archlinux-aarch64"
        process(
            listOf(
                prootBin,
                "-r", rootFs,
                "-L",
                "--link2symlink",
                "--kill-on-exit",
                "--root-id",
                "--cwd=/root",
                "--bind=/dev",
//                "--bind=\"/dev/urandom:/dev/random\"",
                "--bind=/proc",
//                "--bind=\"/proc/self/fd:/dev/fd\"",
//                "--bind=\"/proc/self/fd/0:/dev/stdin\"",
//                "--bind=\"/proc/self/fd/1:/dev/stdout\"",
//                "--bind=\"/proc/self/fd/2:/dev/stderr\"",
                "--bind=/sys",
//                "--bind=\"${rootFs}/proc/.loadavg:/proc/loadavg\"",
//                "--bind=\"${rootFs}/proc/.stat:/proc/stat\"",
//                "--bind=\"${rootFs}/proc/.uptime:/proc/uptime\"",
//                "--bind=\"${rootFs}/proc/.version:/proc/version\"",
//                "--bind=\"${rootFs}/proc/.vmstat:/proc/vmstat\"",
//                "--bind=\"${rootFs}/proc/.sysctl_entry_cap_last_cap:/proc/sys/kernel/cap_last_cap\"",
//                "--bind=\"${rootFs}/sys/.empty:/sys/fs/selinux\"",
                "/usr/bin/env", "-i",
                "\"HOME=/root\"",
                "\"LANG=C.UTF-8\"",
                "\"PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin\"",
                "\"TERM=\${TERM-xterm-256color}\"",
                "\"TMPDIR=/tmp\""
            ) + if (returnOnExit) {
                command.split(" ")
            } else {
                listOf("/bin/sh")
            },
            environment = mapOf(
                "PROOT_LOADER" to this.applicationInfo.nativeLibraryDir + "/loader.so",
                "PROOT_TMP_DIR" to this.applicationInfo.dataDir + "/files/archlinux-aarch64",
            ), output = {
                if (!ignoreLogs) {
                    addLogLine(it)
                }
            }, input = {
                if (!returnOnExit) {
                    this.stdin = it.writer()
                    this.flush(command)
                }
            }
        )
    }

    private suspend fun installDependencies() {
        var isPlasmaInstalled = true
        try {
            execute("pacman -Qg plasma", ignoreLogs = true)
        } catch (e: Exception) {
            isPlasmaInstalled = false
            addLogLine("Plasma is not installed. Proceeding with installation...")
        }
        if (!isPlasmaInstalled) {
            val pacmanLock = fsRoot.resolve("var/lib/pacman/db.lck")
            pacmanLock.delete(); // Previous installation could have failed, so delete the lock file before trying again
            try {
                execute("pacman -Syu plasma --noconfirm")
            } catch (e: Exception) {
                throw SafeToRetryException("An error occurred while installing the desktop environment using the Arch package manager. If the issue was related to network connectivity, it is safe to retry the installation once you have a stable network connection by reopening the app until it succeeds.")
            }
        }
    }

    /**
     * Extract the Arch FS into app internal files directory. Also, do any necessary initial setup.
     */
    private suspend fun pacstrap() {
        // The file was renamed after packaging into APK
        val assetName = "archlinux-aarch64-pd-v4.6.0.tar.xz"
        val tempTarFile =
            File(
                applicationContext.cacheDir,
                "arch.tar"
            )  // Temporary file to store the extracted tar
        // Pacstrap when there is no fs or the temp file is still there (the extraction progress wasn't finished)
        var shouldPacstrap = false
        if (!fsRoot.exists() || fsRoot.listFiles().isNullOrEmpty()) {
            shouldPacstrap = true
            addLogLine("Arch Linux is not installed! Installing...")
        } else if (tempTarFile.exists()) {
            shouldPacstrap = true
            addLogLine("Previous installation failed. Retrying...")
        }
        if (shouldPacstrap) {
            addLogLine("(This may take a few minutes.)")

            fsRoot.deleteRecursively()

            if (!tempTarFile.exists()) {
                // Copy the asset to the cache directory to use with tar
                applicationContext.assets.open(assetName).use { inputStream ->
                    XZCompressorInputStream(inputStream).use { xzIn ->
                        tempTarFile.outputStream().use { outputStream ->
                            xzIn.copyTo(outputStream)
                        }
                    }
                }
            }

            // Prepare the command for executing tar.so (not built-in tar since it doesn't support xattrs)
            val command = listOf(
                applicationContext.applicationInfo.nativeLibraryDir + "/tar.so", // Path to the tar executable
                "-xp", // Options should be separate
                "--acls",
                "--xattrs",
                "--xattrs-include=*",
                "-f",
                tempTarFile.absolutePath,  // Path to the tar.gz file
                "-C",
                fsRoot.parentFile!!.absolutePath  // Specify the destination directory
            )

            process(command, output = { addLogLine(it) })
            tempTarFile.delete()
        }
    }

    /**
     * JNI Callbacks
     */
    override fun createSurface(id: Int) {
        this.surfaceList.add(
            SurfaceData(id)
        )
    }

    override fun ready() {
        compositorReadyLatch.countDown()
    }

}