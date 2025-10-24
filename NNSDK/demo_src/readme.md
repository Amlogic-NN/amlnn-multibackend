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

Place the script under `/opt/yocto-toolchain/32` or `/opt/yocto-toolchain/64`and run it to install.

### 3. Configure and Rebuild
⚠ Build Notes

Before rebuilding the demo, please **edit the `build.sh` script** and set the correct paths for your local environment:
<img width="700" height="150" alt="image" src="https://github.com/user-attachments/assets/ce7cd0a6-0c6b-47ab-8558-16ce9d459a2e" />


```bash
CMAKE_BIN="/opt/cmake-3.24.0-linux-x86_64/bin/cmake"
YOCTO_SDK_ROOT_32="/opt/yocto-toolchain/32"
YOCTO_SDK_ROOT_64="/opt/yocto-toolchain/64"

chmod +x build.sh
./build.sh classify   # Builds tf_delegate_classify_32/64
./build.sh detect     # Builds tf_delegate_detect_32/64
```
