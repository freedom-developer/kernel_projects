#ifndef _SYSCALL_H_
#define _SYSCALL_H_

#include <linux/syscalls.h>
#include <linux/kallsyms.h>

/*
* 查找某个系统调用
*/
void *_find_syscall(char *syscall_name);

#define find_syscall(syscall_name) _find_syscall("sys_" syscall_name)

#endif