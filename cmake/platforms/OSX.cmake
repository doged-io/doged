# Copyright (c) 2017 The Bitcoin developers

set(CMAKE_SYSTEM_NAME Darwin)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(TOOLCHAIN_PREFIX ${CMAKE_SYSTEM_PROCESSOR}-apple-darwin)

# Set Corrosion Rust target
set(Rust_CARGO_TARGET "x86_64-apple-darwin")

# On OSX, we use clang by default.
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

set(CMAKE_C_COMPILER_TARGET ${TOOLCHAIN_PREFIX})
set(CMAKE_CXX_COMPILER_TARGET ${TOOLCHAIN_PREFIX})

set(OSX_MIN_VERSION 11.0)
# OSX_SDK_VERSION 14.0
# Note: don't use XCODE_VERSION, it's a cmake built-in variable !
set(SDK_XCODE_VERSION 15.0)
set(SDK_XCODE_BUILD_ID 15A240d)
set(LLD_VERSION 711)

# On OSX we use various stuff from Apple's SDK.
set(OSX_SDK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/depends/SDKs/Xcode-${SDK_XCODE_VERSION}-${SDK_XCODE_BUILD_ID}-extracted-SDK-with-libcxx-headers")
set(CMAKE_OSX_SYSROOT "${OSX_SDK_PATH}")
set(CMAKE_OSX_DEPLOYMENT_TARGET ${OSX_MIN_VERSION})
set(CMAKE_OSX_ARCHITECTURES x86_64)

# target environment on the build host system
#   set 1st to dir with the cross compiler's C/C++ headers/libs
set(CMAKE_FIND_ROOT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/depends/${TOOLCHAIN_PREFIX};${OSX_SDK_PATH}")

# modify default behavior of FIND_XXX() commands to
# search for headers/libs in the target environment and
# search for programs in the build host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# When cross-compiling for Darwin using Clang, -mlinker-version must be passed
# to ensure that modern linker features are enabled.
string(APPEND CMAKE_C_FLAGS_INIT " -nostdlibinc -iwithsysroot/usr/include -iframeworkwithsysroot/System/Library/Frameworks -mlinker-version=${LLD_VERSION}")
string(APPEND CMAKE_CXX_FLAGS_INIT " -nostdlibinc -iwithsysroot/usr/include/c++/v1 -iwithsysroot/usr/include -iframeworkwithsysroot/System/Library/Frameworks -mlinker-version=${LLD_VERSION}")
string(APPEND CMAKE_EXE_LINKER_FLAGS_INIT " -Wl,-no_adhoc_codesign -fuse-ld=lld")
string(APPEND CMAKE_SHARED_LINKER_FLAGS_INIT " -Wl,-no_adhoc_codesign -fuse-ld=lld")
