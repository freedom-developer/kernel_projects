#include "Syscall.h"

/*
* 查找某个系统调用
*/
void *_find_syscall(char *syscall_name)
{
    void *syscall_addr = NULL;
    if (syscall_name == NULL) {
        printk(KERN_ERR "syscall_name is NULL\n");
        return NULL;
    }
    syscall_addr = kallsyms_lookup_name(syscall_name);
    if (syscall_addr == NULL) {
        printk(KERN_ERR "Failed to find syscall: %s\n", syscall_name);
        return NULL;
    }
    return syscall_addr;
}