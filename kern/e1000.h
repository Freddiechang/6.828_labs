#ifndef JOS_KERN_E1000_H
#define JOS_KERN_E1000_H
#endif  // SOL >= 6
#include <kern/pmap.h>

extern volatile char *e1000;

#define MAX_TXPACK_SIZE 1518
#define MAX_RXPACK_SIZE 2048
#define E1000_DEV_ID_82540EM  0x100E

/* Register Set. (82543, 82544)
 *
 * Registers are defined to be 32 bits and  should be accessed as 32 bit values.
 * These registers are physically located on the NIC, but are mapped into the
 * host memory address space.
 *
 * RW - register is both readable and writable
 * RO - register is read only
 * WO - register is write only
 * R/clr - register is read only and is cleared when read
 * A - register array
 */
#define E1000_STATUS   0x00008  /* Device Status - RO */
#define E1000_TDBAL    0x03800  /* TX Descriptor Base Address Low - RW */
#define E1000_TDBAH    0x03804  /* TX Descriptor Base Address High - RW */
#define E1000_TDLEN    0x03808  /* TX Descriptor Length - RW */
#define E1000_TDH      0x03810  /* TX Descriptor Head - RW */
#define E1000_TDT      0x03818  /* TX Descripotr Tail - RW */
#define E1000_RAL      0x05400  /* Receive Address Low - RW Array */
#define E1000_RAH      0x05404  /* Receive Address High - RW Array */
#define E1000_MTA      0x05200  /* Multicast Table Array - RW Array */
#define E1000_RDBAL    0x02800  /* RX Descriptor Base Address Low (1) - RW */
#define E1000_RDBAH    0x02804  /* RX Descriptor Base Address High (1) - RW */
#define E1000_RDLEN    0x02808  /* RX Descriptor Length (1) - RW */
#define E1000_RDH      0x02810  /* RX Descriptor Head (1) - RW */
#define E1000_RDT      0x02818  /* RX Descriptor Tail (1) - RW */

#define E1000_TCTL     0x00400  /* TX Control - RW */
#define E1000_TCTL_EN     0x00000002    /* enable tx */
#define E1000_TCTL_PSP    0x00000008    /* pad short packets */
#define E1000_TCTL_CT     0x000000f0    /* collision threshold */
#define E1000_TCTL_COLD   0x00040000    /* collision distance */

#define E1000_TIPG     0x00410  /* TX Inter-packet gap -RW */

/* Transmit Descriptor bit definitions */
#define E1000_TXD_CMD_EOP    0x1 /* End of Packet */
#define E1000_TXD_CMD_RS     0x8        /* Report Status */
#define E1000_TXD_STAT_DD    0x00000001 /* Descriptor Done */
#define E1000_TXD_STAT_EC    0x00000002 /* Excess Collisions */
#define E1000_TXD_STAT_LC    0x00000004 /* Late Collisions */

/* Receive Descriptor bit definitions */
#define E1000_RXD_STAT_DD       0x01    /* Descriptor Done */
#define E1000_RXD_STAT_EOP      0x02    /* End of Packet */

#define E1000_IMS      0x000D0  /* Interrupt Mask Set - RW */
#define E1000_ICR      0x000C0  /* Interrupt Cause Read - R/clr */

#define E1000_RCTL             0x00100  /* RX Control - RW */
#define E1000_RCTL_EN          0x00000002    /* enable */
#define E1000_RCTL_LPE         0x00000020    /* long packet enable */
#define E1000_RCTL_LBM_NO      0x00000000    /* no loopback mode */
#define E1000_RCTL_RDMTS_HALF  0x00000000    /* rx desc min threshold size */
#define E1000_RCTL_RDMTS_QUAT  0x00000100    /* rx desc min threshold size */
#define E1000_RCTL_RDMTS_EIGTH 0x00000200    /* rx desc min threshold size */
#define E1000_RCTL_MO_0        0x00000000    /* multicast offset 11:0 */
#define E1000_RCTL_BAM         0x00008000    /* broadcast enable */
#define E1000_RCTL_SZ_2048     0x00000000    /* rx buffer size 2048 */
#define E1000_RCTL_BSEX        0x02000000    /* Buffer size extension */
#define E1000_RCTL_SECRC       0x04000000    /* Strip Ethernet CRC */

#define E1000_ICR_TXDW          0x00000001 /* Transmit desc written back */
#define E1000_IMS_TXDW      E1000_ICR_TXDW      /* Transmit desc written back */

#define NTXDESC 16
#define NRXDESC 128
struct tx_desc
{
	uint64_t addr;
	uint16_t length;
	uint8_t cso;
	uint8_t cmd;
	uint8_t status;
	uint8_t css;
	uint16_t special;
};

struct rx_desc
{
	uint64_t addr;
	uint16_t length;
	uint16_t pack_checksum;
	uint8_t status;
	uint8_t errors;
	uint16_t special;
};

struct tx_pack_buffer
{
	char buffer[MAX_TXPACK_SIZE];
};

struct rx_pack_buffer
{
	char buffer[MAX_RXPACK_SIZE];
};

void transmit_init(volatile char * dev_mmiobase);
int transmit_pack(volatile char * dev_mmiobase, const void * buf, size_t size);
void receive_init(volatile char * dev_mmiobase);
int receive_pack(volatile char * dev_mmiobase, void * buf);