
SET(CMAKE_SYSTEM_NAME Linux)

SET(TOOLCHAIN_DIR $ENV{HOME}/toolchain_mti)
set(CMAKE_CROSSCOMPILING true)

SET(CMAKE_CXX_COMPILER ${TOOLCHAIN_DIR}/host/usr/bin/mips-mti-linux-gnu-g++)
SET(CMAKE_C_COMPILER ${TOOLCHAIN_DIR}/host/usr/bin/mips-mti-linux-gnu-gcc)
#SET(CMAKE_FIND_ROOT_PATH ${TOOLCHAIN_DIR}/host/usr/mipsel-buildroot-linux-gnu/sysroot)
SET(CMAKE_FIND_ROOT_PATH ${TOOLCHAIN_DIR}/target/sysroot)
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
