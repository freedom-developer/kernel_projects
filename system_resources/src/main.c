#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>

#include <linux/proc_fs.h>
#include <linux/sysctl.h>
#include <stddef.h>

#include "sysinfo.h"

MODULE_LICENSE("GPL");

struct sysinfo sysinfo;

// 对应目录 /proc/sys/sysres/sysinfo/xxx
static struct ctl_table sysres_sysinfo[] = {
    {
        .procname = "totalram",
        .data = &sysinfo.totalram,
        .maxlen = sizeof(sysinfo.totalram),
        .mode = 0644,
        .proc_handler = proc_dointvec,
    },
    {
        .procname = "sharedram",
        .data = &sysinfo.sharedram,
        .maxlen = sizeof(sysinfo.sharedram),
        .mode = 0644,
        .proc_handler = proc_dointvec,
    },
    {
        .procname = "freeram",
        .data = &sysinfo.freeram,
        .maxlen = sizeof(sysinfo.freeram),
        .mode = 0644,
        .proc_handler = proc_dointvec,
    },
    {
        .procname = "bufferram",
        .data = &sysinfo.bufferram,
        .maxlen = sizeof(sysinfo.bufferram),
        .mode = 0644,
        .proc_handler = proc_dointvec,
    },
    {
        .procname = "totalhigh",
        .data = &sysinfo.totalhigh,
        .maxlen = sizeof(sysinfo.totalhigh),
        .mode = 0644,
        .proc_handler = proc_dointvec,
    },
    {
        .procname = "freehigh",
        .data = &sysinfo.freehigh,
        .maxlen = sizeof(sysinfo.freehigh),
        .mode = 0644,
        .proc_handler = proc_dointvec,
    },
    {
        .procname = "uptime",
        .data = &sysinfo.uptime,
        .maxlen = sizeof(sysinfo.uptime),
        .mode = 0644,
        .proc_handler = proc_dointvec,
    },
    {
        .procname = "loads",
        .data = &sysinfo.loads,
        .maxlen = sizeof(sysinfo.loads),
        .mode = 0644,
        .proc_handler = proc_dointvec,
    },
    {
        .procname = "totalswap",
        .data = &sysinfo.totalswap,
        .maxlen = sizeof(sysinfo.totalswap),
        .mode = 0644,
        .proc_handler = proc_dointvec,
    },
    {
        .procname = "freeswap",
        .data = &sysinfo.freeswap,
        .maxlen = sizeof(sysinfo.freeswap),
        .mode = 0644,
        .proc_handler = proc_dointvec,
    },
    {
        .procname = "procs",
        .data = &sysinfo.procs,
        .maxlen = sizeof(sysinfo.procs),
        .mode = 0644,
        .proc_handler = proc_dointvec,
    },
    {}
};

// 对应目录 /proc/sys/sysres/sysinfo
static struct ctl_table sysres_entries[] = {
    {
        .procname = "sysinfo",
        .mode = 0555,
        .child = sysres_sysinfo,
    },
    { }
};

// 对应目录 /proc/sys/sysres
static struct ctl_table sysres_root_table[] = {
    {
        .procname = "sysres",
        .mode = 0555,
        .child = sysres_entries,
    },
    { }
};

static struct ctl_table_header *sysres_table_header;

static int __init sys_res_init(void)
{
    do_sysinfo(&sysinfo);

    sysres_table_header = register_sysctl_table(sysres_root_table);
    if (!sysres_table_header) {
        printk(KERN_ERR "register sysres_root_table failed\n");
        return -ENOMEM;
    }
    return 0;
}


static void __exit sys_res_exit(void)
{
    unregister_sysctl_table(sysres_table_header);
}


module_init(sys_res_init);
module_exit(sys_res_exit);