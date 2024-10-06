#!/bin/bash

# Step 1: Build the Docker image
docker build -t wayland-builder .

COMMAND="mkdir \"\$ANDROID_NDK_HOME/local\" \
     && cd /libffi \
     && ./autogen.sh \
     && cd src/aarch64 \
     && ../../configure --host=aarch64-linux-android --target=aarch64-linux-android --disable-shared --enable-static CC=\"\$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android26-clang\" CXX=\"\$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android26-clang++\" LD=\"\$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin/ld\" CPP=\"\$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android26-clang -E\" --prefix=\"\$ANDROID_NDK_HOME/local\" \
     && make \
     && make install \
     && cd /wayland \
     && rm -rf build \
     && meson setup build/ --cross-file ../wayland-crossfile.txt -Ddocumentation=false -Dscanner=false \
     && ninja -C build/ \
     && file /wayland/build/src/libwayland-server.so
"

# Step 2: Run the Docker container, mount the volume, and execute commands
docker run -it --rm \
    -v /Users/teddy/Desktop/github/polar-bear-app/wayland-backend/src/main/cpp/wayland:/wayland \
    -v /Users/teddy/Desktop/github/polar-bear-app/wayland-backend/src/main/cpp/wayland-crossfile.txt:/wayland-crossfile.txt \
    -v /Users/teddy/Desktop/github/polar-bear-app/wayland-backend/src/main/cpp/libffi:/libffi \
    wayland-builder \
    /bin/bash -c "$COMMAND"

