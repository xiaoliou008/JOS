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
	sys_page_alloc(sys_getenvid(), &nsipcbuf, PTE_P | PTE_U | PTE_W);
	sys_yield();	// give up CPU to let ns read the data
	while(1){
		sys_yield();
		size_t sz = 0;
		while(sys_receive_packet(nsipcbuf.pkt.jp_data, &sz));
		nsipcbuf.pkt.jp_len = sz;
		// loop forever if data not received 
		while(sys_ipc_try_send(ns_envid, NSREQ_INPUT, &nsipcbuf, PTE_P | PTE_U | PTE_W) < 0);
		// loop forever if network environment doesn't response
		sys_yield();	// give up CPU to let ns read the data
	}	
}
