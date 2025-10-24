/****************************************************************************
*
*    Copyright (c) 2019  by amlogic Corp.  All rights reserved.
*
*    The material in this file is confidential and contains trade secrets
*    of amlogic Corporation. No part of this work may be disclosed,
*    reproduced, copied, transmitted, or used in any way for any purpose,
*    without the express written permission of amlogic Corporation.
*
***************************************************************************/
/*-------------------------------------------
                Includes
-------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <pthread.h>
#include <float.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "nn_sdk.h"

///////////////////////////////////////////////////////////
#define BILLION 1000000000
static uint64_t tmsStart, tmsEnd, msVal_inference, usVal_inference;
static uint64_t get_perf_count()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)((uint64_t)ts.tv_nsec + (uint64_t)ts.tv_sec * BILLION);
}

static void process_top5_u8(uint8_t *buf, unsigned int num)
{
    int i = 0, j = 0, k = 0;
    unsigned int MaxClass[5] = {0};
    uint8_t fMaxProb[5] = {0, 0, 0, 0, 0};

    for (i = 0; i < num; i++)
    {
        for (j = 0; j < 5; j++)
        {
            if (buf[i] > fMaxProb[j])
            {
                for (k = 4; k > j; k--)
                {
                    fMaxProb[k] = fMaxProb[k - 1];
                    MaxClass[k] = MaxClass[k - 1];
                }
                fMaxProb[j] = buf[i];
                MaxClass[j] = i;
                break;
            }
        }
    }

    for (i=0; i<5; i++)
    {
        printf("top %d:score--%d,class--%d\n", i, fMaxProb[i], MaxClass[i]);
    }
}

static void process_top5_f32(float *buf, unsigned int num)
{
    int i = 0, j = 0, k = 0;
    unsigned int MaxClass[5] = {0};
    float fMaxProb[5] = {-FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX};
    // buf[0] = 17.0;

    for (i = 0; i < num; i++)
    {
        for (j = 0; j < 5; j++)
        {
            if (buf[i] > fMaxProb[j])
            {
                for (k = 4; k > j; k--)
                {
                    fMaxProb[k] = fMaxProb[k - 1];
                    MaxClass[k] = MaxClass[k - 1];
                }
                fMaxProb[j] = buf[i];
                MaxClass[j] = i;
                break;
            }
        }
    }

    for (i=0; i<5; i++)
    {
        // printf("%3d: %8.6f\n", MaxClass[i], fMaxProb[i]);
        printf("top %d:score--%f,class--%d\n", i, fMaxProb[i], MaxClass[i]);
    }
}


int main(int argc,char **argv)
{
    if (argc < 4)
    {
        printf("Usage:\n");
        printf("argv[0]: exe_file\n");
        printf("argv[1]: xxx.tflite\n");
        printf("argv[2]: xxx.jpg/jpeg/png\n");
        printf("argv[3] (optional): 0(cpu) | 1(gpu)\n");
        return -1;
    }

    int ret = 0;

    aml_hw_flag_t hw_flag = AML_HW_CPU;
    if (atoi(argv[3]) == 0) // 0: CPU, 1: GPU via CLI
    {
        hw_flag = AML_HW_CPU;
    }
    else if (atoi(argv[3]) == 1)
    {
        hw_flag = AML_HW_GPU;
    }


    void *qcontext = NULL;
    aml_config config;
    memset(&config, 0, sizeof(aml_config));
    config.path = argv[1];
    config.modelType = TENSORFLOWLITE;
    config.hw_flag = hw_flag;
    qcontext = aml_module_create(&config);
    if (NULL == qcontext)
    {
        printf("aml_module_create fail.\n");
        return -1;
    }

    printf("Use %s backend.\n", hw_flag == AML_HW_GPU ? "GPU" : "CPU");


    tensor_info *input_tensor;
    tensor_info *output_tensor;
    ret = aml_util_getTensorInfo(qcontext, NULL, &input_tensor, &output_tensor);
    if (ret)
    {
        printf("aml_util_getTensorInfo fail.\n");
        return -1;
    }

    printf("\ninput tensors: %d\n", input_tensor->num);
    printf("output tensors: %d\n", output_tensor->num);
    for (int i = 0; i < input_tensor->num; i++)
    {
        printf("input[%d], shape=[%d,%d,%d,%d], zp=%d, scale=%f, type=%d\n", 
                i, 
                input_tensor->info[i].sizes_of_dim[0], 
                input_tensor->info[i].sizes_of_dim[1], 
                input_tensor->info[i].sizes_of_dim[2], 
                input_tensor->info[i].sizes_of_dim[3], 
                input_tensor->info[i].TF_zeropoint, 
                input_tensor->info[i].TF_scale, 
                input_tensor->info[i].data_format);
    }
    for (int i = 0; i < output_tensor->num; i++)
    {
        printf("output[%d], shape=[%d,%d,%d,%d], zp=%d, scale=%f, type=%d\n", 
                i, 
                output_tensor->info[i].sizes_of_dim[0], 
                output_tensor->info[i].sizes_of_dim[1], 
                output_tensor->info[i].sizes_of_dim[2], 
                output_tensor->info[i].sizes_of_dim[3], 
                output_tensor->info[i].TF_zeropoint, 
                output_tensor->info[i].TF_scale, 
                output_tensor->info[i].data_format);
    }


    int iw, ih, n;
    int input_size;
    unsigned char *idata = stbi_load(argv[2], &iw, &ih, &n, 0);
    input_size = iw * ih * n;
    printf("iw = %d, ih = %d, n = %d, input_size = %d\n", iw, ih, n, input_size);
    float *idata_fp32 = (float *)malloc(input_size * sizeof(float));
    for (int i = 0; i < input_size; i++)
    {
        idata_fp32[i] = idata[i] / 127.5 - 1;
    }

    nn_input inData;
    memset(&inData, 0, sizeof(nn_input));
    inData.input_index = 0;
    inData.input = (unsigned char *)idata_fp32;
    inData.size = input_size * sizeof(float);
    inData.input_type = BINARY_RAW_DATA;
    inData.info.input_data_type = AML_INPUT_FP32;
    ret = aml_module_input_set(qcontext, &inData);
    if (ret)
    {
        printf("aml_module_input_set fail.\n");
        return -1;
    }
    stbi_image_free(idata);
    free(idata_fp32);


    aml_output_config_t outconfig;
    nn_output *outdata = NULL;
    memset(&outconfig,0,sizeof(aml_output_config_t));
    outconfig.format = AML_OUTDATA_RAW;
    outdata = (nn_output*)aml_module_output_get(qcontext, outconfig);
    if (NULL == outdata)
    {
        printf("aml_module_output_get fail.\n");
        return -1;
    }

    printf("outdata->out[0].size = %d\n", outdata->out[0].size);
    // process_top5_u8((uint8_t *)outdata->out[0].buf, outdata->out[0].size);
    process_top5_f32((float *)outdata->out[0].buf, outdata->out[0].size/sizeof(float));


    ret = aml_util_freeTensorInfo(input_tensor);
    if (ret)
    {
        printf("aml_util_freeTensorInfo fail.\n");
        return -1;
    }
    ret = aml_util_freeTensorInfo(output_tensor);
    if (ret)
    {
        printf("aml_util_freeTensorInfo fail.\n");
        return -1;
    }


    ret = aml_module_destroy(qcontext);
    if (ret)
    {
        printf("aml_module_destroy fail.\n");
        return -1;
    }

    return ret;
}
