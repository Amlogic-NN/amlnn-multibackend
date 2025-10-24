# Amlogic AI Software Development Kit

## Intro

Amlogic supports multiple deployment schemes for AI models. If your Amlogic development board includes NPU, you can directly use NNSDK to develop your application. If your development board does not support NPU, you can also use CPU and GPU for deployment. For the use of NPU, please visit the official website of Amlogic (https://www.amlogic.cn/) and contact relevant personnel. Below we will introduce the deployment process using CPU and GPU.

We currently support two ways to deploy AI models on the Amlogic development board. The first is  NNSDK path, which is a tflite based deployment solution developed by Amlogic. The second is  MNN path , an open-source framework developed by Alibaba

### NNSDK path

​	                                                                     ![image-20251024093650200](C:\Users\cancan.chang\AppData\Roaming\Typora\typora-user-images\image-20251024093650200.png)     



### MNN path



​	![image-20251024093757328](C:\Users\cancan.chang\AppData\Roaming\Typora\typora-user-images\image-20251024093757328.png)



# Quick Start

1. If you use nnsdk for development

​	Please refer to the NNSDK/README.md



2. if you use MNN for development

​	Please refer to the MNN/README.md



# Acknowledgement

amlnn-multibackend  refs to the following projects:

https://github.com/alibaba/MNN



# License

Apache 2.0