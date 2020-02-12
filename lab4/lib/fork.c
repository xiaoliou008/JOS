// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

#define LAB4_CHALLENGE6
#define LAB4_CHALLENGE7

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
	if((err & FEC_WR) == 0 || (uvpt[PGNUM(addr)] & PTE_COW) == 0){
		cprintf("debug#fault panic, addr = 0x%08x\n", addr);
		panic("pgfault: missing FEC_WR or PTE_COW\n");
	}

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.
	sys_page_alloc(0, (void *)PFTEMP, PTE_W);
	memcpy((void *)PFTEMP, (void *)ROUNDDOWN((uintptr_t)addr, PGSIZE), PGSIZE);
	sys_page_map(0, (void *)PFTEMP, 0, (void *)ROUNDDOWN((uintptr_t)addr, PGSIZE), PTE_W);
	sys_page_unmap(0, (void *)PFTEMP);
	return;

	panic("pgfault not implemented");
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int r;

	// LAB 4: Your code here.
	//panic("duppage not implemented");
	int res = 0;
	if(uvpt[pn] & PTE_W || uvpt[pn] & PTE_COW){
		res = sys_page_map(0, (void *)(pn * PGSIZE), envid, (void *)(pn * PGSIZE), PTE_COW);
		// 这两句之间可能会出现缺页中断，导致本进程页表修改和父子进程的内容不一致。
		res = sys_page_map(0, (void *)(pn * PGSIZE), 0, (void *)(pn * PGSIZE), PTE_COW);
	} else if(uvpt[pn] & PTE_P){
		res = sys_page_map(0, (void *)(pn * PGSIZE), envid, (void *)(pn * PGSIZE), 0);	
	}
	if(res < 0) panic("duppage: failed\n");
	return res;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
	set_pgfault_handler(pgfault);
	envid_t pid = sys_exofork();
	if(pid == 0){		// child
		thisenv = envs + ENVX(sys_getenvid());
		return pid;
	}
#ifdef LAB4_CHALLENGE7
	sys_batch_page_map(pid, 0, PGNUM(USTACKTOP));
#else
	for(unsigned pn = 0; pn < PGNUM(USTACKTOP); pn++){
		if((uvpd[pn>>10] & PTE_P) == 0){
			pn += 1023;		// later pn++
			continue;
		}
		if(uvpt[pn] & PTE_P)
			duppage(pid, pn);
	}
#endif
	int res = 0;
	if((res = sys_page_alloc(pid, (void *)(UXSTACKTOP - PGSIZE), PTE_W)) < 0)
		cprintf("fork: sys_page_alloc %e\n", res);
//	BUG: if((res = sys_env_set_pgfault_upcall(pid, pgfault)) < 0)
	extern void _pgfault_upcall(void);
	if((res = sys_env_set_pgfault_upcall(pid, _pgfault_upcall)) < 0)
		cprintf("fork: sys_env_set_pgfault_upcall %e\n", res);
	sys_env_set_status(pid, ENV_RUNNABLE);
	return pid;
	
	panic("fork not implemented");
}

#ifdef LAB4_CHALLENGE6
// Challenge 6
static int
sduppage(envid_t envid, unsigned pn)
{
	int res = 0;
	if(uvpt[pn] & PTE_COW){
		res = sys_page_map(0, (void *)(pn * PGSIZE), envid, (void *)(pn * PGSIZE), PTE_COW);
		res = sys_page_map(0, (void *)(pn * PGSIZE), 0, (void *)(pn * PGSIZE), PTE_COW);
	} else if(uvpt[pn] & PTE_W){
		res = sys_page_map(0, (void *)(pn * PGSIZE), envid, (void *)(pn * PGSIZE), PTE_W);
	} 
	else if(uvpt[pn] & PTE_P){
		res = sys_page_map(0, (void *)(pn * PGSIZE), envid, (void *)(pn * PGSIZE), 0);	
	}
	if(res < 0) panic("duppage: failed\n");
	return res;
}

int
sfork(void)
{
	set_pgfault_handler(pgfault);
	envid_t pid = sys_exofork();
	if(pid == 0){
		*((struct Env *)(USTACKTOP - 2*PGSIZE)) = *(envs + ENVX(sys_getenvid()));
		((struct Env *)(USTACKTOP - 2*PGSIZE))->env_type = ENV_TYPE_SHARED;
		return pid;		// child
	}
	// parent
	for(unsigned pn = 0; pn < PGNUM(USTACKTOP) - 2; pn++){
		if((uvpd[pn>>10] & PTE_P) == 0){
			pn += 1023;		// later pn++
			continue;
		}
		if(uvpt[pn] & PTE_P)
			sduppage(pid, pn);	// shared memory duppage
	}
	// map page below user stack
	sys_page_alloc(0, (void *)(USTACKTOP - 2*PGSIZE), PTE_W);	// map new page
	*((struct Env *)(USTACKTOP - 2*PGSIZE)) = *thisenv;
	thisenv = (struct Env *)(USTACKTOP - 2*PGSIZE);	// store dup Env struct for thisenv
	((struct Env *)(USTACKTOP - 2*PGSIZE))->env_type = ENV_TYPE_SHARED;
	sys_page_alloc(pid, (void *)(USTACKTOP - 2*PGSIZE), PTE_W);	// map new page for child
	// map page on user stack
	duppage(pid, PGNUM(USTACKTOP) - 1);			// normal duppage
	// map uxstack
	int res = 0;
	if((res = sys_page_alloc(pid, (void *)(UXSTACKTOP - PGSIZE), PTE_W)) < 0)
		cprintf("fork: sys_page_alloc %e\n", res);
	extern void _pgfault_upcall(void);
	if((res = sys_env_set_pgfault_upcall(pid, _pgfault_upcall)) < 0)
		cprintf("fork: sys_env_set_pgfault_upcall %e\n", res);
	sys_env_set_status(pid, ENV_RUNNABLE);
	return pid;
	
//	panic("sfork not implemented");
//	return -E_INVAL;
}
#endif
