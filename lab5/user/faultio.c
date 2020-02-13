// test user-level fault handler -- alloc pages to fix faults

#include <inc/lib.h>
#include <inc/x86.h>

void
umain(int argc, char **argv)
{
        int x, r;
	int nsecs = 1;
	int secno = 0;
	int diskno = 1;
/*
	if (read_eflags() & FL_IOPL_3)
		cprintf("eflags wrong\n");
*/

	if ((r = read_eflags()) & FL_IOPL_3)
		cprintf("eflags wrong\n", r);
/*
	x = r << 4;
	while(x > 1) x >>= 1;
//	x -= 5;
//	x -= 5;
	cprintf("eflags = %08x\n", x);
*/
	// this outb to select disk 1 should result in a general protection
	// fault, because user-level code shouldn't be able to use the io space.

	outb(0x1F6, 0xE0 | (1<<4));

        cprintf("%s: made it here --- bug\n");
}
