## 🧰 Environment Setup & Rebuild (Optional)

### 1. Install CMake
- Download from [CMake Releases](https://github.com/kitware/cmake/releases)  
- Example:
```bash
/opt/cmake-3.24.0-linux-x86_64/bin/cmake
```

### 2. Install Yocto Toolchains
- 32-bit: 
  `poky-glibc-x86_64-amlogic-bsp-armv7at2hf-neon-mesons7-bh201-5.15-a32-toolchain-4.0.20.sh`  
- 64-bit: 
  `poky-glibc-x86_64-meta-toolchain-armv8a-mesont7-an400-5.15-a64-toolchain-4.0.20.sh`

Run the installer and note the installation path (e.g. `/opt/yocto-toolchain/`).

### 3. Configure and Rebuild

```bash
chmod +x build.sh

# Build 64-bit (default)
./build.sh detect
./build.sh classify

# Build 32-bit
./build.sh detect 32
./build.sh classify 32
```

Override the SDK path or Toolchain file via environment variables if needed:

```bash
# Override SDK root
YOCTO_SDK_ROOT=/opt/yocto-toolchain ./build.sh detect

# Use a custom CMake toolchain file
TOOLCHAIN_FILE=/path/to/your/toolchain.cmake ./build.sh detect

# Specify CMake binary and SDK root
YOCTO_SDK_ROOT=/opt/yocto-toolchain CMAKE_BIN=/opt/cmake/bin/cmake ./build.sh detect
```

Output binaries are placed in `../demo/<target>/`:

| Binary | Description |
|---|---|
| `tf_delegate_detect_64` | Object detection, 64-bit |
| `tf_delegate_detect_32` | Object detection, 32-bit |
| `tf_delegate_classify_64` | Classification, 64-bit |
| `tf_delegate_classify_32` | Classification, 32-bit |
