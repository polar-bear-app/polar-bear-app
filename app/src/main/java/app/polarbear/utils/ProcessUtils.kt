package app.polarbear.utils

import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.io.BufferedReader
import java.io.File
import java.io.IOException
import java.io.InputStreamReader
import java.io.OutputStream
import kotlin.concurrent.thread

suspend fun process(
    command: List<String>,
    cwd: File? = null,
    input: (writer: OutputStream) -> Unit = {},
    output: (output: String) -> Unit = {},
    environment: Map<String, String> = emptyMap()
) = withContext(Dispatchers.IO) {
    val processBuilder = ProcessBuilder(command).apply {
        environment().putAll(environment)
        cwd?.let { directory(it) }
    }

    // Start the process
    val process = processBuilder.redirectErrorStream(true).start()

    // Handle output from the process asynchronously
    launch {
        BufferedReader(InputStreamReader(process.inputStream)).use { reader ->
            var line: String?
            while (reader.readLine().also { line = it } != null) {
                output(line!!)
            }
        }
    }

    // Send input to the process
    input(process.outputStream)

    // Monitor the process for completion or timeout and return exit value
    val exitValue = process.waitFor()
    if (exitValue != 0) {
        throw Exception("Command ${processBuilder.command()} execution failed with exit code $exitValue")
    }
}
