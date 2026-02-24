cmake_minimum_required(VERSION 3.10)

# Toolchain must be set via -DCMAKE_TOOLCHAIN_FILE before project().
# ARCH_BITS and YOCTO_SDK_ROOT are passed by the toolchain file / build.sh.

if(NOT DEFINED ARCH_BITS)
    message(FATAL_ERROR "ARCH_BITS must be set to 32 or 64")
endif()

project(tf_delegate_classify C)

set(NNSDK_ROOT        "${CMAKE_CURRENT_SOURCE_DIR}/../../nnsdk_lib")
set(NNSDK_INCLUDE_DIR "${NNSDK_ROOT}/include")
set(THIRDPARTY_DIR    "${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty")

if(ARCH_BITS EQUAL 32)
    set(NNSDK_LIB_DIR "${NNSDK_ROOT}/lib/linux/lib32_yocto")
    set(_ARCH_FLAGS   -march=armv7-a -marm -mfpu=vfp -mfloat-abi=hard)
elseif(ARCH_BITS EQUAL 64)
    set(NNSDK_LIB_DIR "${NNSDK_ROOT}/lib/linux/lib64_yocto")
    set(_ARCH_FLAGS)
else()
    message(FATAL_ERROR "Unsupported ARCH_BITS: ${ARCH_BITS} (expected 32 or 64)")
endif()

set(NNSDK_LIB "${NNSDK_LIB_DIR}/libnnsdk.so")
if(NOT EXISTS "${NNSDK_LIB}")
    message(FATAL_ERROR "libnnsdk.so not found in ${NNSDK_LIB_DIR}")
endif()

add_executable(tf_delegate_classify main.c)

target_include_directories(tf_delegate_classify PRIVATE
    "${NNSDK_INCLUDE_DIR}"
    "${THIRDPARTY_DIR}/stb_image"
)
target_compile_options(tf_delegate_classify PRIVATE
    -Wno-error=format-security
    ${_ARCH_FLAGS}
)
target_link_options(tf_delegate_classify PRIVATE -Wl,--gc-sections)

set_target_properties(tf_delegate_classify PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

target_link_libraries(tf_delegate_classify PRIVATE
    "${NNSDK_LIB}"
    dl m pthread
)
