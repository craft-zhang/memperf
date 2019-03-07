/*
*Enable user-mode ARM performance counter access.
*/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/smp.h>

#define PERF_DEF_OPTS           (1|16)
#define PERF_OPT_RESET_CYCLES   (2|4)
#define PERF_OPT_DIV64          (8)

#define ARMV7_PMCR_MASK         0x3f
#define ARMV7_PMCR_E            (1<<0) /*Enable all counters*/
#define ARMV7_PMCR_P            (1<<1) /*Reset all counters*/
#define ARMV7_PMCR_C            (1<<2) /*Cycle counter reset*/
#define ARMV7_PMCR_D            (1<<3) /*CCNT counts every 64th cpu cycle*/
#define ARMV7_PMCR_X            (1<<4) /*Export of events enable*/
#define ARMV7_PMCR_DP           (1<<5) /*Disable Cycle Count if non-invasive debug*/
#define ARMV7_PMCR_LC           (1<<6) /*Cycle Counter 64bit overflow*/

#define ARMV7_PMCR_N_SHIFT      11     /*Number of counters supported*/
#define ARMV7_PMCR_N_MASK       0x1f

#define ARMV7_PMUSERENR_EN_EL0  (1<<0) /*EL0 access enable*/
#define ARMV7_PMUSERENR_CR      (1<<2) /*Cycle counter read enable*/
#define ARMV7_PMUSERENR_ER      (1<<3) /*Event counter read enable*/

static inline u32 armv7pmu_pmcr_read(void)
{
    u32 val=0;
    asm volatile("mrc p15, 0, %0, c9, c12, 0" :"=r"(val));
    return val;
}

static inline void armv7pmu_pmcr_write(u32 val)
{
    val &= ARMV7_PMCR_MASK;
    isb();
    asm volatile("mcr p15, 0, %0, c9, c12, 0" : :"r"(val));
}

static void
enable_cpu_counters(void* data)
{
    asm volatile("mcr p15, 0, %0, c9, c14, 0" : :"r"(1)); /* initialize */
    armv7pmu_pmcr_write(ARMV7_PMCR_LC|ARMV7_PMCR_E);
    armv7pmu_pmcr_write(armv7pmu_pmcr_read()|ARMV7_PMCR_E|ARMV7_PMCR_LC);
}

static void
disable_cpu_counters(void* data)
{
    asm volatile("mcr p15, 0, %0, c9, c14, 0" : :"r"(0));
    printk("\ndisabling user-mode PMU accesson CPU#%d"
                , smp_processor_id());
    /*Program PMU and disable all counters*/
    armv7pmu_pmcr_write(armv7pmu_pmcr_read() | ~ARMV7_PMCR_E);
}

static int init(void)
{
    isb();
    on_each_cpu(enable_cpu_counters, NULL, 1);
    printk("Enable Access PMU");
    return 0;
}

static void fini(void)
{
    on_each_cpu(disable_cpu_counters, NULL, 1);
    printk("Access PMU Disabled");
    return;
}

module_init(init);
module_exit(fini);
