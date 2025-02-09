#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched/signal.h>
#include <asm/pgtable.h>

#define POINTER 0x7ffd496e08f0

// 根据进程名查找进程
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

// 打印一页中的非0物理地址表项
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

// 根据一个线性地址查找其pgd表项, pud表项, pmd表项, pte表项，进而获取该线性地址对应的物理地址
// 注意此线性地址为某个进程地址空间的线性地址，而不是内核地址空间的线性地址
void print_paging(struct task_struct *tsk, unsigned long addr)
{
    pgd_t *pgd;
    pud_t *pud;
    pmd_t *pmd;
    pte_t *pte;
    int idx;
    unsigned long phys;

    // 查找addr对应的pgd表项：可通过pgd_offset(tsk->mm, addr)获取
    pgd = tsk->mm->pgd; // PGD对应的页的虚拟地址
    print_page((unsigned long *)pgd);
    // 根据线性地址查找pgd表项
    idx = pgd_index(addr);
    pgd = pgd + idx; // pgd表项
    printk("pgd: idx = %d, val = %lx\n", idx, *pgd);
    // 通过pgd_offset获取pgd表项
    printk("pgd: pgd_offset %lx\n", *(pgd_offset(tsk->mm, addr)));
    
    // 根据pgd表项查找addr对应的pud表项：可通过pud_offset(pgd, addr)获取
    pud = pgd_page_vaddr(*pgd); // PUD对应的页的虚拟地址
    print_page((unsigned long *)pud);
    // 根据线性地址查找pud表项
    idx = pud_index(addr);
    pud = pud + idx; // pud表项
    printk("pud: idx = %d, val = %lx\n", idx, *pud);
    // 通过pud_offset获取pud表项
    // printk("pud: pud_offset %lx\n", *(pud_offset(pgd, addr)));

    // 根据pud表项查找pmd表项
    pmd = pud_page_vaddr(*pud); // PMD对应的页的虚拟地址
    print_page((unsigned long *)pmd);
    // 根据线性地址查找pmd表项
    idx = pmd_index(addr);
    pmd = pmd + idx; // pmd表项
    printk("pmd: idx = %d, val = %lx\n", idx, *pmd);
    // 通过pmd_offset获取pmd表项
    printk("pmd: pmd_offset %lx\n", *(pmd_offset(pud, addr)));

    // 根据pmd表项查找pte表项
    pte = (pte_t *)pmd_page_vaddr(*pmd); // PTE对应的页的虚拟地址
    print_page((unsigned long *)pte);
    // 根据线性地址查找pte表项
    idx = pte_index(addr);
    pte = pte + idx; // pte表项
    printk("pte: idx = %d, val = %lx\n", idx, *pte);
    // 通过pte_offset_map获取pte表项
    printk("pte: pte_offset %lx\n", *(pte_offset_map(pmd, addr)));

    // 根据pte表项查找物理地址
    phys = (unsigned long)pte_pfn(*pte); // 获取pte表项对应的页框号
    phys = phys << PAGE_SHIFT; // 页框号转换为物理地址
    phys = phys | (addr & ~PAGE_MASK); // 物理地址
    printk("phys: %lx\n", phys);
    
}

void paging_test(void)
{
    printk("paging_test\n");
    
    printk("CONFIG_PGTABLE_LEVELS: %d\n", CONFIG_PGTABLE_LEVELS);
    
    printk("PAGE_SHIFT: %d\n", PAGE_SHIFT);
    printk("PAGE_SIZE: %d\n", PAGE_SIZE);
    printk("PAGE_MASK: %lx\n", PAGE_MASK);

    printk("PTRS_PER_PTE: %d\n", PTRS_PER_PTE);
    
    printk("PTRS_PER_PMD: %d\n", PTRS_PER_PMD);
    printk("PTRS_PER_PUD: %d\n", PTRS_PER_PUD);
    printk("PTRS_PER_PGD: %d\n", PTRS_PER_PGD);
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
    print_paging(tsk, 0x7fffd4a00047);

    // 给进程tsk的用户空间的指定虚拟地址上分配一个页框，并在此页框中设置一个4字节整数，然后向进程tsk发送信号SIGUSR1，让进程tsk执行信号处理函数，在信号处理函数中读取该整数，并打印出来。
    unsigned long virt = POINTER;
    // struct page *page = get_free_page(GFP_KERNEL);

    
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
