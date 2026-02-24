## 🧰 Environment Setup & Rebuild (Optional)

For detailed rebuid from source, please refer to the following file:  

📄 [NNSDK/demo_src/readme.md](./demo_src/readme.md)

## 🚀 Quick Start (32-bit Board Example)

### 1. Push so to Board

```bash
adb push nnsdk_lib/lib/linux/lib32_yocto/libAmlTFDelegate.so /usr/lib
adb push nnsdk_lib/lib/linux/lib32_yocto/libnnsdk.so /usr/lib
```

### 2. Select a Demo

- **Classification**: `demo/classify/`  
- **Detection**: `demo/detect/` (will generate `*_det.jpg` after each run)

### 3. Push Demo Files to Board
```bash
adb push demo/classify /tmp/classify
adb push demo/detect /tmp/detect
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
./tf_delegate_detect_32 yolov8m_416-416_1out_int8.tflite zidane.jpg 0
```
- Detection results are printed  
- Annotated image saved as `zidane_det.jpg`  

> `0` = CPU, `1` = GPU (requires proper hardware support)

<img width="797" height="450" alt="image" src="https://github.com/user-attachments/assets/4955ac3c-c16a-4360-8aae-f57f374e3841" />

---

## 📂 Directory Layout

```
├── build.sh                      # Yocto rebuild script
├── demo/
│   ├── classify/                 # Classification demo (executables, models, images)
│   └── detect/                   # Detection demo (executables, models, images)
├── demo_src/                       # Source code & CMake configs
├── nnsdk_lib/ # Amlogic NN SDK headers & libs
└── 3rdparty/                      # stb image utilities
```

## 📝 Notes

- GPU mode requires proper hardware support and drivers.
- The detection demo saves annotated images as `.jpg` next to the input file.
- You can test other models or images by placing them in the corresponding `demo/` subdirectory and adjusting the command parameters.
- This project is intended for rapid validation of the **Amlogic NN Delegate** on embedded boards.

## 📘 API Introduction

For detailed API usage and interface documentation, please refer to the following file:

📄 [NNSDK/Docs/API-Introduction-EN.pdf](./Docs/API-Introduction-EN.pdf)

This document contains:
- Function definitions and parameter explanations
- Interface usage examples
- Integration guidelines for developers






