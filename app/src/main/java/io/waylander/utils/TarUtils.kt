package io.waylander.utils

import android.content.Context
import org.apache.commons.compress.archivers.tar.TarArchiveEntry
import org.apache.commons.compress.archivers.tar.TarArchiveInputStream
import org.apache.commons.compress.compressors.gzip.GzipCompressorInputStream
import java.io.File
import java.io.FileOutputStream

fun extractTarAsset(
    context: Context,
    assetName: String,
    destinationFolder: File,
    onProgress: (Float) -> Unit // Progress callback function
) {
    // Create a directory to extract files into
    if (!destinationFolder.exists()) {
        destinationFolder.mkdir()
    }

    // Open the asset file to calculate the total size
    val totalSize: Long
    val entries = mutableListOf<TarArchiveEntry>()

    context.assets.open(assetName).use { inputStream ->
        TarArchiveInputStream(inputStream).use { tarInputStream ->
            var entry: TarArchiveEntry?
            totalSize = tarInputStream.use {
                var size = 0L
                while (it.nextTarEntry.also { entry = it } != null) {
                    entries.add(entry!!)
                    size += entry!!.size // Add the size of each entry
                }
                size
            }
        }
    }

    // Extract files and update progress
    var extractedSize: Long = 0

    context.assets.open(assetName).use { inputStream ->
        TarArchiveInputStream(inputStream).use { tarInputStream ->
            var entry: TarArchiveEntry?
            while (tarInputStream.nextTarEntry.also { entry = it } != null) {
                val outputFile = File(destinationFolder, entry!!.name)
                if (entry!!.isDirectory) {
                    // Create directories as needed
                    outputFile.mkdirs()
                } else {
                    // Create parent directories for the file
                    outputFile.parentFile.mkdirs()
                    // Write the extracted file
                    FileOutputStream(outputFile).use { outputStream ->
                        val buffer = ByteArray(1024) // Buffer for reading
                        var bytesRead: Int
                        var bytesWritten: Long = 0

                        // Read from the tar input stream and write to the output file
                        while (tarInputStream.read(buffer).also { bytesRead = it } != -1) {
                            outputStream.write(buffer, 0, bytesRead)
                            bytesWritten += bytesRead
                            extractedSize += bytesRead
                            // Calculate and update the progress
                            val progress = (extractedSize.toFloat() / totalSize) * 100
                            onProgress(progress) // Update progress
                        }
                    }
                }
            }
        }
    }
}