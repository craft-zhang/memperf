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
//
// 4*4 single precise floating point matric multiplication
//
//    --              --      --               --     --                --         --                 --
//    | k0 - - - - - - |      |  i0  i1  i2  i3 |     |  b0 b0 b0 b0 |         | k0i0 k0i1 .. k0i3 |
//    |                |      |  .   .   .   .  |     |              |         |                   |
//    | k1 - - - - - - |      |  .   .   .   .  |     |  b1 b1 b1 b1 |         | k1i0 k1i1 .. k1i3 |
//    |                |  x   |  .   .   .   .  |  +  |              |     =   |                   |
//    | k2 - - - - - - |      |  .   .   .   .  |     |  b2 b2 b2 b2 |         | k2i0 k2i1 .. k2i3 |
//    |                |      |  .   .   .   .  |     |              |         |                   |
//    | k3 - - - - - - |      |  .   .   .   .  |     |  b3 b3 b3 b3 |         | k3i0 k3i1 .. k3i3 |
//    --              --      --               --     --                --         --                 --
//      kernel 4 x k             input k x 4             biases 4 x 4                 output 4 x 4
//
// interface input:
//         r0 arg1  k
//         r1 arg2  kernel start address
//         r2 arg3  input  start address
//         r3 arg4  output save  address
//         r4 arg5  have biases flag
//         r5 arg6  biases start address
//
// output: no
//
// register definition
// q0-3 4S kernel data  {k3   k2   k1   k0}
// q4-7 4S input  data  {i3   i2   i1   i0}
// q8  dot product for {i3k0, i2k0, i1k0, i0k0}
// q9  dot product for {i3k1, i2k1, i1k1, i0k1}
// q10 dot product for {i3k2, i2k2, i1k2, i0k2}
// q11 dot product for {i3k3, i2k3, i1k3, i0k3}
// q12 dot product for {i3k0, i2k0, i1k0, i0k0}
// q13 dot product for {i3k1, i2k1, i1k1, i0k1}
// q14 dot product for {i3k2, i2k2, i1k2, i0k2}
// q15 dot product for {i3k3, i2k3, i1k3, i0k3}

#if defined(__arm__)

#ifndef REALNAME
#if defined(__APPLE__)
#define REALNAME _sgemm_kernel_4x4_neon
#elif defined(__linux__)
#define REALNAME sgemm_kernel_4x4_neon
#else
#error compiler not support!
#endif
#endif

#define     K           r0
#define     A           r1
#define     B           r2
#define     C           r3
#define     BIAS_TERM   r4
#define     BIAS        r5
#define     L           r6

/**************************************************************************************
* Macro definitions
**************************************************************************************/


#include "sgemm_macros.S"


.macro INIT_4x4

    vsub.f32    q12, q12, q12           // Exec Latency 4 Execution Throughput 1 Utilized Pipelines F0/F1
    vsub.f32    q13, q13, q13
    vsub.f32    q14, q14, q14
    vsub.f32    q15, q15, q15

.endm


.macro KERNEL_4x4_INIT_1

    vld1.32     {q0, q1}, [A:128]!          // Exec Latency 5 Execution Throughput 1 Utilized Pipelines L
    vld1.32     {q4, q5}, [B:128]!

    vld1.32     {q2, q3}, [A:128]!
    vmul.f32    q8 , q4, d0[0]
    vld1.32     {q6, q7}, [B:128]!
    vmul.f32    q9 , q4, d0[1]
    vmul.f32    q10, q4, d1[0]
    vmul.f32    q11, q4, d1[1]

    vld1.32     {q0}, [A:128]!
    vmla.f32    q12, q5, d2[0]
    vld1.32     {q4}, [B:128]!
    vmla.f32    q13, q5, d2[1]
    vmla.f32    q14, q5, d3[0]
    vmla.f32    q15, q5, d3[1]

.endm


.macro KERNEL_4x4_LOOP_1

    vld1.32     {q3}, [A:128]!
    vmla.f32    q8 , q4, d0[0]
    vld1.32     {q7}, [B:128]!
    vmla.f32    q9 , q4, d0[1]
    vmla.f32    q10, q4, d1[0]
    vmla.f32    q11, q4, d1[1]

    vld1.32     {q0}, [A:128]!
    vmla.f32    q12, q5, d2[0]
    vld1.32     {q4}, [B:128]!
    vmla.f32    q13, q5, d2[1]
    vmla.f32    q14, q5, d3[0]
    vmla.f32    q15, q5, d3[1]

.endm


.macro KERNEL_4x4_LOOP_2

    vld1.32     {q1}, [A:128]!
    vmla.f32    q8 , q6, d4[0]
    vld1.32     {q5}, [B:128]!
    vmla.f32    q9 , q6, d4[1]
    vmla.f32    q10, q6, d5[0]
    vmla.f32    q11, q6, d5[1]

    vld1.32     {q2}, [A:128]!
    vmla.f32    q12, q7, d6[0]
    vld1.32     {q6}, [B:128]!
    vmla.f32    q13, q7, d6[1]
    vmla.f32    q14, q7, d7[0]
    vmla.f32    q15, q7, d7[1]

.endm


.macro KERNEL_4x4_END_1

    vld1.32     {q3}, [A:128]!
    vmla.f32    q8 , q4, d0[0]
    vld1.32     {q7}, [B:128]!
    vmla.f32    q9 , q4, d0[1]
    vmla.f32    q10, q4, d1[0]
    vmla.f32    q11, q4, d1[1]

    vmla.f32    q12, q5, d2[0]
    vmla.f32    q13, q5, d2[1]
    vmla.f32    q14, q5, d3[0]
    vmla.f32    q15, q5, d3[1]

.endm


.macro KERNEL_4x4_END_2

    vmla.f32    q8 , q6, d4[0]
    vmla.f32    q9 , q6, d4[1]
    vmla.f32    q10, q6, d5[0]
    vmla.f32    q11, q6, d5[1]

    vmla.f32    q12, q7, d6[0]
    vmla.f32    q13, q7, d6[1]
    vmla.f32    q14, q7, d7[0]
    vmla.f32    q15, q7, d7[1]

    vadd.f32    q12, q12, q8
    vadd.f32    q13, q13, q9
    vadd.f32    q14, q14, q10
    vadd.f32    q15, q15, q11

.endm


.macro KERNEL_4x4_SUB

    vld1.32     {q0}, [A:128]!
    vld1.32     {q4}, [B:128]!
    vmla.f32    q12, q4, d0[0]
    vmla.f32    q13, q4, d0[1]
    vmla.f32    q14, q4, d1[0]
    vmla.f32    q15, q4, d1[1]

.endm


.macro SAVE_4x4

    vst1.32     {q12}, [C]!
    vst1.32     {q13}, [C]!
    vst1.32     {q14}, [C]!
    vst1.32     {q15}, [C]

.endm


/**************************************************************************************
* End of macro definitions
**************************************************************************************/
    .text
    // .align 8
//                                   r0        r1        r2        r3      r4                r5
// void sgemm_kernel_4x4_neon(size_t k, float *a, float *b, float *c, bool bias_term, float *bias);
SUBROUTIN

    PROLOGUE

    ldr     BIAS_TERM, [sp, #96]
    ldr     BIAS, [sp, #100]

    // initial
    cmp     BIAS_TERM, #0
    beq     non_biases
#ifdef FOR_FC
    vld1.32 {q12}, [BIAS:128]
    vld1.32 {q13}, [BIAS:128]
    vld1.32 {q14}, [BIAS:128]
    vld1.32 {q15}, [BIAS:128]
#else
    vld1.32 {d24[], d25[]}, [BIAS:32]!
    vld1.32 {d26[], d27[]}, [BIAS:32]!
    vld1.32 {d28[], d29[]}, [BIAS:32]!
    vld1.32 {d30[], d31[]}, [BIAS:32]!
#endif
    b       sgemm_kernel_4x4_start_8

non_biases:

    INIT_4x4

sgemm_kernel_4x4_start_8:

    asrs    L , K, #3                   // L = K / 8
    cmp     L , #2
    blt     sgemm_kernel_4x4_only_8     // L < 2 

    KERNEL_4x4_INIT_1
    KERNEL_4x4_LOOP_2
    KERNEL_4x4_LOOP_1
    KERNEL_4x4_LOOP_2

    subs    L, L, #2
    ble     sgemm_kernel_4x4_end_8      // (L -= 2) == 0

sgemm_kernel_4x4_mid_8:

    KERNEL_4x4_LOOP_1
    KERNEL_4x4_LOOP_2
    KERNEL_4x4_LOOP_1
    KERNEL_4x4_LOOP_2

    subs    L, L, #1
    bgt     sgemm_kernel_4x4_mid_8      // (L -= 1) > 0

sgemm_kernel_4x4_end_8:

    KERNEL_4x4_LOOP_1
    KERNEL_4x4_LOOP_2
    KERNEL_4x4_END_1
    KERNEL_4x4_END_2

    b       sgemm_kernel_4x4_L_0

sgemm_kernel_4x4_only_8:

    cmp     L, #1
//    blt sgemm_kernel_4x4_init         // L < 1
    blt     sgemm_kernel_4x4_L_0

    KERNEL_4x4_INIT_1
    KERNEL_4x4_LOOP_2
    KERNEL_4x4_END_1
    KERNEL_4x4_END_2

//    b       sgemm_kernel_4x4_L_0

//sgemm_kernel_4x4_init:

//    INIT_4x4

sgemm_kernel_4x4_L_0:

    ands    L , K, #7                   // K ands 1 1 1 , L = K % 8 
    ble     sgemm_kernel_4x4_writeback  // L = 0

sgemm_kernel_4x4_only_1:

    KERNEL_4x4_SUB

    subs    L, L, #1
    bne     sgemm_kernel_4x4_only_1     // L > 0

sgemm_kernel_4x4_writeback:

    SAVE_4x4

sgemm_kernel_4x4_quit:

    EPILOGUE

#endif
