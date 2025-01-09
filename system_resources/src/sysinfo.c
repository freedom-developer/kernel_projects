#include <linux/init.h>
#include <linux/module.h>
#include <linux/mm.h>        // 包含 si_meminfo 函数
#include <linux/kernel.h>    // 包含 printk

#include <linux/kallsyms.h>
#include <linux/swap.h>
#include <linux/sched/loadavg.h>

typedef void (*get_avenrun_fn)(unsigned long *, unsigned long , int );

int do_sysinfo(struct sysinfo *info)
{
    unsigned long mem_total, sav_total;
    unsigned int mem_unit, bitcount;
    struct timespec tp;
    get_avenrun_fn get_avenrun;
    int *nr_threads;

    memset(info, 0, sizeof(struct sysinfo));

    get_monotonic_boottime(&tp);
    info->uptime = tp.tv_sec + (tp.tv_nsec ? 1 : 0);

    get_avenrun = (get_avenrun_fn)kallsyms_lookup_name("get_avenrun");
    if (get_avenrun == NULL) {
        printk(KERN_ERR "lookup get_avenrun function failed\n");
        return -1;
    }
    (*get_avenrun)(info->loads, 0, SI_LOAD_SHIFT - FSHIFT);

    nr_threads = (int *)kallsyms_lookup_name("nr_threads");
    if (nr_threads == NULL) {
        printk(KERN_ERR "lookup nr_threads failed\n");
        return -1;
    }
    info->procs = *nr_threads;

    si_meminfo(info);
#ifdef CONFIG_SWAP
    si_swapinfo(info);
#endif

    mem_total = info->totalram + info->totalswap;
    if (mem_total < info->totalram || mem_total < info->totalswap)
        goto out;

    bitcount = 0;
    mem_unit = info->mem_unit;
    while (mem_unit > 1) {
        bitcount++;
        mem_unit >>= 1;
        sav_total = mem_total;
        mem_total <<= 1;
        if (mem_total < sav_total)
            goto out;
    }

    info->mem_unit = 1;
    info->totalram <<= bitcount;
    info->freeram <<= bitcount;
    info->sharedram <<= bitcount;
    info->bufferram <<= bitcount;
    info->totalswap <<= bitcount;
    info->freeswap <<= bitcount;
    info->totalhigh <<= bitcount;
    info->freehigh <<= bitcount;

out:
    return 0;
}




// 获取系统资源



