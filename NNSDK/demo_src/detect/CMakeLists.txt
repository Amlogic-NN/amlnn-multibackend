cmake_minimum_required(VERSION 3.10)

if(NOT DEFINED ARCH_BITS)
    message(FATAL_ERROR "ARCH_BITS must be set to 32 or 64")
endif()

if(NOT DEFINED YOCTO_SDK_ROOT)
    message(FATAL_ERROR "YOCTO_SDK_ROOT must point to the Yocto SDK root directory")
endif()

set(CMAKE_SYSTEM_NAME Linux)

set(NNSDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../nnsdk_lib")
set(NNSDK_INCLUDE_DIR "${NNSDK_ROOT}/include")
set(THIRDPARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty")

set(HOST_SYSROOT "${YOCTO_SDK_ROOT}/sysroots/x86_64-pokysdk-linux")

if(NOT IS_DIRECTORY "${HOST_SYSROOT}")
    message(FATAL_ERROR "Yocto host sysroot not found: ${HOST_SYSROOT}")
endif()

set(ARCH_EXTRA_FLAGS)

if(ARCH_BITS EQUAL 32)
    set(TARGET_SYSROOT "${YOCTO_SDK_ROOT}/sysroots/armv7at2hf-neon-poky-linux-gnueabi")
    set(CROSS_COMPILE_TRIPLE "arm-poky-linux-gnueabi")
    set(NNSDK_LIB_DIR "${NNSDK_ROOT}/lib/linux/lib32_yocto")
    set(ARCH_EXTRA_FLAGS -march=armv7-a -marm -mfpu=vfp -mfloat-abi=hard)
elseif(ARCH_BITS EQUAL 64)
    set(TARGET_SYSROOT "${YOCTO_SDK_ROOT}/sysroots/armv8a-poky-linux")
    set(CROSS_COMPILE_TRIPLE "aarch64-poky-linux")
    set(NNSDK_LIB_DIR "${NNSDK_ROOT}/lib/linux/lib64_yocto")
else()
    message(FATAL_ERROR "Unsupported ARCH_BITS value: ${ARCH_BITS} (expected 32 or 64)")
endif()

if(NOT IS_DIRECTORY "${TARGET_SYSROOT}")
    message(FATAL_ERROR "Yocto target sysroot not found: ${TARGET_SYSROOT}")
endif()

set(CMAKE_C_COMPILER "${HOST_SYSROOT}/usr/bin/${CROSS_COMPILE_TRIPLE}/${CROSS_COMPILE_TRIPLE}-gcc")
set(CMAKE_CXX_COMPILER "${HOST_SYSROOT}/usr/bin/${CROSS_COMPILE_TRIPLE}/${CROSS_COMPILE_TRIPLE}-g++")

set(CMAKE_SYSROOT "${TARGET_SYSROOT}")
string(REPLACE ";" " " ARCH_EXTRA_FLAGS_STR "${ARCH_EXTRA_FLAGS}")
set(CMAKE_C_FLAGS_INIT "--sysroot=${TARGET_SYSROOT} ${ARCH_EXTRA_FLAGS_STR}")
set(CMAKE_CXX_FLAGS_INIT "--sysroot=${TARGET_SYSROOT} ${ARCH_EXTRA_FLAGS_STR}")
set(CMAKE_EXE_LINKER_FLAGS_INIT "--sysroot=${TARGET_SYSROOT}")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "--sysroot=${TARGET_SYSROOT}")

project(tf_delegate_detect C CXX)

set(CMAKE_FIND_ROOT_PATH "${TARGET_SYSROOT}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(NNSDK_LIB "${NNSDK_LIB_DIR}/libnnsdk.so")
if(NOT EXISTS "${NNSDK_LIB}")
    message(FATAL_ERROR "libnnsdk.so not found in ${NNSDK_LIB_DIR}")
endif()

add_executable(tf_delegate_detect
    main.cpp
)

target_include_directories(tf_delegate_detect PRIVATE
    "${NNSDK_INCLUDE_DIR}"
    "${THIRDPARTY_DIR}/stb_image"
)
target_compile_options(tf_delegate_detect PRIVATE -Wno-error=format-security)
if(ARCH_EXTRA_FLAGS)
    target_compile_options(tf_delegate_detect PRIVATE ${ARCH_EXTRA_FLAGS})
endif()
target_link_options(tf_delegate_detect PRIVATE -Wl,--gc-sections)

set_target_properties(tf_delegate_detect PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

target_link_libraries(tf_delegate_detect
    PRIVATE
        "${NNSDK_LIB}"
        dl
        m
        pthread
)
