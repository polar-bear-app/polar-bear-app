# Use the official Arch Linux image
FROM archlinux/archlinux:base

# Update and install necessary dependencies
RUN pacman -Syu --noconfirm \
    base-devel \
    meson \
    ninja \
    clang \
    llvm \
    pkgconf \
    python \
    python-pip \
    git \
    wayland \
    libffi \
    libxml2 \
    expat \
    meson \
    ninja \
    unzip \
    autoconf

# Download and extract Android NDK
RUN curl -o android-ndk-r27b-linux.zip https://dl.google.com/android/repository/android-ndk-r27b-linux.zip && \
    unzip android-ndk-r27b-linux.zip && \
    rm android-ndk-r27b-linux.zip

# Set environment variables
ENV ANDROID_NDK_HOME=/android-ndk-r27b
ENV PATH=$PATH:/android-ndk-r27b

# Set working directory for the container
WORKDIR /

COPY ndk-crossfile.txt /ndk-crossfile.txt

# Create output directory for built files
RUN mkdir /output

# Build libffi and copy to /output
RUN git clone https://github.com/libffi/libffi.git
RUN cd /libffi && \
    ./autogen.sh && \
    cd src/aarch64 && \
    ../../configure \
        --host=aarch64-linux-android \
        --target=aarch64-linux-android \
        --disable-shared \
        --enable-static \
        CC="$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android26-clang" \
        CXX="$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android26-clang++" \
        LD="$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin/ld" \
        CPP="$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android26-clang -E" \
        --prefix="/output" && \
    make && \
    make install

RUN git clone --branch $(pacman -Qi wayland | grep Version | awk '{print $3}' | cut -d'-' -f1) https://gitlab.freedesktop.org/wayland/wayland.git
RUN git clone https://gitlab.freedesktop.org/wayland/wayland-protocols.git

# Generate Wayland protocols
RUN wayland-scanner server-header < /wayland-protocols/stable/xdg-shell/xdg-shell.xml > /output/include/xdg-shell.h && \
    wayland-scanner private-code < /wayland-protocols/stable/xdg-shell/xdg-shell.xml > /output/include/xdg-shell.c

# Build Wayland
RUN cd /wayland && \
    meson setup build/ --cross-file /ndk-crossfile.txt -Ddocumentation=false -Dscanner=false -Dprefix=/output && \
    ninja -C build/ && \
    ninja -C build/ install
