#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/syscall.h>

// tegra pmu invalide because of cpu idle

#define errExit(msg) do { \
        perror(msg); \
        exit(EXIT_FAILURE); \
    } while (0)

#ifdef __linux__
#define CPU_SETSIZE 1024
#define CPU_SET(cpu, cpusetp) \
  ((cpusetp)->__bits[(cpu)/__NCPUBITS] |= (1UL << ((cpu) % __NCPUBITS)))
#define CPU_ZERO(cpusetp) \
  memset((cpusetp), 0, sizeof(cpu_set_t))
#endif

#define START_PROFILE struct timeval tvs,tve; \
        long long ts, te, ta; \
        gettimeofday(&tvs,NULL); \
        ts = tvs.tv_sec*1000000 + tvs.tv_usec;
#define PROFILE_POINT(name) gettimeofday(&tve,NULL); \
        te = tve.tv_sec*1000000 + tve.tv_usec; \
        fprintf(stderr, "%s:%lld us\n", name, te - ts);

static uint32_t rdtsc32(void) {
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

/* Simple loop body to keep things interested. Make sure it gets inlined. */
static inline int loop(int* __restrict__ a,
                       int* __restrict__ b,
                       int n) {
    unsigned sum = 0;
    for (int i = 0; i < n; ++i)
        if(a[i] > b[i])
            sum += a[i] + 5;
    return sum;
}

void entry(size_t n) {
    uint32_t time_start = 0;
    uint32_t time_end   = 0;

    int *a  = NULL;
    int *b  = NULL;
    int len = n;
    int sum = 0;

    a = malloc(len*sizeof(*a));
    b = malloc(len*sizeof(*b));

    for (int i = 0; i < len; ++i) {
            a[i] = i+128;
            b[i] = i+64;
    }

    printf("%d: beginning loop\n", len);
    START_PROFILE
    time_start = rdtsc32();
    sum = loop(a, b, len);
    time_end   = rdtsc32();
    PROFILE_POINT("systime")
    printf("%d: done. sum = %d; time delta = %u\n", len, sum, time_end - time_start);

    free(a); free(b);
    return;
}

int main() {
    pid_t pid;
    pid = syscall(SYS_gettid);
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(4, &set);
    int syscallret = syscall(__NR_sched_setaffinity, pid, sizeof(set), &set);
    if (syscallret) {
        fprintf(stderr, "syscall error %d\n", syscallret);
        return -1;
    }
    for(size_t i = 0; i < 10; i++) {
    //entry(16);
    //entry(32);
    //entry(64);
    //entry(128);
    //entry(256);
    //entry(512);
    //entry(1024);
    entry(40960);
    }

    return 0;
}
