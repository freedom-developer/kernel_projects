#include <linux/module.h>
#include <linux/init.h>
#include <linux/version.h>
#include <linux/kdev_t.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>

#define PROC_FILE_BUF_LEN 4096
static char proc_file_buf[PROC_FILE_BUF_LEN];
static size_t count;

// 打开文件
static int proc_file_open(struct inode *inode, struct file *file)
{
    printk("proc_file_open\n");

    return 0;
}

// 读文件
static ssize_t proc_file_read(struct file *file, char __user *buffer, size_t size, loff_t *f_pos)
{
    printk("proc_file_read, count: %ld, f_pos: %ld\n", count, *f_pos);

    // 读取proc_file文件内容
    if (!count)
        return 0;
    
    size_t left = count - *f_pos;
    
    size_t copy = left < size ? left : size;
    if (copy_to_user(buffer, proc_file_buf + *f_pos, copy)) {
        printk("读取失败\n");
        return -EFAULT;
    }

    *f_pos += copy;

    return copy;
}

static ssize_t proc_file_write(struct file *file, const char __user *buffer, size_t size, loff_t *f_pos)
{
    printk("proc_file_write\n");
    if (count + *f_pos >= PROC_FILE_BUF_LEN)
        return 0;
    size_t left = PROC_FILE_BUF_LEN - *f_pos;
    size_t copy = left < size ? left : size;

    // 追加写
    if (copy_from_user(proc_file_buf + count + *f_pos, buffer, copy)) {
        printk("写失败\n");
        return -EFAULT;
    }

    count += copy;
    *f_pos += copy;

    return copy;
}

static const struct file_operations proc_file_fops = {
    .owner = THIS_MODULE,
    .open = proc_file_open,
    .read = proc_file_read,
    .write = proc_file_write,
};

static void proc_test(void)
{
    printk("proc_test\n");
    // 创建一个proc_fs目录
    struct proc_dir_entry *proc_dir = proc_mkdir("proc_test", NULL);
    if (!proc_dir) {
        printk("proc_mkdir failed\n");
        return;
    }
    
    // 创建一个proc_fs文件
    struct proc_dir_entry *proc_file = proc_create("proc_file", 0666, proc_dir, &proc_file_fops);
    if (!proc_file) {
        printk("proc_create failed\n");
        return;
    }
    
    printk("proc_test success\n");
}

static void proc_test_remove(void)
{
    printk("proc_test_remove\n");
    // 删除proc_fs目录
    remove_proc_subtree("proc_test", NULL);
}


static int __init demo_init(void)
{
    printk("demo init\n");
    proc_test();

    return 0;
}

static void __exit demo_exit(void)
{
    proc_test_remove();
    printk("demo_exit\n");
}

module_init(demo_init);
module_exit(demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("shengbangwu");
MODULE_DESCRIPTION("demo");
