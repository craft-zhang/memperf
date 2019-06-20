#include "cpy_opt.h"
#include <stdio.h>
#ifdef __linux__
#ifdef DARMV7ACOUNTER
#include "gettimearmv7a.h"
#define gettime getclock
#else
#include "gettime.h"
#define gettime gettimeus
#endif // DARMV7ACOUNTER
#else
#error "do not support this arch"
#endif // __linux__

/* Three lines of rumors */
/* for understanding how the compiler frontend works, to check the assembly code gcc generated */
/* for understanding how the compiler backend works, to check the binary code gcc generated    */
/* for understanding how the CPU works, to observe the behavior of the CPU                     */

/* load test (with neon summing to be somewhat fair against unoptimized version */
int cpy_lsopt(double *ptr, int stride, int block_size, int it)
{
  // printf("%p %d %d %d\n", ptr, stride, block_size, it);
  register double *entry asm("r4");
  register size_t n asm("r5");
  n = block_size / stride;
  register size_t stride_r asm("r6");
  stride_r = stride * sizeof(double);

  uint32 hi0, lo0, hi1, lo1;
  gettime(&hi0, &lo0);

  {
    int i, j;
    for (i = 0; i < it; i++) {
      for (j = 0; j < stride; j++) {
        entry = ptr + j;
        // printf("%p %d %d %d %d\n", entry, stride_r, n, i, j);
        /* that addition doesn't make much sense, but we don't care about the result... */
        __asm__ __volatile__(
          "vsub.i64 d8, d8, d8 \n\t"
          "vsub.i64 d9, d9, d9 \n\t"
          "vsub.i64 d10, d10, d10 \n\t"
          "vsub.i64 d11, d11, d12 \n\t"
          "asrs r7, %2, #3 \n"
"ls_loop_8: \n\t"
          "vld1.32 {d0}, [%0], %1 \n\t"
          "vadd.i64 d8, d8, d0 \n\t"
          "vld1.32 {d1}, [%0], %1 \n\t"
          "vadd.i64 d9, d9, d1 \n\t"
          "vld1.32 {d2}, [%0], %1 \n\t"
          "vadd.i64 d10, d10, d2 \n\t"
          "vld1.32 {d3}, [%0], %1 \n\t"
          "vadd.i64 d11, d11, d3 \n\t"
          "sub r7, r7, #1 \n\t"
          "vld1.32 {d4}, [%0], %1 \n\t"
          "vadd.i64 d8, d8, d4 \n\t"
          "vld1.32 {d5}, [%0], %1 \n\t"
          "vadd.i64 d9, d9, d5 \n\t"
          "vld1.32 {d6}, [%0], %1 \n\t"
          "vadd.i64 d10, d10, d6 \n\t"
          "vld1.32 {d7}, [%0], %1 \n\t"
          "vadd.i64 d11, d11, d7 \n\t"
          "cmp r7, #1 \n\t"
          "bge ls_loop_8 \n\t"
        :
        :"r"(entry), "r"(stride_r), "r"(n)
        :"cc", "r7", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "d8", "d9", "d10", "d11"
        );
      }
    }
  }

  gettime(&hi1, &lo1);
  return subtract64(hi0,lo0,hi1,lo1);
}

/* const store test */
int cpy_vsopt(double *ptr, int stride, int block_size, int it)
{
  uint32 hi0, lo0, hi1, lo1;
  gettime(&hi0, &lo0);
  gettime(&hi1, &lo1);
  return subtract64(hi0,lo0,hi1,lo1);
}

/* load copy test (strided load, contiguous store) */
int cpy_lcopt(double *ptr_l, double *ptr_s, int stride, int block_size, int it)
{
  uint32 hi0, lo0, hi1, lo1;
  gettime(&hi0, &lo0);
  gettime(&hi1, &lo1);
  return subtract64(hi0,lo0,hi1,lo1);
}

/* copy store test (contiguous load, strided store) */
int cpy_csopt(double *ptr_l, double *ptr_s, int stride, int block_size, int it)
{
  uint32 hi0, lo0, hi1, lo1;
  gettime(&hi0, &lo0);
  gettime(&hi1, &lo1);
  return subtract64(hi0,lo0,hi1,lo1);
}
