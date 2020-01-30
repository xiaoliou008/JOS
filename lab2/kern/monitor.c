// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>

// lab2 challenge 2（本来用了page_walk，但后来发现得重写）
#include <kern/pmap.h>
#include <inc/error.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line


struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
	{ "backtrace", "Call mon_backtrace function", mon_backtrace },
	{ "showmappings", "Display physical page mappings", mon_showmap },
	{ "chperm", "Change page permission", mon_chperm },
	{ "dumpmem", "Dump the contents of a range of memory", mon_dumpmem },
};

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(commands); i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", _start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	// Your code here.
	cprintf("Stack backtrace:\n");
	uint32_t *ebp = (uint32_t *)read_ebp();
	while(ebp != NULL){			// 根据entry.S可知，%ebp为0时表示到了内核初始化之前的状态了
		uint32_t eip = *(ebp + 1);		// 获取返回地址
		cprintf("  ebp %08x  eip %08x  args", ebp, eip);
		for(int i=2;i<7;i++){			// 获取参数
			cprintf(" %08x", *(ebp + i));
		}
		cprintf("\n");
		
		struct Eipdebuginfo info;
		if(debuginfo_eip(eip, &info) < 0)
			cprintf("Error debuginfo_eip\n");
		else cprintf("         %s:%d: %.*s+%d\n", info.eip_file, info.eip_line,
				info.eip_fn_namelen, info.eip_fn_name, eip - info.eip_fn_addr);
		ebp = (uint32_t *)*ebp;			// 进入上一层函数
	}
	return 0;
}

// challenge 2 in lab2
pte_t *
pgdir_walk_c(pde_t *pgdir, const void *va, int *level)
{
	pde_t *pde = pgdir + PDX(va);
	// because of challenge 1
	if(*pde & PTE_PS){
		if(level != NULL) *level = 0;
		return (pte_t *)pde;
	}
	if(level != NULL) *level = 1;
	pte_t *pgtb = NULL;
	if((*pde & PTE_P) && (pgtb = KADDR(PTE_ADDR(*pde)))){
		return pgtb + PTX(va);
	}
	else return NULL;
}
// challenge 2-1 in lab2
int
mon_showmap(int argc, char **argv, struct Trapframe *tf)
{
	extern pde_t *kern_pgdir;
	if(argc < 2){
		cprintf("showmap: At least one argument\n");
		return -E_INVAL;
	}
	else if(argc == 2){
		cprintf("VA\t\tphysical_addr\tpermission\n");
		char *va_str = argv[1];
		if(va_str[1] == 'x') va_str += 2;	// skip "0x"
		uintptr_t va = ROUNDDOWN(strtol(va_str, NULL, 16), PGSIZE);
		pte_t *ptep = pgdir_walk_c(kern_pgdir, (void *)va, NULL);
		if(ptep == NULL || !(*ptep & PTE_P)) cprintf("0x%x\tNo mapping\t0x%x\n", va, ptep ? *ptep : 0);
		else{
			char perm_str[4] = "-r-";
			if(*ptep & PTE_U) perm_str[0] = 'u';
			if(*ptep & PTE_W) perm_str[2] = 'w';
			cprintf("0x%08x\t0x%08x\t%s\n", va, (*ptep) & (~0xFFF), perm_str);
		}
	}
	else{	// argc == 3, if more than that, just ignore
		cprintf("VA range\t\tphysical_addr\tpermission\n");
		char *va_str = argv[1];
		if(va_str[1] == 'x') va_str += 2;	// skip "0x"
		uintptr_t va_start = ROUNDDOWN(strtol(va_str, NULL, 16), PGSIZE);
		va_str = argv[2];
		if(va_str[2] == 'x') va_str += 2;	// skip "0x"
		uintptr_t va_end = ROUNDDOWN(strtol(va_str, NULL, 16), PGSIZE);
		if(va_start > va_end){				// swap
			va_start ^= va_end;
			va_end ^= va_start;
			va_start ^= va_end;
		}
		for(;va_start <= va_end;va_start += PGSIZE){
			int level = -1;
			pte_t *ptep = pgdir_walk_c(kern_pgdir, (void *)va_start, &level);
			if(!level){	// 扩展页，页大小4MB
				cprintf("debug# kern_pgdir = %p, ptep = %p\n", kern_pgdir, ptep);
				if(!(*ptep & PTE_P)) cprintf("0x%x\tNo mapping\n", va_start);
				else{
					char perm_str[4] = "-r-";
					if(*ptep & PTE_U) perm_str[0] = 'u';
					if(*ptep & PTE_W) perm_str[2] = 'w';
					cprintf("0x%08x-0x%08x\t0x%08x\t%s\n",
						va_start, va_start+0x3FFFFF, (*ptep) & (~0xFFF), perm_str);					
				}
				va_start += PTSIZE - PGSIZE;		// 加上4MB后进入下一轮循环
				continue;
			}
			if(ptep == NULL || !(*ptep & PTE_P)) cprintf("0x%x\tNo mapping\n", va_start);
			else{
				char perm_str[4] = "-r-";
				if(*ptep & PTE_U) perm_str[0] = 'u';
				if(*ptep & PTE_W) perm_str[2] = 'w';
				cprintf("0x%08x-0x%08x\t0x%08x\t%s\n",
					va_start, va_start+0xFFF, (*ptep) & (~0xFFF), perm_str);
			}
		}
	}
	return 0;
}

// challenge 2-2 in lab2
// K> chperm 0xf0000000 +u
// K> chperm 0xf0000000 -u
// K> chperm 0xf0000000 +u -w
int
mon_chperm(int argc, char **argv, struct Trapframe *tf)
{
	if(argc < 2){
		cprintf("chperm: missing virtual addresss\n");
		return -E_INVAL;
	}
	else if(argc == 2){
		cprintf("chperm :missing operand\n");
		return -E_INVAL;
	}
	else{
		char *va_str = argv[1];
		if(va_str[1] == 'x') va_str += 2;
		uintptr_t va = ROUNDDOWN(strtol(va_str, NULL, 16), PGSIZE);
		pte_t *ptep = pgdir_walk_c(kern_pgdir, (void *)va, NULL);
		if(ptep == NULL || !(*ptep & PTE_P)){
			cprintf("No mapping\n");
			return 0;
		}
		int perm[2] = {0, 0};	// perm[0]: u, perm[1]: w
		for(int i=2;i<argc;i++){
			if(!strncmp(argv[i], "+u", 2)) perm[0] = 1;
			else if(!strncmp(argv[i], "+w", 2)) perm[1] = 1;
			else if(!strncmp(argv[i], "-u", 2))	perm[0] = -1;
			else if(!strncmp(argv[i], "-w", 2)) perm[1] = -1;
		}
		// User/Supervisor
		switch(perm[0]){
		case 1:
			*ptep |= PTE_U;
			break;
		case -1:
			*ptep &= ~PTE_U;
			break;
		}
		// Read/Write
		switch(perm[1]){
		case 1:
			*ptep |= PTE_W;
			break;
		case -1:
			*ptep &= ~PTE_W;
			break;
		}
	}
	return 0;
}

// challenge 2-3 in lab2
// K>dumpmem 0xf0000000 4 -v
int
mon_dumpmem(int argc, char **argv, struct Trapframe *tf)
{
	if(argc < 3){
		cprintf("mon_dumpmem: Too few arguments (min 3)\n");
		return -E_INVAL;
	}
	else if(argc == 4 && !strncmp(argv[3], "-p", 2)){
		// 应该也可以用__attribute__((aligned(PGSIZE)))来设置对齐
		uintptr_t map_start = MMIOLIM;
		char *buf_start = (char *)map_start;
		// 下面要存储旧的映射，等结束后恢复
		pte_t *ptep = pgdir_walk(kern_pgdir, (void *)map_start, true);
		pte_t old_pte = *ptep;
//		cprintf("old_physaddr = 0x%08x\n", old_pte & ~0xfff);
//		cprintf("buf_start = 0x%p\n", buf_start);
		
		char *pa_str = argv[1];
		if(pa_str[1] == 'x') pa_str += 2;
		uintptr_t pa_base = strtol(pa_str, NULL, 16);
		pa_str = argv[2];
		if(pa_str[1] == 'x') pa_str += 2;
		int cnt = 4 * strtol(pa_str, NULL, 10);			// 10进制
		int col = 0, char_cnt = 0;
		char trans_buf[5];
		memset(trans_buf, 0, sizeof(trans_buf));
//		cprintf("debug#dumpmem start map\n");
		tlb_invalidate(kern_pgdir, (void *)map_start);	// update TLB
		*ptep = ROUNDDOWN(pa_base, PGSIZE) | PTE_P;
//		cprintf("debug#dumpmem start tla invalid\n");
		int i = pa_base - ROUNDDOWN(pa_base, PGSIZE);
//		cprintf("debug#dumpmem start read\n");
		// 按字节读取
		for(;cnt-- > 0;pa_base++, i++){
			if(!(pa_base % PGSIZE)){
				*ptep = pa_base | PTE_P;
				tlb_invalidate(kern_pgdir, (void *)map_start);		// update TLB
				i = 0;
			}
			trans_buf[char_cnt++] = buf_start[i];		// dump data by char
			if(char_cnt >= 4){
				if(col >= 5){
					cprintf("\n");
					col = 1;
				} else if(col) {
					cprintf("\t");
					col++;
				}
				else col++;
				cprintf("0x%08x", *((int *)trans_buf));	// print by double word
				char_cnt = 0;
			}
		}
		if(col < 5) cprintf("\n");
		*ptep = old_pte;	// 恢复
	}
	else{		// default: -v virtual addr
		char *va_str = argv[1];
		if(va_str[1] == 'x') va_str += 2;
		uintptr_t va_base = strtol(va_str, NULL, 16);
		pte_t *ptep = pgdir_walk_c(kern_pgdir, (void *)va_base, NULL);
		if(ptep == NULL || !(*ptep & PTE_P)){
			cprintf("No mapping at 0x%08x\n", va_base);
			return 0;
		}
		va_str = argv[2];
		if(va_str[1] == 'x') va_str += 2;
		int cnt = strtol(va_str, NULL, 10);			// 10进制
		int col = 0;
		for(;cnt-- > 0;va_base += 4){
			// 按字对齐，这样保证可能整除，+3是为了防止一个字横跨两个页
			if(!(ROUNDDOWN(va_base + 3, 4) % PGSIZE)){
				pte_t *ptep = pgdir_walk_c(kern_pgdir, (void *)va_base, NULL);
				if(ptep == NULL || !(*ptep & PTE_P)){
					cprintf("No mapping at 0x%08x\n", va_base);
					return 0;
				}
			}
			if(col >= 5){
				cprintf("\n");
				col = 1;
			} else if(col) {
				cprintf("\t");
				col++;
			}
			else col++;
			cprintf("0x%08x", *((int *)va_base));			
		}
		if(col < 5) cprintf("\n");
	}
	return 0;
}


/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < ARRAY_SIZE(commands); i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("\033[31;47mWelcome to the JOS kernel monitor!\033[0m\n");
	cprintf("\033[34;43mType 'help' for a list of commands.\033[0m\n");
	
/*	// 为了测试问题4
	unsigned int i = 0x00646c72;
	cprintf("\033[0;32;40m H%x Wo%s", 57616, &i);
*/

	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
