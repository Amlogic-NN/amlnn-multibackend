<p align="center">
  <img src="https://upload.wikimedia.org/wikipedia/commons/thumb/4/43/Amlogic_logo.svg/500px-Amlogic_logo.svg.png" alt="Amlogic Logo" width="200"/>
</p>

<h1 align="center">Amlogic NN SDK Demo</h1>

<p align="center">
  🧠 Image Classification & Object Detection on Amlogic Boards  
  <br>
  <a href="https://github.com/your-repo/amlogic-nn-demo"><img src="https://img.shields.io/badge/GitHub-Repository-blue?logo=github" alt="GitHub"></a>
  <a href="#"><img src="https://img.shields.io/badge/Build-Passing-brightgreen?logo=cmake" alt="Build"></a>
  <a href="#"><img src="https://img.shields.io/badge/License-MIT-yellow.svg" alt="License"></a>
  <a href="#"><img src="https://img.shields.io/badge/Platform-Yocto%20Linux-orange" alt="Platform"></a>
</p>

---

## 🚀 Quick Start (32-bit Board Example)

### 1. Copy Required Shared Libraries
```bash
adb push nnsdk_v2.8.5_2025_0801_merged/lib/linux/lib32_yocto/libAmlTFDelegate.so /usr/lib
adb push nnsdk_v2.8.5_2025_0801_merged/lib/linux/lib32_yocto/libnnsdk.so /usr/lib
```

### 2. Select a Demo
- **Classification**: `case/classify/`  
- **Detection**: `case/detect/` (will generate `*_det.jpg` after each run)

### 3. Push Demo Files to Board
```bash
adb push case/classify /tmp/classify
adb push case/detect /tmp/detect
```

### 4. Run Classification
```bash
adb shell
cd /tmp/classify
chmod +x tf_delegate_classify_32
./tf_delegate_classify_32 mobilenet_v2_float32.tflite fish_224x224.jpeg 0
```
- Prints **Top-5 predictions** with confidence scores  
- `0` means using **CPU backend**

### 5. Run Detection
```bash
adb shell
cd /tmp/detect
chmod +x tf_delegate_detect_32
./tf_delegate_detect_32 yolov8n_uint8.tflite zidane.jpg 0
```
- Detection results are printed  
- Annotated image saved as `zidane_det.jpg`  

> `0` = CPU, `1` = GPU (requires proper hardware support)

---

## 📂 Directory Layout

```
├── build.sh                      # Yocto rebuild script
├── case/
│   ├── classify/                 # Classification demo (executables, models, images)
│   └── detect/                   # Detection demo (executables, models, images)
├── example/                       # Source code & CMake configs
├── nnsdk_v2.8.5_2025_0801_merged/ # Amlogic NN SDK headers & libs
└── 3rdparty/                      # stb image utilities
```

---

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

Place the script under `/opt/yocto-toolchain` and run it to install.

### 3. Configure and Rebuild
```bash
YOCTO_SDK_ROOT_32="/opt/yocto-toolchain/32"
YOCTO_SDK_ROOT_64="/opt/yocto-toolchain/64"

./build.sh classify   # Builds tf_delegate_classify_32/64
./build.sh detect     # Builds tf_delegate_detect_32/64
```

---

## 📝 Notes

- GPU mode requires proper hardware support and drivers.
- The detection demo saves annotated images as `.jpg` next to the input file.
- You can test other models or images by placing them in the corresponding `case/` subdirectory and adjusting the command parameters.
- This project is intended for rapid validation of the **Amlogic NN Delegate** on embedded boards.

---

## 📜 License
This project is provided for evaluation and development purposes with Amlogic NN SDK.

---

<p align="center">
  Made with ❤️ for Embedded AI Development
</p>

