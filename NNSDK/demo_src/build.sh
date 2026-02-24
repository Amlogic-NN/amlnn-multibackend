#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 || $# -gt 2 ]]; then
  echo "Usage: $0 classify|detect [32|64]" >&2
  exit 1
fi

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${ROOT_DIR}/.." && pwd)"
CASE_DIR="${REPO_ROOT}/demo"
TARGET="${1}"
ARCH_BITS="${2:-64}"

case "${TARGET}" in
  classify)
    PROJECT_DIR="${ROOT_DIR}/classify"
    YOCTO_BIN_NAME="tf_delegate_classify"
    ;;
  detect)
    PROJECT_DIR="${ROOT_DIR}/detect"
    YOCTO_BIN_NAME="tf_delegate_detect"
    ;;
  *)
    echo "Unsupported target \"${TARGET}\". Usage: $0 classify|detect [32|64]" >&2
    exit 1
    ;;
esac

if [[ "${ARCH_BITS}" != "32" && "${ARCH_BITS}" != "64" ]]; then
  echo "Unsupported ARCH_BITS \"${ARCH_BITS}\". Must be 32 or 64." >&2
  exit 1
fi

# ---------------------------------------------------------------------------
# Configurable via environment variables
# ---------------------------------------------------------------------------
CMAKE_BIN="${CMAKE_BIN:-cmake}"
YOCTO_SDK_ROOT="${YOCTO_SDK_ROOT:-/data/yuandian/tools/poky/4.0.20}"

# Default to the built-in Yocto toolchain; override with any CMake toolchain file
TOOLCHAIN_FILE="${TOOLCHAIN_FILE:-${ROOT_DIR}/cmake/yocto-toolchain.cmake}"

# Export variables for CMake (important for try_compile and toolchain fallbacks)
export YOCTO_SDK_ROOT
export ARCH_BITS

# ---------------------------------------------------------------------------

CASE_SUBDIR="${CASE_DIR}/${TARGET}"
BUILD_DIR="${ROOT_DIR}/build/yocto_${TARGET}/${ARCH_BITS}"

if [[ ! -d "${PROJECT_DIR}" ]]; then
  echo "error: project directory not found: ${PROJECT_DIR}" >&2
  exit 1
fi
if [[ ! -d "${CASE_DIR}" ]]; then
  echo "error: install base directory not found: ${CASE_DIR}" >&2
  exit 1
fi

echo "==> Building Yocto ${ARCH_BITS}-bit"
echo "    toolchain : ${TOOLCHAIN_FILE}"
echo "    SDK root  : ${YOCTO_SDK_ROOT}"

mkdir -p "${CASE_SUBDIR}"
rm -rf "${BUILD_DIR}"

"${CMAKE_BIN}" \
  -S "${PROJECT_DIR}" \
  -B "${BUILD_DIR}" \
  -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN_FILE}" \
  -DYOCTO_SDK_ROOT="${YOCTO_SDK_ROOT}" \
  -DARCH_BITS="${ARCH_BITS}" \
  -DCMAKE_BUILD_TYPE=Release

"${CMAKE_BIN}" --build "${BUILD_DIR}" --config Release --target "${YOCTO_BIN_NAME}"

SRC="${BUILD_DIR}/bin/${YOCTO_BIN_NAME}"
DEST="${CASE_SUBDIR}/${YOCTO_BIN_NAME}_${ARCH_BITS}"

if [[ ! -f "${SRC}" ]]; then
  echo "error: build artifact not found: ${SRC}" >&2
  exit 1
fi

cp "${SRC}" "${DEST}"
chmod +x "${DEST}"

# Strip (best-effort)
HOST_SYSROOT="${YOCTO_SDK_ROOT}/sysroots/x86_64-pokysdk-linux"
if [[ "${ARCH_BITS}" == "32" ]]; then
  CROSS_TRIPLE="arm-poky-linux-gnueabi"
else
  CROSS_TRIPLE="aarch64-poky-linux"
fi
STRIP_TOOL="${HOST_SYSROOT}/usr/bin/${CROSS_TRIPLE}/${CROSS_TRIPLE}-strip"
if [[ -x "${STRIP_TOOL}" ]]; then
  "${STRIP_TOOL}" --strip-unneeded "${DEST}"
else
  echo "warning: strip tool not found; keeping debug info." >&2
fi

echo "Done: ${DEST}"
