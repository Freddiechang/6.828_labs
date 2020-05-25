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
	while(1)
	{
		if( (r = ipc_recv(&ns_envid, (void *)&nsipcbuf, &perm)) < 0)
		{
			cprintf("ns output.c: ipc_recv\n: %e", r);
			panic("ns output.c: ipc recv failed");
		}
		if(r != NSREQ_OUTPUT)
		{
			panic("wrong message from ns");
		}
		pkt = &nsipcbuf.pkt;
		while( (r = sys_net_try_send((void *)pkt->jp_data, pkt->jp_len)) == E_TX_BUSY)
		{
			sys_yield();
		}
	}
}
