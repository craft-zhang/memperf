/*
    time measuring with armv7a pmu
    armv7a cycle clock counter
    need insert pmu_enable loaded kernel module first
    normal resolution 1 usec
 */

#include <sys/types.h>

typedef unsigned long uint32;

extern inline uint32 rdtsc32(void) {
    unsigned r;
    static int init = 0;
    if (!init) {
        __asm__ __volatile__ ("mcr p15, 0, %0, c9, c12, 2" :: "r"(1<<31)); /* stop the cc */
        __asm__ __volatile__ ("mcr p15, 0, %0, c9, c12, 0" :: "r"(5));     /* initialize */
        __asm__ __volatile__ ("mcr p15, 0, %0, c9, c12, 1" :: "r"(1<<31)); /* start the cc */
        init = 1;
    }
    __asm__ __volatile__ ("mrc p15, 0, %0, c9, c13, 0" : "=r"(r));
    return r;
}

extern inline void getclock(uint32 *hi, uint32 *lo)
{
  *hi = 0; // TODO:
  *lo = rdtsc32();
}

/* 64 bit subtract t1-t0 */
extern inline float subtract64(uint32 hi0, uint32 lo0, uint32 hi1, uint32 lo1 )
{
  float lor, hir;
  hir = (float)(hi1 - hi0); // TODO:
  if (lo0 < lo1) {
    lor = ((float)(lo1 - lo0)) / 2000.f; // TODO: freq
  } else {
    lor = ((float)(0xFFFFFFFF - (lo0 - lo1))) / 2000.f; // TODO: freq
  }
  return lor; // TODO:
}
