#include "ns.h"

extern union Nsipc nsipcbuf;

void
output(envid_t ns_envid)
{
	binaryname = "ns_output";

	// LAB 6: Your code here:
	// 	- read a packet from the network server
	//	- send the packet to the device driver
	int r, perm;
	struct jif_pkt *pkt;
	if( (r = sys_page_alloc(thisenv->env_id, (void *)UTEMP, PTE_U|PTE_W|PTE_P)) < 0)
	{
		cprintf("ns output.c: %e\n", r);
		panic("ns output.c: page alloc failed");
	}
	while(1)
	{
		if( (r = ipc_recv(&ns_envid, (void *)UTEMP, &perm)) < 0)
		{
			cprintf("ns output.c: ipc_recv\n: %e", r);
			panic("ns output.c: ipc recv failed");
		}
		if(r != NSREQ_OUTPUT)
		{
			panic("wrong message from ns");
		}
		pkt = (struct jif_pkt *) UTEMP;
		while( (r = sys_net_try_send((void *)pkt->jp_data, pkt->jp_len)) == E_TX_BUSY)
		{
			sys_yield();
		}
	}
}
