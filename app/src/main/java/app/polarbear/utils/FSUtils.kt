package app.polarbear.utils

import android.content.Context
import androidx.compose.runtime.MutableState
import org.apache.commons.compress.compressors.xz.XZCompressorInputStream
import java.io.File
import kotlin.concurrent.thread

fun checkAndPacstrap(
    context: Context,
    addLogLine: (line: String) -> Unit,
    onFinished: (exitCode: Int) -> Unit
) {
    try {
        // The file was renamed after packaging into APK
        val assetName = "archlinux-aarch64-pd-v4.6.0.tar.xz"
        val tempTarFile =
            File(context.cacheDir, "arch.tar")  // Temporary file to store the extracted tar
        val destinationFolder = File(context.filesDir, "archlinux-aarch64");
        // Pacstrap when there is no fs or the temp file is still there (the extraction progress wasn't finished)
        var shouldPacstrap = false
        if (!destinationFolder.exists() || destinationFolder.listFiles().isNullOrEmpty()) {
            shouldPacstrap = true
            addLogLine("Arch Linux is not installed! Installing...")
        } else if (tempTarFile.exists()) {
            shouldPacstrap = true
            addLogLine("Previous installation failed. Retrying...")
        }
        if (shouldPacstrap) {
            thread {
                addLogLine("(This may take a few minutes.)")

                destinationFolder.deleteRecursively()

                if (!tempTarFile.exists()) {
                    // Copy the asset to the cache directory to use with tar
                    context.assets.open(assetName).use { inputStream ->
                        XZCompressorInputStream(inputStream).use { xzIn ->
                            tempTarFile.outputStream().use { outputStream ->
                                xzIn.copyTo(outputStream)
                            }
                        }
                    }
                }

                // Prepare the command for executing tar.so (not built-in tar since it doesn't support xattrs)
                val command = listOf(
                    context.applicationInfo.nativeLibraryDir + "/tar.so", // Path to the tar executable
                    "-xp", // Options should be separate
                    "--acls",
                    "--xattrs",
                    "--xattrs-include=*",
                    "-f",
                    tempTarFile.absolutePath,  // Path to the tar.gz file
                    "-C",
                    destinationFolder.parentFile!!.absolutePath  // Specify the destination directory
                )

                process(command, output = addLogLine, onFinished = {
                    tempTarFile.delete()
                    onFinished(it)
                })
            }
        } else {
            onFinished(0)
        }
    } catch (e: Exception) {
        addLogLine("=== Unexpected error: ${e.message} ===")
    }
}