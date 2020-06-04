# Cross-compiling requires CMake 2.6 or newer. To cross-compile, first modify
# this file to set the proper settings and paths. Then use it from build/ like:
# cmake .. -DCMAKE_TOOLCHAIN_FILE=../XCompile.txt \
#          -DCMAKE_INSTALL_PREFIX=/usr/mingw32/mingw
# If you already have a toolchain file setup, you may use that instead of this
# file.

SET(NACL 1)

# the name of the target operating system
SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_VERSION 1)
SET(CMAKE_SYSTEM_PROCESSOR ${NACL_ARCH})

# which compilers to use for C and C++
SET(CMAKE_C_COMPILER "${NACLCC}")
SET(CMAKE_CXX_COMPILER "${NACLCXX}")
SET(CMAKE_LINKER "${NACLLD}")
SET(CMAKE_AR "${NACLAR}" CACHE FILEPATH "Archiver")
SET(_CMAKE_TOOLCHAIN_PREFIX ${NACL_CROSS_PREFIX})

SET(NACL_LIBC "${NACL_LIBC}")

# here is the target environment located
SET(CMAKE_FIND_ROOT_PATH ${NACL_TOOLCHAIN_ROOT})

# Reference CMAKE_TOOLCHAIN_FILE and CMAKE_PREFIX_PATH here to avoid
# cmake warnings:
# http://public.kitware.com/pipermail/cmake/2013-February/053476.html
SET(DUMMY_VALUE ${CMAKE_TOOLCHAIN_FILE} ${CMAKE_PREFIX_PATH})

# adjust the default behaviour of the FIND_XXX() commands:
# search headers and libraries in the target environment, search 
# programs in the host environment
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
SET(CMAKE_REQUIRED_INCLUDES "${NACL_SDK_ROOT}/include")
LINK_DIRECTORIES(${NACL_SDK_LIBDIR})

# nacl abi says 32bits little endian
SET(CMAKE_SIZEOF_VOID_P 4)
