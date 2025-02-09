#include <linux/module.h>
#include <linux/init.h>
#include <linux/version.h>
#include <linux/kdev_t.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>

#include <linux/sched/signal.h>
#include <asm/pgtable.h>

#define POINTER 0x7ffd496e08f0

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

struct task_struct *find_task_by_name(const char *name)
{
    struct task_struct *p;
    for_each_process(p) {
        if (!strcmp(p->comm, name)) {
            return p;
        }
    }
    return NULL;
}

// 打印一页中的表项
void print_page(unsigned long *page)
{
    int i;
    // 页中保存的地址是物理地址
    if (!page) {
        printk("page is NULL\n");
        return;
    }
    for (i = 0; i < PTRS_PER_PTE; i++) {
        if (!page[i]) {
            continue;
        }
        printk("%03d: %lx\n", i, page[i]);
    }
}

// 根据一个线性地址查找其pgd表项, pud表项, pmd表项, pte表项
void print_paging(struct mm_struct *mm, unsigned long addr)
{
    pgd_t *pgd;
    pud_t *pud;
    pmd_t *pmd;
    pte_t *pte;
    int idx;
    unsigned long phys;

    // 根据mm_struct结构体查找pgd表项
    pgd = mm->pgd; // PGD对应的页的虚拟地址
    print_page((unsigned long *)pgd);
    // 根据线性地址查找pgd表项
    idx = pgd_index(addr);
    pgd = pgd + idx; // pgd表项
    printk("pgd: idx = %d, val = %lx\n", idx, *pgd);
    
    // 根据pgd表项查找pud表项
    pud = pgd_page_vaddr(*pgd); // PUD对应的页的虚拟地址
    print_page((unsigned long *)pud);
    // 根据线性地址查找pud表项
    idx = pud_index(addr);
    pud = pud + idx; // pud表项
    printk("pud: idx = %d, val = %lx\n", idx, *pud);

    // 根据pud表项查找pmd表项
    pmd = pud_page_vaddr(*pud); // PMD对应的页的虚拟地址
    print_page((unsigned long *)pmd);
    // 根据线性地址查找pmd表项
    idx = pmd_index(addr);
    pmd = pmd + idx; // pmd表项
    printk("pmd: idx = %d, val = %lx\n", idx, *pmd);

    // 根据pmd表项查找pte表项
    pte = (pte_t *)pmd_page_vaddr(*pmd); // PTE对应的页的虚拟地址
    print_page((unsigned long *)pte);
    // 根据线性地址查找pte表项
    idx = pte_index(addr);
    pte = pte + idx; // pte表项
    printk("pte: idx = %d, val = %lx\n", idx, *pte);

    // 根据pte表项查找物理地址
    phys = (unsigned long)pte_pfn(*pte); // 获取pte表项对应的页框号
    phys = phys << PAGE_SHIFT; // 页框号转换为物理地址
    phys = phys | (addr & ~PAGE_MASK); // 物理地址
    printk("phys: %lx\n", phys);
    
}

void paging_test(void)
{
    printk("paging_test\n");
    printk("PAGE_OFFSET: %lx\n", PAGE_OFFSET);
    printk("PAGE_SHIFT: %lx\n", PAGE_SHIFT);
    printk("PAGE_MASK: %lx\n", PAGE_MASK);
    printk("PAGE_SIZE: %lx\n", PAGE_SIZE);

    struct task_struct *tsk = find_task_by_name("a.out");
    if (!tsk) {
        printk(KERN_ERR "find_task_by_name failed\n");
        return -1;
    }
    printk("find_task_by_name success\n");
    printk("pid: %d, comm: %s\n", tsk->pid, tsk->comm);
    pgd_t *pgd = tsk->mm->pgd;
    printk("cr3: %lx\n", pgd);

    // 给进程tsk的用户空间的指定虚拟地址上分配一个页框，并在此页框中设置一个4字节整数，然后向进程tsk发送信号SIGUSR1，让进程tsk执行信号处理函数，在信号处理函数中读取该整数，并打印出来。
    unsigned long virt = POINTER;
    struct page *page = get_free_page(GFP_KERNEL);
    if (!page) {
        printk(KERN_ERR "get_free_page failed\n");
        return -1;
    }
    printk("get_free_page success\n");
    printk("page: %lx\n", page);
    printk("page->virtual: %lx\n", page_address(page));
    printk("page->physical: %lx\n", page_to_phys(page));
    printk("page->index: %lx\n", page->index);

    
}

static int __init demo_init(void)
{
    printk("demo init\n");
    paging_test();
    

    return 0;
}

static void __exit demo_exit(void)
{
    printk("demo_exit\n");
}

module_init(demo_init);
module_exit(demo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("shengbangwu");
MODULE_DESCRIPTION("demo");
