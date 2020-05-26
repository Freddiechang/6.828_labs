#include "ns.h"

extern union Nsipc nsipcbuf;

void
input(envid_t ns_envid)
{
	binaryname = "ns_input";

	// LAB 6: Your code here:
	// 	- read a packet from the device driver
	//	- send it to the network server
	// Hint: When you IPC a page to the network server, it will be
	// reading from it for a while, so don't immediately receive
	// another packet in to the same physical page.
	int i, r, perm;
	struct jif_pkt *pkt;
	for(i = 0; i < NRECVPAGE; i++)
	{
		if( (r = sys_page_alloc(thisenv->env_id, (void *)INPUT_HELPER_VA + i * PGSIZE, 
		    PTE_P | PTE_U | PTE_W)) < 0)
			{
				cprintf("ns input.c: page alloc\n: %e", r);
			    panic("ns input.c: page alloc failed");
			}
	}
	pkt = &nsipcbuf.pkt;
	// the corresponding page frame is an COW page after fork, we must allocate 
    // our own address space before entering kernel mode (sys_nic_recv)
	//memmove(&nsipcbuf, "0000", 1);
	if( (r = sys_page_alloc(thisenv->env_id, (void *)pkt, 
		    PTE_P | PTE_U | PTE_W)) < 0)
			{
				cprintf("ns input.c: page alloc 2\n: %e", r);
			    panic("ns input.c: page alloc failed");
			}
	i = 0;
	while(1)
	{
		while( (r = sys_net_recv((void *)pkt->jp_data)) == -E_RX_BUSY)
		{
			sys_yield();
		}
		if (r < 0)
            panic("%s: sys_net_recv: %e", binaryname, r);
		pkt->jp_len = r;
		memmove((void *)INPUT_HELPER_VA + i * PGSIZE, pkt, sizeof(union Nsipc));
		ipc_send(ns_envid, NSREQ_INPUT, (void *)INPUT_HELPER_VA + i * PGSIZE, PTE_P | PTE_U);
		i = (i + 1) % NRECVPAGE;
	}
}
