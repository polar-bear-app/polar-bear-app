#include <jni.h>
#include <string>
#include <unistd.h>

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_wayland_1backend_NativeLib_executeCommand(
        JNIEnv* env,
        jobject /* this */,
        jstring jcommand) {

    const char* command = env->GetStringUTFChars(jcommand, 0);
    char buffer[1024];
    std::string result = "";

// Open the command for reading using popen()
    FILE* pipe = popen(command, "r");
    if (!pipe) {
        result = "popen() failed!";
    } else {
        // Poll the output intermittently
        while (true) {
            // Read available output
            while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                result += buffer;
            }

            // Break the loop when the pipe closes (command finishes)
            if (feof(pipe)) {
                break;
            }

            // Sleep for a while to allow more output to accumulate
            sleep(1);  // Delay for a second
        }

        // Close the pipe
        pclose(pipe);
    }
    //
    // Return the accumulated result
    return env->NewStringUTF(result.c_str());
}