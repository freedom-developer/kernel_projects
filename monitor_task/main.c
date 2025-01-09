#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/syscalls.h>

// 定义要跟踪的进程名
static char target_process[128] = "./a.out";
static struct task_struct *monitor_thread;

void monitor_process(struct task_struct *p) 
{
    long ret;
    int status;
    ret = sys_ptrace(PTRACE_ATTACH, p->tgid, NULL, NULL);
    if (ret < 0) {
        printk(KERN_ERR "sys_ptrace failed\n");
    }
    printk(KERN_INFO "sys_ptrace ret: %ld\n", ret);

    sys_waitpid(p->tgid, &status, 0);
    sys_ptrace(PTRACE_SETOPTIONS, p->tgid, 0, PTRACE_O_TRACESYSGOOD);
    
    while (1) {
        sys_ptrace(PTRACE_GETREGS, p->tgid, 0, &regs);
    }
    

}

// 注册 tracepoint 的回调函数
static int monitor_thread_fn(void *data)
{
    struct task_struct *p;

    // 保持线程运行
    while (!kthread_should_stop()) {
        for_each_process(p) {
            if (strcmp(p->comm, target_process) == 0) {
                printk(KERN_INFO "Found target process: %s (PID: %d), begin to monitor\n", p->comm, p->pid);
                monitor_process(p);
                break;
            }
        }

        msleep(HZ);
    }

    return 0;
}

static int __init syscall_monitor_init(void)
{
    // 创建监控线程
    monitor_thread = kthread_run(monitor_thread_fn, NULL, "syscall_monitor");
    if (IS_ERR(monitor_thread)) {
        printk(KERN_ERR "Failed to create monitor thread\n");
        return PTR_ERR(monitor_thread);
    }

    printk(KERN_INFO "Syscall monitor started\n");
    return 0;
}

static void __exit syscall_monitor_exit(void)
{
    if (monitor_thread) {
        kthread_stop(monitor_thread);
    }
    printk(KERN_INFO "Syscall monitor stopped\n");
}

module_init(syscall_monitor_init);
module_exit(syscall_monitor_exit);
MODULE_LICENSE("GPL");