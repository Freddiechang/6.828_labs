#include <inc/string.h>
#include <kern/e1000.h>
#include <kern/pmap.h>
#include <inc/error.h>

// LAB 6: Your driver code here
static struct tx_pack_buffer list_tx_buffer[NTXDESC];
static struct tx_desc list_tx_desc[NTXDESC];
static struct rx_pack_buffer list_rx_buffer[NRXDESC];
static struct rx_desc list_rx_desc[NRXDESC];

void transmit_init(volatile char * dev_mmiobase)
{
    uint32_t i;

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
    if( (list_tx_desc[pack_count].status & E1000_TXD_STAT_DD) == 0)
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


void receive_init(volatile char * dev_mmiobase)
{
    uint32_t i;

     // dev init regs
    ((uint32_t *)dev_mmiobase)[E1000_RAL / 4] = 0x12005452;
    ((uint32_t *)dev_mmiobase)[E1000_RAH / 4] = 0x80005634;
    for(i = 0; i < 512 / sizeof(uint32_t); i++)
    {
        ((uint32_t *)dev_mmiobase)[E1000_MTA / 4 + i] = 0x0;
    }
    ((uint32_t *)dev_mmiobase)[E1000_IMS / 4] = 0x0;
    ((uint32_t *)dev_mmiobase)[E1000_RDBAL / 4] = PADDR(list_rx_desc);
    ((uint32_t *)dev_mmiobase)[E1000_RDBAH / 4] = 0x0;
    ((uint32_t *)dev_mmiobase)[E1000_RDLEN / 4] = sizeof(struct rx_desc) * NRXDESC;
    for(i = 0; i < NRXDESC; i++)
    {
        list_rx_desc[i].addr = PADDR(list_rx_buffer[i].buffer);
        list_rx_desc[i].status = 0x0;
    }
    ((uint32_t *)dev_mmiobase)[E1000_RDH / 4] = 0x0;
    ((uint32_t *)dev_mmiobase)[E1000_RDT / 4] = NRXDESC - 1;
    ((uint32_t *)dev_mmiobase)[E1000_RCTL / 4] = E1000_RCTL_EN | E1000_RCTL_RDMTS_EIGTH
        | E1000_RCTL_SECRC;
}

int receive_pack(volatile char * dev_mmiobase, void * buf)
{
    static uint16_t pack_count = NRXDESC - 1;
    pack_count = (pack_count + 1) % NRXDESC;
    if( (list_rx_desc[pack_count].status & E1000_RXD_STAT_DD) == 0)
    {
        return -E_RX_BUSY;
    }
    memmove(buf, (void *)list_rx_buffer[pack_count].buffer, list_rx_desc[pack_count].length);
    list_tx_desc[pack_count].status &= ~(E1000_TXD_STAT_DD | E1000_RXD_STAT_EOP);
    ((uint32_t *)dev_mmiobase)[E1000_RDT / 4] = pack_count;
    return list_rx_desc[pack_count].length;
}