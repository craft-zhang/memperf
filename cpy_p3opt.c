#include "cpy_opt.h"

#include <sys/types.h>

#ifdef __linux__
#ifdef DARMV7ACOUNTER
#include "gettimearmv7a.h"
#else
#include "gettime.h"
#define gettime gettimeus
#endif // DARMV7ACOUNTER
#else
#error "do not support this arch"
#endif // __linux__

double u;

/* load test (with neon summing to be somewhat fair against unoptimized version */
/* for understanding how the compiler works, to check the assembly code gcc generated */
int cpy_lsopt(double *ptr, int stride, int block_size, int it)
{
  szie_t n;
  n = block_size / stride;
  stride *= sizeof(double);

  uint32 hi0, lo0, hi1, lo1;
  gettime(&hi0, &lo0);

  {
    size_t i, j;
    for (i = 0; i < it; i++) {
      for (j = 0; j < stride; j++) {
        /* that addition doesn't make much sense, but we don't care about the result... */
        __asm__ __volatile__("
          ldr r0, %1
          vsub.i32 d4, d4, d4
          ldr r1, %2
          vsub.i32 d5, d5, d5
          vsub.i32 d6, d6, d6
          vsub.i32 d7, d7, d7

          vld1.32 {d0}, [r0], r1
          vadd.i32 d4, d4, d0
          vld1.32 {d1}, [r0], r1
          vadd.i32 d5, d5, d1
          vld1.32 {d2}, [r0], r1
          vadd.i32 d6, d6, d2
          vld1.32 {d3}, [r0], r1
          vadd.i32 d7, d7, d3"
        :
        : "r" (ax), "r" (bx), "r" (cx), "f" (u0), "f" (u1)
        : "st", "st(1)", "st(2)", "st(3)", "st(4)"
        );
        ptr++;
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
