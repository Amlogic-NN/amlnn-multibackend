<p align="center">
  <img src="https://upload.wikimedia.org/wikipedia/commons/thumb/4/43/Amlogic_logo.svg/500px-Amlogic_logo.svg.png" alt="Amlogic Logo" width="200"/>
</p>

<h1 align="center">Amlogic AI Software Development Kit</h1>

<p align="center">
  🧠 Image Classification & Object Detection on Amlogic Boards  
  <br>
  <a href="https://github.com/your-repo/amlogic-nn-demo"><img src="https://img.shields.io/badge/GitHub-Repository-blue?logo=github" alt="GitHub"></a>
  <a href="#"><img src="https://img.shields.io/badge/Build-Passing-brightgreen?logo=cmake" alt="Build"></a>
  <a href="#"><img src="https://img.shields.io/badge/License-MIT-yellow.svg" alt="License"></a>
  <a href="#"><img src="https://img.shields.io/badge/Platform-Yocto%20Linux-orange" alt="Platform"></a>
</p>

---


## Intro

Amlogic supports multiple deployment schemes for AI models. If your Amlogic development board includes NPU, you can directly use NNSDK to develop your application. If your development board does not support NPU, you can also use CPU and GPU for deployment. For the use of NPU, please visit the official website of Amlogic (https://www.amlogic.cn/) and contact relevant personnel. Below we will introduce the deployment process using CPU and GPU.

We currently support two ways to deploy AI models on the Amlogic development board. The first is  NNSDK path, which is a tflite based deployment solution developed by Amlogic. The second is  MNN path , an open-source framework developed by Alibaba

### NNSDK path

​	                                                                       

<p align="center">
<img width="277" height="487" alt="image-20251024093650200" src="https://github.com/user-attachments/assets/2e01e9f5-9ff6-4ec1-8b78-533aba7a7c02" />
</p>

### MNN path

<p align="center">
<img width="839" height="463" alt="image-20251024093757328" src="https://github.com/user-attachments/assets/6fdf8ec0-f610-4e15-84b9-2ae6b09300c3" />
</p>

​	



## Quick Start

1. If you use nnsdk for development

    Please refer to the [NNSDK/README.md](https://github.com/Amlogic-NN/amlnn-multibackend/blob/main/NNSDK/README.md)

2. if you use MNN for development

    Please refer to the [MNN/README.md](https://github.com/Amlogic-NN/amlnn-multibackend/blob/main/MNN/README.md)



## Acknowledgement

amlnn-multibackend  refs to the following projects:

https://github.com/alibaba/MNN



## 📜 License


Apache 2.0
This project is provided for evaluation and development purposes with Amlogic NN SDK.


---

<p align="center">
  Made with ❤️ for Embedded AI Development
</p>



