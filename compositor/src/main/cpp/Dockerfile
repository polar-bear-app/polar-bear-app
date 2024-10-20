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

RUN curl -o android-ndk-r27b-linux.zip https://dl.google.com/android/repository/android-ndk-r27b-linux.zip && \
    unzip android-ndk-r27b-linux.zip && \
    rm android-ndk-r27b-linux.zip

# Set the environment variables for NDK
ENV ANDROID_NDK_HOME=/android-ndk-r27b
ENV PATH=$PATH:/android-ndk-r27b

# Set the working directory inside the container
WORKDIR /
