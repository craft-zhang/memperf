/*
 * This File Is Part Of DeepNet.
 *
 * DeepNet -- Deep ConvNet Forward Tool With Embedded Optimization.
 * Copyright (C) 2018, CloudWalk Technology Co., Ltd..
 * All Rights Reserved.
 *
 * Author: zhangdanfeng@cloudwalk.cn
 * Developed at CloudWalk (ShangHai China).
 */

#pragma once

#include <stddef.h>

// for kc in gotoBLAS
#define KERNEL_L1_LIMIT 256
// for mc in gotoBLAS
#define KERNEL_L2_LIMIT 256

// kernel_size = input_channel x kernel_h x kernel_w, col_num = output_h * output_w
size_t interleave_kernel_space_size(size_t kernel_chan, size_t kernel_size);

void interleave_kernel(float *kernel, float *kernel_interleaved, size_t kernel_chan, size_t kernel_size);

size_t interleave_col_space_size(size_t kernel_size, size_t col_num);

void interleave_col(float *col, float *col_interleaved, size_t kernel_size, size_t col_num);

void conv_sgemm(float *kernel, float *col, float *biases
            , size_t ouput_channel, size_t kernel_size, size_t col_num
            , bool bias_term, bool relu_fused, float *output);

void fc_batch_sgemm(float *input, float *weight, float *biases
            , size_t batch_size, size_t input_size, size_t output_size
            , bool bias_term, bool relu_fused, float *output);
