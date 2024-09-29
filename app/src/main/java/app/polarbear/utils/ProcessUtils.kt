package app.polarbear.utils

import java.io.BufferedReader
import java.io.File
import java.io.IOException
import java.io.InputStreamReader
import java.io.OutputStream
import kotlin.concurrent.thread

fun process(
    processBuilder: ProcessBuilder,
    input: (stdin: OutputStream) -> Unit,
    output: (output: String) -> Unit,
    onFinished: (exitCode: Int) -> Unit
) {
    try {
        // Start the process
        val process = processBuilder.redirectErrorStream(true).start()

        // Start a thread to handle output from the process
        thread {
            BufferedReader(InputStreamReader(process.inputStream)).use { reader ->
                var line: String?
                while (reader.readLine().also { line = it } != null) {
                    output(line!!)
                }
            }
        }

        // Send input to the process
        input(process.outputStream)

        // Monitor the process for completion or timeout
        thread {
            try {
                val exitValue = process.waitFor()
                if (exitValue == 0) {
                    output("=== Command executed successfully ===")
                } else {
                    output("=== Error occurred during command execution: Exit code $exitValue ===")
                }
                onFinished(exitValue)
            } catch (e: InterruptedException) {
                Thread.currentThread().interrupt()
                output("=== Command execution was interrupted ===")
            }
        }
    } catch (e: IOException) {
        output("=== Error executing command: ${e.message} ===")
    } catch (e: Exception) {
        output("=== Unexpected error: ${e.message} ===")
    }
}

fun process(
    command: List<String>,
    cwd: File? = null,
    input: (writer: OutputStream) -> Unit = {},
    output: (output: String) -> Unit = {},
    onFinished: (exitCode: Int) -> Unit = {},
    environment: Map<String, String> = mapOf()
) {
    val processBuilder = ProcessBuilder(command)
    processBuilder.environment().putAll(environment)
    cwd?.let { processBuilder.directory(it) }
    process(processBuilder, input, output, onFinished)
}