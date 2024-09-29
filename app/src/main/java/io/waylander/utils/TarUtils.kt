package io.waylander.utils

import android.content.Context
import org.apache.commons.compress.archivers.tar.TarArchiveEntry
import java.io.BufferedInputStream
import java.io.File
import java.io.InputStream
import java.nio.file.Files
import java.nio.file.Path
import java.nio.file.Paths
import org.apache.commons.compress.archivers.tar.TarArchiveInputStream
import org.apache.commons.compress.compressors.gzip.GzipCompressorInputStream
import org.apache.commons.compress.utils.IOUtils

fun extractTarAsset(
    context: Context,
    assetName: String,
    destinationFolder: File,
    onProgress: (output: String) -> Unit // Progress callback function
) {
    val assetManager = context.assets
    val inputStream: InputStream = assetManager.open(assetName)

    // Wrap the input stream to handle GZIP and TAR
    val gzipInputStream = GzipCompressorInputStream(BufferedInputStream(inputStream))
    val tarInputStream = TarArchiveInputStream(gzipInputStream)

    var entry = tarInputStream.nextTarEntry
    var totalBytesRead = 0L
    var totalBytesToRead = 0L

    // First pass to calculate total size of the TAR entries
    while (entry != null) {
        totalBytesToRead += entry.size
        entry = tarInputStream.nextTarEntry
    }

    // Reset the input stream to start over
    tarInputStream.close() // Close the current stream
    val newInputStream = GzipCompressorInputStream(BufferedInputStream(assetManager.open(assetName)))
    val newTarInputStream = TarArchiveInputStream(newInputStream)

    val symlinkEntries = mutableListOf<Pair<TarArchiveEntry, File>>()

    entry = newTarInputStream.nextTarEntry

    try {
        while (entry != null) {
            val outputFile = File(destinationFolder, entry.name)

            when {
                entry.isDirectory -> {
                    outputFile.mkdirs()  // Create directory
                }
                entry.linkName.isNotEmpty() -> {
                    // Store symlink information for later
                    symlinkEntries.add(entry to outputFile)
                }
                else -> {
                    // Regular file extraction
                    outputFile.parentFile.mkdirs()  // Create parent directories
                    outputFile.outputStream().use { fileOut ->
                        val bytesRead = IOUtils.copy(newTarInputStream, fileOut)  // Copy data from TAR to file
                        totalBytesRead += bytesRead
                    }
                }
            }

            // Notify progress as a percentage
            val percentComplete = (totalBytesRead * 100 / totalBytesToRead).toInt()
            onProgress("Extracting: $percentComplete% (${entry.name})")

            entry = newTarInputStream.nextTarEntry  // Move to the next entry
        }

        // Now create symlinks after all entries are processed
        for ((symlinkEntry, symLink) in symlinkEntries) {
            // Handle symbolic link
            val linkPath = if (symlinkEntry.linkName.startsWith("/")) {
                Paths.get(destinationFolder.absolutePath).resolve(symlinkEntry.linkName.substring(1))
            } else {
                Paths.get(symLink.parentFile.absolutePath).resolve(symlinkEntry.linkName)
            }
            Files.createSymbolicLink(symLink.toPath(), linkPath) // Create symlink
        }
    } finally {
        newTarInputStream.close()  // Close the TAR input stream
    }
    onProgress("Extraction complete!")
}