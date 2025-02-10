#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched/signal.h>
#include <asm/pgtable.h>
#include <linux/moduleparam.h>
#include <linux/uaccess.h>
#include <linux/mm.h>

unsigned long tsk_user_space_addr = 0x7fff2db60687;
module_param(tsk_user_space_addr, ulong, 0644);

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
    printk("pgd: idx = %d, val = %lx\n", idx, pgd_val(*pgd));
    // 通过pgd_offset获取pgd表项
    printk("pgd: pgd_offset %lx\n", pgd_val(*(pgd_offset(tsk->mm, addr))));
    
    // 根据pgd表项查找addr对应的pud表项：可通过pud_offset(pgd, addr)获取
    pud = pgd_page_vaddr(*pgd); // PUD对应的页的虚拟地址
    print_page((unsigned long *)pud);
    // 根据线性地址查找pud表项
    idx = pud_index(addr);
    pud = pud + idx; // pud表项
    printk("pud: idx = %d, val = %lx\n", idx, pud_val(*pud));
    // 通过pud_offset获取pud表项
    printk("pud: pud_offset %lx\n", pud_val(*(pud_offset((p4d_t*)pgd, addr))));

    // 根据pud表项查找pmd表项
    pmd = pud_page_vaddr(*pud); // PMD对应的页的虚拟地址
    print_page((unsigned long *)pmd);
    // 根据线性地址查找pmd表项
    idx = pmd_index(addr);
    pmd = pmd + idx; // pmd表项
    printk("pmd: idx = %d, val = %lx\n", idx, pmd_val(*pmd));
    // 通过pmd_offset获取pmd表项
    printk("pmd: pmd_offset %lx\n", pmd_val(*(pmd_offset(pud, addr))));

    // 根据pmd表项查找pte表项
    pte = (pte_t *)pmd_page_vaddr(*pmd); // PTE对应的页的虚拟地址
    print_page((unsigned long *)pte);
    // 根据线性地址查找pte表项
    idx = pte_index(addr);
    pte = pte + idx; // pte表项
    printk("pte: idx = %d, val = %lx\n", idx, *pte);
    // 通过pte_offset_map获取pte表项
    printk("pte: pte_offset %lx\n", pte_val(*(pte_offset_map(pmd, addr))));

    // 根据pte表项查找物理地址
    phys = (unsigned long)pte_pfn(*pte); // 获取pte表项对应的页框号
    phys = phys << PAGE_SHIFT; // 页框号转换为物理地址
    phys = phys | (addr & ~PAGE_MASK); // 物理地址
    printk("phys: %lx\n", phys);
    
}

// 判断进程地址空间的线性地址是否有效，如无效，则分配物理内存
void tsk_addr_test(struct task_struct *tsk, unsigned long user_addr)
{
    printk("tsk_addr_test: user address is %lx\n", user_addr);

    // 获取pgd
    pgd_t *pgd = pgd_offset(tsk->mm, user_addr);
    printk("pgd: %lx\n", pgd_val(*pgd));

    // 获取pud
    pud_t *pud;
    if (!pgd_present(*pgd)) {
        pud = (pud_t *)get_zeroed_page(GFP_KERNEL);
        // populate pgd
        set_pgd(pgd, __pgd(__pa(pud) | _PAGE_TABLE)); 
    }
    pud = pud_offset((p4d_t *)pgd, user_addr);
    printk("pud: %lx\n", pud_val(*pud));

    // 获取pmd
    pmd_t *pmd;
    if (!pud_present(*pud)) {
        pmd = (pmd_t *)get_zeroed_page(GFP_KERNEL);
        set_pud(pud, __pud(__pa(pmd) | _PAGE_TABLE));
    }
    pmd = pmd_offset(pud, user_addr);
    printk("pmd: %lx\n", pmd_val(*pmd));

    // 获取pte
    pte_t *pte;
    if (!pmd_present(*pmd)) {
        pte = (pte_t *)get_zeroed_page(GFP_KERNEL);
        set_pmd(pmd, __pmd(__pa(pte) | _PAGE_TABLE));
    }
    pte = pte_offset_map(pmd, user_addr);
    printk("pte: %lx\n", pte_val(*pte));

    // 映射物理内存
    if (!pte_present(*pte)) {
        struct page *page = alloc_page(GFP_KERNEL);
        if (!page) {
            printk("alloc_page failed\n");
            return;
        }
        set_pte(pte, pfn_pte((unsigned long)(page - (struct page*)0xffd4000000000000UL), (pgprot_t)_PAGE_TABLE));
        // set_pte(pte, mk_pte(page, (pgprot_t)_PAGE_TABLE));
    }
    
    printk("pte: %lx\n", pte_val(*pte));

    // 通知进程
    send_sig(SIGUSR1, tsk, 0);
}

void paging_test(void)
{
    printk("paging_test\n");
    
    printk("CONFIG_PGTABLE_LEVELS: %d\n", CONFIG_PGTABLE_LEVELS);
    printk("PAGE_SHIFT: %d\n", PAGE_SHIFT);
    printk("PMD_SHIFT: %d\n", PMD_SHIFT);
    printk("PUD_SHIFT: %d\n", PUD_SHIFT);
    printk("PGDIR_SHIFT: %d\n", PGDIR_SHIFT);

    struct task_struct *tsk = find_task_by_name("a.out");
    if (!tsk) {
        printk(KERN_ERR "find_task_by_name failed\n");
        return;
    }
    printk("find_task_by_name success\n");
    printk("pid: %d, comm: %s\n", tsk->pid, tsk->comm);
    
    tsk_addr_test(tsk, tsk_user_space_addr);

    print_paging(tsk, tsk_user_space_addr);
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
