/*
 * Author: zhangdanfeng@cloudwalk.cn
 * Developed at ShangHai China.
 */
//
// TODO: rank-one update cannot fuse relu with gemm
#include "conv_2d_fast.h"

#include <assert.h>
#include <stdint.h>

#define min(x, y) x < y? x: y;

#ifdef __cplusplus
extern "C" {
#endif
    void sgemm_kernel_interleave_a_neon(size_t k, float *a, size_t m, size_t a_cols, float *a_to);
    void sgemm_kernel_interleave_b_neon(size_t k, float *b, size_t n, size_t b_cols, float *b_to);

    void sgemm_kernel_4x4_neon(size_t k, float *a, float *b, float *c
                , bool bias_term, float *bias);
    void fc_sgemm_kernel_4x4_neon(size_t k, float *a, float *b, float *c
                , bool bias_term, float *bias);
    void sgemm_kernel_4x8_neon(size_t k, size_t update
                , float *a, float *b, float *c, size_t row_stride_c
                , bool bias_term, float *bias);
    void fc_sgemm_kernel_4x8_neon(size_t k, size_t update
                , float *a, float *b, float *c, size_t row_stride_c
                , bool bias_term, float *bias);
#ifdef __cplusplus
}
#endif

size_t interleave_kernel_space_size(size_t kernel_chan, size_t kernel_size)
{
    assert(kernel_chan > 0 && kernel_size > 0);
    if (kernel_chan & 3)
        kernel_chan = ((kernel_chan >> 2) + 1) << 2;
    assert(kernel_chan > 0 && (kernel_chan & 3) == 0);
    return kernel_chan * kernel_size * sizeof(float);
}

void interleave_kernel(float *kernel, float *kernel_interleaved, size_t kernel_chan, size_t kernel_size)
{
    // block start point
    size_t kernel_block, channel_block;
    // block real depth
    size_t kernel_depth, channel_depth;
    // current index
    size_t channel_index;

    for (kernel_block = 0; kernel_block < kernel_size; kernel_block += KERNEL_L1_LIMIT) {
        kernel_depth = min(kernel_size - kernel_block, KERNEL_L1_LIMIT);
        for (channel_block = 0; channel_block < kernel_chan; channel_block += KERNEL_L2_LIMIT) {
            channel_depth = min(kernel_chan - channel_block, KERNEL_L2_LIMIT);
            for (channel_index = 0; channel_index < (channel_depth & -4); channel_index += 4) {
                sgemm_kernel_interleave_a_neon(kernel_depth
                            , kernel + (channel_block + channel_index) * kernel_size + kernel_block
                            , 4, kernel_size, kernel_interleaved);
                kernel_interleaved += kernel_depth * 4;
            }
            if (channel_depth & 0x3) {
                sgemm_kernel_interleave_a_neon(kernel_depth
                            , kernel + (channel_block + channel_index) * kernel_size + kernel_block
                            , channel_depth & 0x3, kernel_size, kernel_interleaved);
                kernel_interleaved += kernel_depth * 4;
            }
        }
    }
}

size_t interleave_col_space_size(size_t kernel_size, size_t col_num)
{
    assert(kernel_size > 0 && col_num > 0);
    if (col_num & 3)
        col_num = ((col_num >> 2) + 1) << 2;
    assert(col_num > 0 && (col_num & 3) == 0);
    return col_num * kernel_size * sizeof(float);
}

void interleave_col(float *col, float *col_interleaved, size_t kernel_size, size_t col_num)
{
    // block start point
    size_t kernel_block;
    // block real depth
    size_t kernel_depth;
    // current index
    size_t col_index;

    for (kernel_block = 0; kernel_block < kernel_size; kernel_block += KERNEL_L1_LIMIT) {
        kernel_depth = min(kernel_size - kernel_block, KERNEL_L1_LIMIT);
        for (col_index = 0; col_index < (col_num & -8); col_index += 8) {
            sgemm_kernel_interleave_b_neon(kernel_depth
                        , col + kernel_block * col_num + col_index
                        , 8, col_num, col_interleaved);
            col_interleaved += kernel_depth*8;
        }
        size_t nr;
        // column remainder
        for ( ; col_index < col_num; col_index += 4) {
            nr = min(col_num - col_index, 4);
            sgemm_kernel_interleave_b_neon(kernel_depth
                        , col + kernel_block * col_num + col_index
                        , nr, col_num, col_interleaved);
            col_interleaved += kernel_depth*4;
        }
    }
}

void conv_sgemm(float *kernel, float *col, float *biases
            // kernel_size = input_channel x kernel_h x kernel_w, col_num = output_h * output_w
            , size_t ouput_channel, size_t kernel_size, size_t col_num
            , bool bias_term, bool relu_fused, float *output)
{
    float result[32];
    // block start point
    size_t kernel_block, channel_block;
    // block real depth
    size_t kernel_depth, channel_depth;
    // current index
    size_t channel_index, col_index;

    size_t i, j;

    float *cur_output;

    for (kernel_block = 0; kernel_block < kernel_size; kernel_block += KERNEL_L1_LIMIT) {
        kernel_depth = min(kernel_size - kernel_block, KERNEL_L1_LIMIT);
        // rank 1 update
        cur_output = output;
        for (channel_block = 0; channel_block < ouput_channel; channel_block += KERNEL_L2_LIMIT) {
            channel_depth = min(ouput_channel - channel_block, KERNEL_L2_LIMIT);
            // GEBP, data flow in L1 data cache and L2 cache
            for (col_index = 0; col_index < (col_num & -8); col_index += 8) {
                for (channel_index = 0; channel_index < (channel_depth & -4); channel_index += 4) {
                    sgemm_kernel_4x8_neon(kernel_depth
                                , kernel_block
                                , kernel + channel_index * kernel_depth
                                , col + col_index * kernel_depth
                                , cur_output + channel_index * col_num + col_index, col_num
                                , bias_term&(!kernel_block), biases + channel_block + channel_index);
                }
                if (channel_depth & 0x3) {
                    sgemm_kernel_4x8_neon(kernel_depth
                                , 0
                                , kernel + channel_index * kernel_depth
                                , col + col_index * kernel_depth
                                , result, 8
                                , bias_term&(!kernel_block), biases + channel_block + channel_index);
                    if (kernel_block) {
                        for(i = 0; i < (channel_depth & 0x3); i++) {
                            *(cur_output + (channel_index + i) * col_num + col_index + 0) += result[(i<<3)+0];
                            *(cur_output + (channel_index + i) * col_num + col_index + 1) += result[(i<<3)+1];
                            *(cur_output + (channel_index + i) * col_num + col_index + 2) += result[(i<<3)+2];
                            *(cur_output + (channel_index + i) * col_num + col_index + 3) += result[(i<<3)+3];
                            *(cur_output + (channel_index + i) * col_num + col_index + 4) += result[(i<<3)+4];
                            *(cur_output + (channel_index + i) * col_num + col_index + 5) += result[(i<<3)+5];
                            *(cur_output + (channel_index + i) * col_num + col_index + 6) += result[(i<<3)+6];
                            *(cur_output + (channel_index + i) * col_num + col_index + 7) += result[(i<<3)+7];
                        }
                    }  else {
                        for(i = 0; i < (channel_depth & 0x3); i++) {
                            *(cur_output + (channel_index + i) * col_num + col_index + 0) = result[(i<<3)+0];
                            *(cur_output + (channel_index + i) * col_num + col_index + 1) = result[(i<<3)+1];
                            *(cur_output + (channel_index + i) * col_num + col_index + 2) = result[(i<<3)+2];
                            *(cur_output + (channel_index + i) * col_num + col_index + 3) = result[(i<<3)+3];
                            *(cur_output + (channel_index + i) * col_num + col_index + 4) = result[(i<<3)+4];
                            *(cur_output + (channel_index + i) * col_num + col_index + 5) = result[(i<<3)+5];
                            *(cur_output + (channel_index + i) * col_num + col_index + 6) = result[(i<<3)+6];
                            *(cur_output + (channel_index + i) * col_num + col_index + 7) = result[(i<<3)+7];
                        }
                    }
                    channel_index += 4;
                }
            }
            size_t mr, nr;
            // column remainder
            for ( ; col_index < col_num; col_index += 4) {
                nr = min(col_num - col_index, 4);
                for (channel_index = 0; channel_index < channel_depth; channel_index += 4) {
                    mr = min(channel_depth - channel_index, 4);
                    sgemm_kernel_4x4_neon(kernel_depth
                                , kernel + channel_index * kernel_depth
                                , col + col_index * kernel_depth
                                , result
                                , bias_term&(!kernel_block), biases + channel_block + channel_index);
                    if (kernel_block) {
                        for(i = 0; i < mr; i++) {
                            for(j = 0; j < nr; j++) {
                                *(cur_output + (channel_index + i) * col_num + col_index + j) += result[(i << 2) + j];
                            }
                        }
                    }  else {
                        for(i = 0; i < mr; i++) {
                            for(j = 0; j < nr; j++) {
                                *(cur_output + (channel_index + i) * col_num + col_index + j) = result[(i << 2) + j];
                            }
                        }
                    }
                }
            }
            cur_output += col_num * channel_depth; // real
            kernel += kernel_depth * channel_index; // rounded up for aligned load
        }
        col += kernel_depth * col_index; // rounded up for aligned load
    }
    return;
}

void fc_batch_sgemm(float *kernel, float *col, float *biases
            // kernel_size = input_channel x kernel_h x kernel_w, col_num = output_h * output_w
            , size_t ouput_channel, size_t kernel_size, size_t col_num
            , bool bias_term, bool relu_fused, float *output)
{
    float result[32];
    // block start point
    size_t kernel_block, channel_block;
    // block real depth
    size_t kernel_depth, channel_depth;
    // current index
    size_t channel_index, col_index;

    size_t i, j;

    float *cur_output;

    for (kernel_block = 0; kernel_block < kernel_size; kernel_block += KERNEL_L1_LIMIT) {
        kernel_depth = min(kernel_size - kernel_block, KERNEL_L1_LIMIT);
        // rank 1 update
        cur_output = output;
        for (channel_block = 0; channel_block < ouput_channel; channel_block += KERNEL_L2_LIMIT) {
            channel_depth = min(ouput_channel - channel_block, KERNEL_L2_LIMIT);
            // GEBP, data flow in L1 data cache and L2 cache
            for (col_index = 0; col_index < (col_num & -8); col_index += 8) {
                for (channel_index = 0; channel_index < (channel_depth & -4); channel_index += 4) {
                    fc_sgemm_kernel_4x8_neon(kernel_depth
                                , kernel_block
                                , kernel + channel_index * kernel_depth
                                , col + col_index * kernel_depth
                                , cur_output + channel_index * col_num + col_index, col_num
                                , bias_term&(!kernel_block), biases + col_index);
                }
                if (channel_depth & 0x3) {
                    fc_sgemm_kernel_4x8_neon(kernel_depth
                                , 0
                                , kernel + channel_index * kernel_depth
                                , col + col_index * kernel_depth
                                , result, 8
                                , bias_term&(!kernel_block), biases + col_index);
                    if (kernel_block) {
                        for(i = 0; i < (channel_depth & 0x3); i++) {
                            *(cur_output + (channel_index + i) * col_num + col_index + 0) += result[(i<<3)+0];
                            *(cur_output + (channel_index + i) * col_num + col_index + 1) += result[(i<<3)+1];
                            *(cur_output + (channel_index + i) * col_num + col_index + 2) += result[(i<<3)+2];
                            *(cur_output + (channel_index + i) * col_num + col_index + 3) += result[(i<<3)+3];
                            *(cur_output + (channel_index + i) * col_num + col_index + 4) += result[(i<<3)+4];
                            *(cur_output + (channel_index + i) * col_num + col_index + 5) += result[(i<<3)+5];
                            *(cur_output + (channel_index + i) * col_num + col_index + 6) += result[(i<<3)+6];
                            *(cur_output + (channel_index + i) * col_num + col_index + 7) += result[(i<<3)+7];
                        }
                    }  else {
                        for(i = 0; i < (channel_depth & 0x3); i++) {
                            *(cur_output + (channel_index + i) * col_num + col_index + 0) = result[(i<<3)+0];
                            *(cur_output + (channel_index + i) * col_num + col_index + 1) = result[(i<<3)+1];
                            *(cur_output + (channel_index + i) * col_num + col_index + 2) = result[(i<<3)+2];
                            *(cur_output + (channel_index + i) * col_num + col_index + 3) = result[(i<<3)+3];
                            *(cur_output + (channel_index + i) * col_num + col_index + 4) = result[(i<<3)+4];
                            *(cur_output + (channel_index + i) * col_num + col_index + 5) = result[(i<<3)+5];
                            *(cur_output + (channel_index + i) * col_num + col_index + 6) = result[(i<<3)+6];
                            *(cur_output + (channel_index + i) * col_num + col_index + 7) = result[(i<<3)+7];
                        }
                    }
                    channel_index += 4;
                }
            }
            size_t mr, nr;
            // column remainder
            for ( ; col_index < col_num; col_index += 4) {
                nr = min(col_num - col_index, 4);
                for (channel_index = 0; channel_index < channel_depth; channel_index += 4) {
                    mr = min(channel_depth - channel_index, 4);
                    fc_sgemm_kernel_4x4_neon(kernel_depth
                                , kernel + channel_index * kernel_depth
                                , col + col_index * kernel_depth
                                , result
                                , bias_term&(!kernel_block), biases + col_index);
                    if (kernel_block) {
                        for(i = 0; i < mr; i++) {
                            for(j = 0; j < nr; j++) {
                                *(cur_output + (channel_index + i) * col_num + col_index + j) += result[(i << 2) + j];
                            }
                        }
                    }  else {
                        for(i = 0; i < mr; i++) {
                            for(j = 0; j < nr; j++) {
                                *(cur_output + (channel_index + i) * col_num + col_index + j) = result[(i << 2) + j];
                            }
                        }
                    }
                }
            }
            cur_output += col_num * channel_depth; // real
            kernel += kernel_depth * channel_index; // rounded up for aligned load
        }
        col += kernel_depth * col_index; // rounded up for aligned load
    }
    return;
}
