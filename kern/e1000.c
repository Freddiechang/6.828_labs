#include <kern/e1000.h>
#include <kern/pmap.h>

// LAB 6: Your driver code here
static struct eth_pack_buffer * list_tx_buffer[NTXDESC];

void transmit_init(volatile char * dev_mmiobase)
{
    struct PageInfo * pp;
    uint32_t addr = NETVA;
    uint32_t i;
    int r;
    pte_t * pte;

    // alloc and map a page for tx_desc list
    if( (pp = page_alloc(1)) == NULL)
    {
        panic("e1000.c : transmit_init: OOM\n");
    }
    ((uint32_t *)dev_mmiobase)[E1000_TDBAL / 4] = page2pa(pp);
    if( (r = page_insert(kern_pgdir, pp, (void *)addr, PTE_P | PTE_W)) < 0)
    {
        cprintf("transmit_init: %e\n", r);
        panic("transmit_init");
    }
    addr += PGSIZE;
     // dev init regs
    ((uint32_t *)dev_mmiobase)[E1000_TDBAH / 4] = 0x0;
    ((uint32_t *)dev_mmiobase)[E1000_TDLEN / 4] = sizeof(struct tx_desc) * NTXDESC;
    ((uint32_t *)dev_mmiobase)[E1000_TDH / 4] = 0x0;
    ((uint32_t *)dev_mmiobase)[E1000_TDT / 4] = 0x0;
    ((uint32_t *)dev_mmiobase)[E1000_TCTL / 4] = E1000_TCTL_EN | E1000_TCTL_PSP | E1000_TCTL_CT | E1000_TCTL_COLD;
    ((uint32_t *)dev_mmiobase)[E1000_TIPG / 4] = 10 + (4 << 10) + (6 << 20);

    // buffer alloc for every tx desc
    for(i = 0; i < NTXDESC; i++)
    {
        if(i % 2 == 0)
        {
            if( (pp = page_alloc(1)) == NULL)
            {
                panic("e1000.c : transmit_init 2: OOM\n");
            }
            if( (r = page_insert(kern_pgdir, pp, (void *)addr, PTE_P | PTE_W)) < 0)
            {
                cprintf("transmit_init: %e\n", r);
                panic("transmit_init 2");
            }
            addr += PGSIZE;
        }
        list_tx_buffer[i] = (struct eth_pack_buffer *)(addr + (i % 2) * sizeof(struct eth_pack_buffer));
    }
}