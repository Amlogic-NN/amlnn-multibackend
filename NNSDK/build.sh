#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
  echo "Usage: $0 classify|detect" >&2
  exit 1
fi

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CASE_DIR="${ROOT_DIR}/case"
TARGET="${1}"

case "${TARGET}" in
  classify)
    PROJECT_DIR="${ROOT_DIR}/example/classify"
    CASE_SUBDIR="${CASE_DIR}/classify"
    YOCTO_BIN_NAME="tf_delegate_classify"
    ;;
  detect)
    PROJECT_DIR="${ROOT_DIR}/example/detect"
    CASE_SUBDIR="${CASE_DIR}/detect"
    YOCTO_BIN_NAME="tf_delegate_detect"
    ;;
  *)
    echo "Unsupported target \"${TARGET}\". Usage: $0 classify|detect" >&2
    exit 1
    ;;
esac

YOCTO_BUILD_ROOT="${ROOT_DIR}/build/yocto_${TARGET}"

CMAKE_BIN="${CMAKE_BIN:-/mnt/fileroot/xinxin.he/environment/cmake/cmake-3.24.0-linux-x86_64/bin/cmake}"

YOCTO_SDK_ROOT_32="${YOCTO_SDK_ROOT_32:-/mnt/fileroot/xinxin.he/environment/new-yocto/32}"
YOCTO_SDK_ROOT_64="${YOCTO_SDK_ROOT_64:-/mnt/fileroot/xinxin.he/environment/new-yocto/64}"
YOCTO_ARCH_BITS=("32" "64")

ensure_cmake() {
  if ! command -v "${CMAKE_BIN}" >/dev/null 2>&1; then
    echo "error: unable to find cmake, set CMAKE_BIN to a valid cmake binary." >&2
    exit 1
  fi
}

build_yocto() {
  mkdir -p "${CASE_SUBDIR}"
  rm -rf "${YOCTO_BUILD_ROOT}"

  for bits in "${YOCTO_ARCH_BITS[@]}"; do
    local sdk_root cross_triple suffix
    if [[ "${bits}" == "32" ]]; then
      sdk_root="${YOCTO_SDK_ROOT_32}"
      cross_triple="arm-poky-linux-gnueabi"
      suffix="32"
    elif [[ "${bits}" == "64" ]]; then
      sdk_root="${YOCTO_SDK_ROOT_64}"
      cross_triple="aarch64-poky-linux"
      suffix="64"
    else
      echo "error: unsupported ARCH_BITS '${bits}'." >&2
      exit 1
    fi

    if [[ ! -d "${sdk_root}" ]]; then
      echo "error: Yocto SDK root not found: ${sdk_root}" >&2
      exit 1
    fi

    echo "==> Building Yocto ${bits}-bit (SDK: ${sdk_root})"
    local build_dir="${YOCTO_BUILD_ROOT}/${bits}"

    "${CMAKE_BIN}" \
      -S "${PROJECT_DIR}" \
      -B "${build_dir}" \
      -DARCH_BITS="${bits}" \
      -DYOCTO_SDK_ROOT="${sdk_root}" \
      -DCMAKE_BUILD_TYPE=Release

    "${CMAKE_BIN}" --build "${build_dir}" --config Release --target "${YOCTO_BIN_NAME}"

    local src="${build_dir}/bin/${YOCTO_BIN_NAME}"
    local dest="${CASE_SUBDIR}/${YOCTO_BIN_NAME}_${suffix}"

    if [[ ! -f "${src}" ]]; then
      echo "error: build artifact not found: ${src}" >&2
      exit 1
    fi

    cp "${src}" "${dest}"
    chmod +x "${dest}"

    local strip_tool="${sdk_root}/sysroots/x86_64-pokysdk-linux/usr/bin/${cross_triple}/${cross_triple}-strip"
    if [[ -x "${strip_tool}" ]]; then
      "${strip_tool}" --strip-unneeded "${dest}"
    else
      echo "warning: strip tool not found (${strip_tool}); keeping debug info in output." >&2
    fi

    echo "Installed ${dest}"
  done

  echo "Yocto build completed."
}

ensure_cmake
build_yocto
