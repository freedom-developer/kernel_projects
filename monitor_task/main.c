#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include "../common/Syscall.h"
#include <linux/ptrace.h>


// 定义要跟踪的进程名
static char target_process[128] = "a.out";
static struct task_struct *monitor_thread;

typedef asmlinkage long (*ptrace_t)(long request, long pid, long addr, long data);
typedef asmlinkage pid_t (*waitpid_t)(pid_t pid, int *stat_addr, int options);

void monitor_process(struct task_struct *p) 
{  
    struct user_regs_struct regs;   
    long ret; 
    
    // printk(KERN_INFO "monitor_process: %s (PID: %d)\n", p->comm, p->pid);
    ptrace_t ptrace = (ptrace_t) find_syscall("ptrace");
    if (ptrace == NULL) {
        printk(KERN_ERR "Failed to find ptrace\n");
        return;
    }
    if ((ret = ptrace(PTRACE_ATTACH, p->tgid, NULL, NULL)) < 0) {
        printk(KERN_ERR "Failed to attach to process %d, ret: %ld\n", p->tgid, ret);
        return;
    }
    if ((ret = ptrace(PTRACE_SETOPTIONS, p->tgid, 0, PTRACE_O_TRACESYSGOOD)) < 0) {
        printk(KERN_ERR "Failed to set options, ret: %ld\n", ret);
        return;
    }

    // 等待进程完成初始化
    waitpid_t waitpid = (waitpid_t) find_syscall("waitpid");
    if (waitpid == NULL) {
        printk(KERN_ERR "Failed to find waitpid\n");
        return;
    }
    waitpid(p->tgid, NULL, 0);
    while (1)
    {
        // 获取寄存器
        if ((ret = ptrace(PTRACE_GETREGS, p->tgid, NULL, &regs)) < 0) {
            printk(KERN_ERR "Failed to get registers, ret: %ld\n", ret);
            break;
        }

        // 打印寄存器
        printk(KERN_INFO "regs: %lx\n", regs.ip);

        // 单步执行
        if ((ret = ptrace(PTRACE_SINGLESTEP, p->tgid, NULL, NULL)) < 0) {
            printk(KERN_ERR "Failed to single step, ret: %ld\n", ret);
            break;
        }

        // 等待进程完成单步执行
        waitpid(p->tgid, NULL, 0);
        
        // 判断进程是否完成
        if (regs.orig_ax == __NR_exit || regs.orig_ax == __NR_exit_group) {
            printk(KERN_INFO "Process %d called exit\n", p->tgid);
            break;
        }
    }
    
}

// 注册 tracepoint 的回调函数
static int monitor_thread_fn(void *data)
{
    struct task_struct *p;
    asmlinkage pid_t (*getpid)(void);
    getpid = (asmlinkage pid_t (*)(void)) find_syscall("getpid");
    if (getpid == NULL) {
        printk(KERN_ERR "Failed to find getpid\n");
        return -1;
    } else {
        printk(KERN_INFO "getpid: %p, monitor pid is %d\n", getpid, getpid());
    }

    // 保持线程运行
    while (!kthread_should_stop()) {
        for_each_process(p) {
            if (strstr(p->comm, target_process)) {
                printk(KERN_INFO "Found target process: %s (PID: %d), begin to monitor\n", p->comm, p->tgid);
                monitor_process(p);
                break;
            }
        }
        printk(KERN_INFO "Process %s is not running\n", target_process);

        msleep(1000);
    }

    return 0;
}

static int __init monitor_init(void)
{
    // 创建监控线程
    monitor_thread = kthread_run(monitor_thread_fn, NULL, "monitor");
    if (IS_ERR(monitor_thread)) {
        printk(KERN_ERR "Failed to create monitor thread\n");
        return PTR_ERR(monitor_thread);
    }
    
    printk(KERN_INFO "monitor started\n");
    return 0;
}

static void __exit monitor_exit(void)
{
    if (monitor_thread) {
        kthread_stop(monitor_thread);
    }
    printk(KERN_INFO "monitor stopped\n");
}

module_init(monitor_init);
module_exit(monitor_exit);
MODULE_LICENSE("GPL");