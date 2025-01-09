#ifndef _MEMINFO_H
#define _MEMINFO_H

#include <linux/mm.h>

int do_sysinfo(struct sysinfo *si);

#endif