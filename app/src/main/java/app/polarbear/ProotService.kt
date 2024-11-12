package app.polarbear

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.app.Service
import android.content.Context
import android.content.Intent
import android.os.Binder
import android.os.Build
import android.os.IBinder
import androidx.core.app.NotificationCompat
import app.polarbear.compositor.NativeLib
import app.polarbear.utils.checkAndPacstrap
import app.polarbear.utils.process
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.flow.SharedFlow
import kotlinx.coroutines.flow.asSharedFlow
import java.io.OutputStreamWriter

class ProotService : Service() {

    companion object {
        const val CHANNEL_ID = "ProotChannel"
        const val NOTIFICATION_ID = 1

        const val ACTION_STOP = "ACTION_STOP"
        const val ACTION_LOGS = "ACTION_LOGS"
    }

    private val logLines = ArrayDeque<String>(20) // Store the latest 20 log lines
    private val _logFlow = MutableSharedFlow<List<String>>(replay = 1)
    val logFlow: SharedFlow<List<String>> = _logFlow.asSharedFlow()

    private var stdin: OutputStreamWriter? = null
    private val binder = LocalBinder()
    private var isStarted = false

    inner class LocalBinder : Binder() {
        fun getService(): ProotService = this@ProotService
    }

    override fun onBind(intent: Intent?): IBinder {
        return binder
    }

    override fun onCreate() {
        super.onCreate()
        createNotificationChannel()
        startForeground(NOTIFICATION_ID, createNotification())
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {

        when (intent?.action) {
            ACTION_STOP -> {
                this.stopSelf();
            }

            else -> {
                if (!isStarted) {
                    isStarted = true

                    val display = NativeLib.start();

                    // val command = "HOME=/root XDG_RUNTIME_DIR=/tmp WAYLAND_DISPLAY=$display WAYLAND_DEBUG=client dbus-run-session startplasma-wayland"
                    val command =
                        "HOME=/root XDG_RUNTIME_DIR=/tmp WAYLAND_DISPLAY=$display WAYLAND_DEBUG=client weston --debug"

                    checkAndPacstrap(this, { addLogLine(it) }) {
                        val appInfo = this.applicationInfo
                        val rootFs = appInfo.dataDir + "/files/archlinux-aarch64"
                        process(
                            listOf(
                                appInfo.nativeLibraryDir + "/proot.so",
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
                                "\"TMPDIR=/tmp\"",
                                "/bin/sh"
                            ), environment = mapOf(
                                "PROOT_LOADER" to appInfo.nativeLibraryDir + "/loader.so",
                                "PROOT_TMP_DIR" to appInfo.dataDir + "/files/archlinux-aarch64",
                            ), output = { addLogLine(it) }, input = {
                                this.stdin = it.writer()
                                if (command != null) {
                                    this.flush(command)
                                } else {
                                    this.flush("uname -a")
                                }
                            }
                        )
                    }
                }
            }
        }
        return START_NOT_STICKY
    }

    private fun createNotification(): Notification {
        val contentIntent = Intent(this, MainActivity::class.java).apply {
            flags = Intent.FLAG_ACTIVITY_SINGLE_TOP or Intent.FLAG_ACTIVITY_NEW_TASK
        }

        // Stop intent to stop the service and close the app
        val stopIntent = Intent(this, ProotService::class.java).apply {
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
            .setSmallIcon(android.R.drawable.ic_menu_info_details)
            .setContentIntent(
                PendingIntent.getActivity(
                    this,
                    0,
                    contentIntent,
                    PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE
                )
            )
            .addAction(android.R.drawable.ic_menu_close_clear_cancel, "Stop", stopPendingIntent)
            .addAction(android.R.drawable.ic_menu_info_details, "Logs", logsPendingIntent)
            .build()
    }

    private fun createNotificationChannel() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val channel = NotificationChannel(
                CHANNEL_ID,
                "Linux Background Service",
                NotificationManager.IMPORTANCE_DEFAULT
            ).apply {
                description = "Notifications for Arch Linux is running in the background."
            }
            val notificationManager =
                getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
            notificationManager.createNotificationChannel(channel)
        }
    }

    // Method to add a log line and maintain only the latest 20 lines
    fun addLogLine(line: String) {
        if (logLines.size >= 20) {
            logLines.removeFirst() // Remove the oldest line
        }
        logLines.addLast(line) // Add new log line

        _logFlow.tryEmit(logLines.toList()) // Emit the current log list
    }

    fun flush(command: String) {
        this.stdin?.appendLine(command)
        this.stdin?.flush()
    }

    override fun onDestroy() {
        super.onDestroy()
        isStarted = false
    }
}