#include "cpy_opt.h"

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
  register size_t n asm("r4");
  n = block_size / stride;
  stride *= sizeof(double);

  uint32 hi0, lo0, hi1, lo1;
  gettime(&hi0, &lo0);

  {
    int i, j;
    for (i = 0; i < it; i++) {
      for (j = 0; j < stride; j++) {
        /* that addition doesn't make much sense, but we don't care about the result... */
        __asm__ __volatile__(
	  "asrs    %3, r5, #3 \n"
"ls_loop_8: \n\t"
	  "cmp r5, #1 \n\t"
          "blt ls_loop_end\n\t" 
          "vld1.32 {d0}, [%1], %2 \n\t"
          "vld1.32 {d1}, [%1], %2 \n\t"
          "vld1.32 {d2}, [%1], %2 \n\t"
          "vld1.32 {d3}, [%1], %2 \n\t"
	  "sub r5, r5, #1 \n\t"
          "vld1.32 {d4}, [%1], %2 \n\t"
          "vld1.32 {d5}, [%1], %2 \n\t"
          "vld1.32 {d6}, [%1], %2 \n\t"
          "vld1.32 {d7}, [%1], %2 \n\t"
"ls_loop_end: \n\t"
        :
	:"r"(ptr), "r"(stride), "r"(n)
        :"cc"
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
