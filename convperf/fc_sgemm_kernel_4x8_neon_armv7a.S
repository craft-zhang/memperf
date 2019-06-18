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

#define FOR_FC 1

#if defined(__APPLE__)
#define REALNAME _fc_sgemm_kernel_4x8_neon
#elif defined(__linux__)
#define REALNAME fc_sgemm_kernel_4x8_neon
#else
#error compiler not support!
#endif

#include "./sgemm_kernel_4x8_neon_armv7a.S"
