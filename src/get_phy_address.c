#include <linux/vmalloc.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/init_task.h>

asmlinkage int get_phy_address(unsigned long *vir_addrs, int vir_addr_len,
                               unsigned long *phy_addrs, int phy_addr_len)
{
    int i;
    struct page *_page;
    pgd_t *pgd;
    pud_t *pud;
    pmd_t *pmd;
    pte_t *pte;
    void *laddr;

    unsigned long address;
    unsigned long *vir_address = (unsigned long *)vmalloc(vir_addr_len * sizeof(unsigned long));
    unsigned long *phy_address = (unsigned long *)vmalloc(phy_addr_len * sizeof(unsigned long));

    if ((vir_addr_len > phy_addr_len) ||
        (vir_address == NULL || phy_address == NULL))
        return -1;
    
    if (copy_from_user(vir_address, vir_addrs, vir_addr_len * sizeof(unsigned long)) != 0) {
        printk("Copy from user error!\n");
        return -2;
    }

    printk("pid %d\n", current->pid);
    for (i = 0; i < vir_addr_len; ++i) {
        address = vir_address[i];

        pgd = pgd_offset(current->mm, address);
        if (pgd_none(*pgd) || unlikely(pgd_bad(*pgd))) 
            return -3;

        pud = pud_offset(pgd, address);
        if (pud_none(*pud) || unlikely(pud_bad(*pud)))
            return -4;

        pmd = pmd_offset(pud, address);
        if (pmd_none(*pmd) || unlikely(pmd_bad(*pmd)))
            return -5;

        pte = pte_offset_kernel(pmd, address);
        if (!pte)
            return -6;

        _page = pte_page(*pte);
        laddr = page_address(_page);
        phy_address[i] = virt_to_phys(laddr) | (address & ~PAGE_MASK);
        
        printk("thread_pid %d: vir_address %lx ---> phy_address %lx", current->pid, address, phy_address[i]);
    }

    if (copy_to_user(phy_addrs, phy_address, phy_addr_len * sizeof(unsigned long)) != 0) {
        printk("copy_to_user function error!\n");
        return -7;
    }
    return 0;
}