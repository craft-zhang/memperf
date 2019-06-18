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
// 4*8 single precise floating point matric multiplication
//
//    --              --      --               --     --            --         --                 --
//    | k0 - - - - - - |      |  i0  i1  ..  i8 |     |  b0 b0 .. b0 |         | k0i0 k0i1 .. k0i8 |
//    |                |      |  .   .   ..  .  |     |              |         |                   |
//    | k1 - - - - - - |      |  .   .   ..  .  |     |  b1 b1 .. b1 |         | k1i0 k1i1 .. k1i8 |
//    |                |  x   |  .   .   ..  .  |  +  |              |     =   |                   |
//    | k2 - - - - - - |      |  .   .   ..  .  |     |  b2 b2 .. b2 |         | k2i0 k2i1 .. k2i8 |
//    |                |      |  .   .   ..  .  |     |              |         |                   |
//    | k3 - - - - - - |      |  .   .   ..  .  |     |  b3 b3 .. b3 |         | k3i0 k3i1 .. k3i8 |
//    --              --      --               --     --            --         --                 --
//      kernel 4 x k             input k x 8             biases 4 x 8                 output 4 x 8
//
// interface input:
//         r0 arg1  k      for   depth
//         r1 arg1  update flag  for    rank-one update
//         r2 arg2  kernel start address
//         r3 arg3  input  start address
//         r4 arg4  output save  address / have   biases flag
//         r5 arg4  output stride        / biases start  address
//
// output: no
//
// register definition
// q0-7 4S kernel and input data
// q8  dot product for {i3k0, i2k0, i1k0, i0k0}
// q9  dot product for {i7k0, i6k0, i5k0, i4k0}
// q10 dot product for {i3k1, i2k1, i1k1, i0k1}
// q11 dot product for {i7k1, i6k1, i5k1, i4k1}
// q12 dot product for {i3k2, i2k2, i1k2, i0k2}
// q13 dot product for {i7k2, i6k2, i5k2, i4k2}
// q14 dot product for {i3k3, i2k3, i1k3, i0k3}
// q15 dot product for {i7k3, i6k3, i5k3, i4k3}

#if defined(__arm__)

#ifndef REALNAME
#if defined(__APPLE__)
#define REALNAME _sgemm_kernel_4x8_neon
#elif defined(__linux__)
#define REALNAME sgemm_kernel_4x8_neon
#else
#error compiler not support!
#endif
#endif

#define     K           r0
#define     UPDATE      r1
#define     A           r2
#define     B           r3
#define     BIAS_TERM   r4
#define     BIAS        r5
#define     C1          r4
#define     RSC         r5
#define     C2          r6
#if defined(__ANDROID__)
#define     C3          r7
#define     C4          r8
#define     L           r10
#else // defined(__APPLE__)
#define     C3          r8
#define     C4          r10
#define     L           r11
#endif

/**************************************************************************************
* Macro definitions
**************************************************************************************/


#include "sgemm_macros.S"


.macro INIT_4x8

    vsub.f32    q8 , q8 , q8            // Exec Latency 4 Execution Throughput 1 Utilized Pipelines F0/F1
    vsub.f32    q9 , q9 , q9
    vsub.f32    q10, q10, q10
    vsub.f32    q11, q11, q11

    vsub.f32    q12, q12, q12
    vsub.f32    q13, q13, q13
    vsub.f32    q14, q14, q14
    vsub.f32    q15, q15, q15

.endm


.macro KERNEL_4x8_INIT_1

    vld1.32     {q0}, [A:128]!          // Exec Latency 5 Execution Throughput 1 Utilized Pipelines L
    vld1.32     {q1, q2}, [B:128]!

    vld1.32     {q3}, [A:128]!
    vld1.32     {q4, q5}, [B:128]!

    vmla.f32    q8 , q1, d0[0]
    vld1.32     {q6, q7}, [A:128]!
    vmla.f32    q10, q1, d0[1]
    vmla.f32    q12, q1, d1[0]
    vmla.f32    q14, q1, d1[1]

    vld1.32     {q1}, [B:128]!
    vmla.f32    q9 , q2, d0[0]
    vmla.f32    q11, q2, d0[1]
    vmla.f32    q13, q2, d1[0]
    vmla.f32    q15, q2, d1[1]

.endm


.macro KERNEL_4x8_LOOP_1

    vld1.32     {q5}, [B:128]!
    vmla.f32    q8 , q1, d0[0]
    vld1.32     {q7}, [A:128]!
    vmla.f32    q10, q1, d0[1]
    vmla.f32    q12, q1, d1[0]
    vmla.f32    q14, q1, d1[1]

    vld1.32     {q1}, [B:128]!
    vmla.f32    q9 , q2, d0[0]
    vmla.f32    q11, q2, d0[1]
    vmla.f32    q13, q2, d1[0]
    vmla.f32    q15, q2, d1[1]

.endm


.macro KERNEL_4x8_LOOP_2

    vld1.32     {q2}, [B:128]!
    vmla.f32    q8 , q4, d6[0]
    vld1.32     {q0}, [A:128]!
    vmla.f32    q10, q4, d6[1]
    vmla.f32    q12, q4, d7[0]
    vmla.f32    q14, q4, d7[1]

    vld1.32     {q4}, [B:128]!
    vmla.f32    q9 , q5, d6[0]
    vmla.f32    q11, q5, d6[1]
    vmla.f32    q13, q5, d7[0]
    vmla.f32    q15, q5, d7[1]

.endm


.macro KERNEL_4x8_LOOP_3

    vld1.32     {q5}, [B:128]!
    vmla.f32    q8 , q1, d12[0]
    vld1.32     {q3}, [A:128]!
    vmla.f32    q10, q1, d12[1]
    vmla.f32    q12, q1, d13[0]
    vmla.f32    q14, q1, d13[1]

    vld1.32     {q1}, [B:128]!
    vmla.f32    q9 , q2, d12[0]
    vmla.f32    q11, q2, d12[1]
    vmla.f32    q13, q2, d13[0]
    vmla.f32    q15, q2, d13[1]

.endm


.macro KERNEL_4x8_LOOP_4

    vld1.32     {q2}, [B:128]!
    vmla.f32    q8 , q4, d14[0]
    vld1.32     {q6}, [A:128]!
    vmla.f32    q10, q4, d14[1]
    vmla.f32    q12, q4, d15[0]
    vmla.f32    q14, q4, d15[1]

    vld1.32     {q4}, [B:128]!
    vmla.f32    q9 , q5, d14[0]
    vmla.f32    q11, q5, d14[1]
    vmla.f32    q13, q5, d15[0]
    vmla.f32    q15, q5, d15[1]

.endm


.macro KERNEL_4x8_END_2

    vld1.32     {q2}, [B:128]!
    vmla.f32    q8 , q4, d6[0]
    vmla.f32    q10, q4, d6[1]
    vmla.f32    q12, q4, d7[0]
    vmla.f32    q14, q4, d7[1]

    vld1.32     {q4}, [B:128]!
    vmla.f32    q9 , q5, d6[0]
    vmla.f32    q11, q5, d6[1]
    vmla.f32    q13, q5, d7[0]
    vmla.f32    q15, q5, d7[1]

.endm


.macro KERNEL_4x8_END_3

    vld1.32     {q5}, [B:128]!
    vmla.f32    q8 , q1, d12[0]
    vmla.f32    q10, q1, d12[1]
    vmla.f32    q12, q1, d13[0]
    vmla.f32    q14, q1, d13[1]

    vmla.f32    q9 , q2, d12[0]
    vmla.f32    q11, q2, d12[1]
    vmla.f32    q13, q2, d13[0]
    vmla.f32    q15, q2, d13[1]

.endm


.macro KERNEL_4x8_END_4

    vmla.f32    q8 , q4, d14[0]
    vmla.f32    q10, q4, d14[1]
    vmla.f32    q12, q4, d15[0]
    vmla.f32    q14, q4, d15[1]

    vmla.f32    q9 , q5, d14[0]
    vmla.f32    q11, q5, d14[1]
    vmla.f32    q13, q5, d15[0]
    vmla.f32    q15, q5, d15[1]

.endm


.macro KERNEL_4x8_SUB

    vld1.32     {q0}, [A:128]!
    vld1.32     {q1, q2}, [B:128]!

    vmla.f32    q8 , q1, d0[0]
    vmla.f32    q10, q1, d0[1]
    vmla.f32    q12, q1, d1[0]
    vmla.f32    q14, q1, d1[1]

    vmla.f32    q9 , q2, d0[0]
    vmla.f32    q11, q2, d0[1]
    vmla.f32    q13, q2, d1[0]
    vmla.f32    q15, q2, d1[1]

.endm


.macro UPDATE_4x8

    vld1.32     {q0,q1}, [C1]
    vld1.32     {q2,q3}, [C2]
    vld1.32     {q4,q5}, [C3]
    vld1.32     {q6,q7}, [C4]

    vadd.f32    q8 , q8, q0
    vadd.f32    q9 , q9, q1
    vadd.f32    q10, q10, q2
    vadd.f32    q11, q11, q3
    vadd.f32    q12, q12, q4
    vadd.f32    q13, q13, q5
    vadd.f32    q14, q14, q6
    vadd.f32    q15, q15, q7

.endm

.macro SAVE_4x8

    vst1.32     {q8 , q9 }, [C1]
    vst1.32     {q10, q11}, [C2]
    vst1.32     {q12, q13}, [C3]
    vst1.32     {q14, q15}, [C4]

.endm


/**************************************************************************************
* End of macro definitions
**************************************************************************************/
    .text
    // .align 8
//                                   r0        r1             r2        r3        r4        r5                 r6                r7
// void sgemm_kernel_4x8_neon(size_t k, size_t update, float *a, float *b, float *c, size_t row_stride_c, bool bias_term, float *bias);
SUBROUTIN

    PROLOGUE

    // initial
    ldr     BIAS_TERM, [sp, #104]
    ldr     BIAS, [sp, #108]
    cmp     BIAS_TERM, #0
    beq     non_biases
#ifdef FOR_FC
    vld1.32 {q8 , q9 }, [BIAS:128]
    vld1.32 {q10, q11}, [BIAS:128]
    vld1.32 {q12, q13}, [BIAS:128]
    vld1.32 {q14, q15}, [BIAS:128]
#else
    vld1.32 {d16[], d17[]}, [BIAS:32]
    vld1.32 {d18[], d19[]}, [BIAS:32]!
    vld1.32 {d20[], d21[]}, [BIAS:32]
    vld1.32 {d22[], d23[]}, [BIAS:32]!
    vld1.32 {d24[], d25[]}, [BIAS:32]
    vld1.32 {d26[], d27[]}, [BIAS:32]!
    vld1.32 {d28[], d29[]}, [BIAS:32]
    vld1.32 {d30[], d31[]}, [BIAS:32]!
#endif
    b       smmult_start

non_biases:

    INIT_4x8

smmult_start:

    ldr     C1 , [sp, #96]
    ldr     RSC, [sp, #100]

    lsl     RSC, RSC, #2                // multiply with size of float
    add     C2, C1, RSC                 // second line of C
    add     C3, C2, RSC                 // third line of C
    add     C4, C3, RSC                 // fourth line of C

sgemm_kernel_4x8_start_8:

    asrs    L , K, #3                   // L = K / 8
    cmp     L , #2  
    blt     sgemm_kernel_4x8_only_8     // K < 2

    KERNEL_4x8_INIT_1
    KERNEL_4x8_LOOP_2
    KERNEL_4x8_LOOP_3
    KERNEL_4x8_LOOP_4

    KERNEL_4x8_LOOP_1
    KERNEL_4x8_LOOP_2
    KERNEL_4x8_LOOP_3
    KERNEL_4x8_LOOP_4

    subs    L, L, #2
    ble     sgemm_kernel_4x8_end_8      // (L -= 2) == 0

sgemm_kernel_4x8_mid_8:

    KERNEL_4x8_LOOP_1
    KERNEL_4x8_LOOP_2
    KERNEL_4x8_LOOP_3
    KERNEL_4x8_LOOP_4
    
    KERNEL_4x8_LOOP_1
    KERNEL_4x8_LOOP_2
    KERNEL_4x8_LOOP_3
    KERNEL_4x8_LOOP_4

    subs    L, L, #1
    bgt     sgemm_kernel_4x8_mid_8      // (L -= 1) > 0

sgemm_kernel_4x8_end_8:

    KERNEL_4x8_LOOP_1
    KERNEL_4x8_LOOP_2
    KERNEL_4x8_LOOP_3
    KERNEL_4x8_LOOP_4
    
    KERNEL_4x8_LOOP_1
    KERNEL_4x8_END_2
    KERNEL_4x8_END_3
    KERNEL_4x8_END_4

    b       sgemm_kernel_4x8_L_0

sgemm_kernel_4x8_only_8:

    cmp     L, #1
//     blt     sgemm_kernel_4x8_init       // L < 1
    blt     sgemm_kernel_4x8_L_0        // L < 1

    KERNEL_4x8_INIT_1
    KERNEL_4x8_LOOP_2
    KERNEL_4x8_LOOP_3
    KERNEL_4x8_LOOP_4
    KERNEL_4x8_LOOP_1
    KERNEL_4x8_END_2
    KERNEL_4x8_END_3
    KERNEL_4x8_END_4

//     b       sgemm_kernel_4x8_L_0

// sgemm_kernel_4x8_init:

//     INIT_4x8

sgemm_kernel_4x8_L_0:

    ands    L , K, #7                   // L = K % 8
    ble     sgemm_kernel_4x8_writeback  // L = 0

sgemm_kernel_4x8_only_1:

    KERNEL_4x8_SUB

    subs    L, L, #1
    bne     sgemm_kernel_4x8_only_1     // L > 0
    
sgemm_kernel_4x8_writeback:

    cmp     UPDATE, #0
    bgt     sgemm_kernel_4x8_update

sgemm_kernel_4x8_save:

    SAVE_4x8
    b       sgemm_kernel_4x8_quit

sgemm_kernel_4x8_update:

    UPDATE_4x8
    SAVE_4x8

sgemm_kernel_4x8_quit:

    EPILOGUE

#endif
