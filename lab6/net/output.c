#include "ns.h"

extern union Nsipc nsipcbuf;

void
output(envid_t ns_envid)
{
	binaryname = "ns_output";

	// LAB 6: Your code here:
	// 	- read a packet from the network server
	//	- send the packet to the device driver
	uint32_t req, whom;
	int perm, r;

	while (1) {
		perm = 0;
		req = ipc_recv((int32_t *) &whom, &nsipcbuf, &perm);
		
		// All requests must contain an argument page
		if (!(perm & PTE_P)) {
			cprintf("Invalid request from %08x: no argument page\n",
				whom);
			continue; // just leave it hanging...
		}

		switch(req){
		case NSREQ_OUTPUT:
			if((envid_t)whom == ns_envid)
				while(sys_transmit_packet(nsipcbuf.pkt.jp_data, nsipcbuf.pkt.jp_len));
			// loop forever if sys_transmit_packet returns nonzero
			break;
		default:
			cprintf("Invalid request code %d from %08x\n", req, whom);
			break;
		}
	}
}
