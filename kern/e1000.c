#include <inc/string.h>
#include <kern/e1000.h>
#include <kern/pmap.h>
#include <inc/error.h>

// LAB 6: Your driver code here
static struct eth_pack_buffer list_tx_buffer[NTXDESC];
static struct tx_desc list_tx_desc[NTXDESC];

void transmit_init(volatile char * dev_mmiobase)
{
    uint32_t i;
    int r;
    pte_t * pte;

     // dev init regs
    ((uint32_t *)dev_mmiobase)[E1000_TDBAL / 4] = PADDR(list_tx_desc);
    ((uint32_t *)dev_mmiobase)[E1000_TDBAH / 4] = 0x0;
    ((uint32_t *)dev_mmiobase)[E1000_TDLEN / 4] = sizeof(struct tx_desc) * NTXDESC;
    ((uint32_t *)dev_mmiobase)[E1000_TDH / 4] = 0x0;
    ((uint32_t *)dev_mmiobase)[E1000_TDT / 4] = 0x0;
    ((uint32_t *)dev_mmiobase)[E1000_TCTL / 4] = E1000_TCTL_EN | E1000_TCTL_PSP | E1000_TCTL_CT | E1000_TCTL_COLD;
    ((uint32_t *)dev_mmiobase)[E1000_TIPG / 4] = 10 + (4 << 10) + (6 << 20);
    for(i = 0; i < NTXDESC; i++)
    {
        list_tx_desc[i].addr = PADDR(list_tx_buffer[i].buffer);
        list_tx_desc[i].status = E1000_TXD_STAT_DD;
    }
    
}

int transmit_pack(volatile char * dev_mmiobase, const void * buf, size_t size)
{
    // check size < max_pack_size, maybe in syscall interface

    // look for available desc
    static uint16_t pack_count = 0;
    if(list_tx_desc[pack_count].addr != (uint32_t)NULL && 
        (list_tx_desc[pack_count].status & E1000_TXD_STAT_DD) == 0)
    {
        return -E_TX_BUSY;
    }
    list_tx_desc[pack_count].status &= ~E1000_TXD_STAT_DD;
    memmove((void *)list_tx_buffer[pack_count].buffer, buf, size);
    list_tx_desc[pack_count].length = size;
    list_tx_desc[pack_count].cmd = E1000_TXD_CMD_RS | E1000_TXD_CMD_EOP;
    pack_count = (pack_count + 1) % NTXDESC;
    ((uint32_t *)dev_mmiobase)[E1000_TDT / 4] = pack_count;
    return 0;
}