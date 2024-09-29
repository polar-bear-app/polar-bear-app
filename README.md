# Notes

## Why the setup file is .bin and not .tar.gz?

Although the format of the file is a tar.gz, using the gz extension make Android Studio extract the file as tar. If context.assets.open is used, the input stream would be the tar file, which increases the size.

## What is proot_standalone_api28.so?

Normally, proot will extract a loader to a temporary directory and then execute the binary. However, this is not possible for Android > 28 due to the X^W security restrictions.

In order to support latest Android target, the loader is "unbundled", so you will see a proot.so and loader.so file in the jniLibs folder to maintain executability. proot.so will load loader.so via environment variable PROOT_LOADER. The 2 files is downloaded via https://github.com/green-green-avk/build-proot-android/tree/master/packages.

API <= 28 is recommended to use the proot_standalone_api28.so file nevertheless, since the file is built with a newer version of proot from termux/proot.
