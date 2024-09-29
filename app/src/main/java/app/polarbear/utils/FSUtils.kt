package app.polarbear.utils

import android.content.Context
import androidx.compose.runtime.MutableState
import java.io.File

fun checkAndPacstrap(
    context: Context,
    stdout: MutableState<String>,
    onFinished: (exitCode: Int) -> Unit
) {
    try {
        // The file was renamed after packaging into APK
        val assetName = "ArchLinuxARM-aarch64-latest.bin"
        val tempTarFile =
            File(context.cacheDir, assetName)  // Temporary file to store the extracted tar
        val destinationFolder = File(context.filesDir, "arch");
        // Pacstrap when there is no fs or the temp file is still there (the extraction progress wasn't finished)
        var shouldPacstrap = false
        if (!destinationFolder.exists()) {
            shouldPacstrap = true
            stdout.value = "Arch Linux is not installed! Installing...\n"
        } else if (tempTarFile.exists()) {
            shouldPacstrap = true
            stdout.value = "Previous installation failed. Retrying...\n"
        }
        if (shouldPacstrap) {
            stdout.value += "(This may take a few minutes.)\n"

            destinationFolder.deleteRecursively()
            destinationFolder.mkdirs()

            // Copy the asset to the cache directory to use with tar
            context.assets.open(assetName).use { inputStream ->
                tempTarFile.outputStream().use { outputStream ->
                    inputStream.copyTo(outputStream)
                }
            }

            // Prepare the command for executing tar.so (not built-in tar since it doesn't support xattrs)
            val command = listOf(
                context.applicationInfo.nativeLibraryDir + "/tar.so", // Path to the tar executable
                "-zxp", // Options should be separate
                "--acls",
                "--xattrs",
                "--xattrs-include=*",
                "-f", tempTarFile.absolutePath,  // Path to the tar.gz file
                "-C", destinationFolder.absolutePath  // Specify the destination directory
            )

            process(command, cwd = destinationFolder, onFinished = {
                tempTarFile.delete()
                onFinished(it)
            })
        } else {
            onFinished(0)
        }
    } catch (e: Exception) {
        stdout.value += "=== Unexpected error: ${e.message} ==="
    }
}