# CMake toolchain for cross-building to aarch64 Linux (Cortex-A53 / EmuELEC).
set(CMAKE_SYSTEM_NAME      Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

set(CMAKE_C_COMPILER   aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

# Multiarch sysroot: libsdl2-dev:arm64 puts headers under /usr/include/SDL2
# and libs under /usr/lib/aarch64-linux-gnu/.
set(CMAKE_FIND_ROOT_PATH /usr/aarch64-linux-gnu /usr/lib/aarch64-linux-gnu /usr)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE BOTH)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)

# pkg-config arm64 sysroot
set(ENV{PKG_CONFIG_PATH} "/usr/lib/aarch64-linux-gnu/pkgconfig:/usr/share/pkgconfig")
set(ENV{PKG_CONFIG_LIBDIR} "/usr/lib/aarch64-linux-gnu/pkgconfig:/usr/share/pkgconfig")

# Cortex-A53 tuning
set(CMAKE_C_FLAGS_RELEASE   "-O2 -march=armv8-a -mtune=cortex-a53 -DNDEBUG" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -march=armv8-a -mtune=cortex-a53 -DNDEBUG" CACHE STRING "" FORCE)
