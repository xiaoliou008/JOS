/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>

#define CHALLENGE1

###################################################################
# exceptions/interrupts
###################################################################

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */
#define TRAPHANDLER(name, num)						\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushl $(num);							\
	jmp _alltraps

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)					\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps
	
	
// lab 3 challenge 1
#define TRAPENTRY(name, num, istrap, dpl)		\
	.data;							\
	.int name;			/* addr */	\
	.byte 0;			/* gd_args gd_rsv1 */	\
	.byte (0x80 | (dpl << 5) | istrap);	/* gd_type gd_dpl */ 	\
	.word 0;			/* gd_off_31_16 */ 	\
	.text;							\
	.ifndef name;					\
	.globl name;					\
	.type name, @function;			\
	.align 2;						\
	name:							\
	pushl $(num);							\
	jmp _alltraps;					\
	.endif

#define TRAPENTRY_NOEC(name, num, istrap, dpl)		\
	.data;							\
	.int name;			/* addr */	\
	.byte 0;			/* gd_args gd_rsv1 */	\
	.byte (0x80 | (dpl << 5) | istrap);	/* gd_type gd_dpl */ 	\
	.word 0;			/* gd_off_31_16 */ 	\
	.text;							\
	.ifndef name;					\
	.globl name;					\
	.type name, @function;			\
	.align 2;						\
	name:							\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps;					\
	.endif
	

.text

/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */
#ifndef CHALLENGE1
	TRAPHANDLER_NOEC(trap0, 0);
	TRAPHANDLER_NOEC(trap1, 1);
	TRAPHANDLER_NOEC(trap2, 2);
	TRAPHANDLER_NOEC(trap3, 3);
	TRAPHANDLER_NOEC(trap4, 4);
	TRAPHANDLER_NOEC(trap5, 5);
	TRAPHANDLER_NOEC(trap6, 6);
	TRAPHANDLER_NOEC(trap7, 7);
	TRAPHANDLER(trap8, 8);
	//TRAPHANDLER_NOEC(trap9, 9);
	TRAPHANDLER(trap10, 10);
	TRAPHANDLER(trap11, 11);
	TRAPHANDLER(trap12, 12);
	TRAPHANDLER(trap13, 13);
	TRAPHANDLER(trap14, 14);
	TRAPHANDLER_NOEC(trap16, 16);
	TRAPHANDLER_NOEC(trap17, 17);
	TRAPHANDLER_NOEC(trap18, 18);
	TRAPHANDLER_NOEC(trap19, 19);
	TRAPHANDLER_NOEC(trap48, 48);
	TRAPHANDLER_NOEC(trap_default, 500);
#else
.data
	.align 2;
	.globl idt;
idt:
	TRAPENTRY_NOEC(trap0, 0, STS_IG32, 0);
	TRAPENTRY_NOEC(trap1, 1, STS_TG32, 0);
	TRAPENTRY_NOEC(trap2, 2, STS_TG32, 0);
	TRAPENTRY_NOEC(trap3, 3, STS_TG32, 3);
	TRAPENTRY_NOEC(trap4, 4, STS_TG32, 3);
	TRAPENTRY_NOEC(trap5, 5, STS_IG32, 3);
	TRAPENTRY_NOEC(trap6, 6, STS_IG32, 0);
	TRAPENTRY_NOEC(trap7, 7, STS_IG32, 0);
	TRAPENTRY(trap8, 8, STS_IG32, 0);
	TRAPENTRY_NOEC(trap_default, 500, STS_IG32, 0);
/*	TRAPENTRY_NOEC(trap_default, 9, STS_IG32, 0);	*/
	TRAPENTRY(trap10, 10, STS_IG32, 0);
	TRAPENTRY(trap11, 11, STS_IG32, 0);
	TRAPENTRY(trap12, 12, STS_IG32, 0);
	TRAPENTRY(trap13, 13, STS_IG32, 0);
	TRAPENTRY(trap14, 14, STS_IG32, 0);
	TRAPENTRY_NOEC(trap_default, 15, STS_IG32, 0);
	TRAPENTRY_NOEC(trap16, 16, STS_IG32, 0);
	TRAPENTRY_NOEC(trap17, 17, STS_TG32, 3);
	TRAPENTRY_NOEC(trap18, 18, STS_TG32, 3);
	TRAPENTRY_NOEC(trap19, 19, STS_TG32, 3);
	.rept 28;		/* int 20 ~ int 47 */
	TRAPENTRY_NOEC(trap_default, 0, STS_TG32, 3);
	.endr;
	TRAPENTRY_NOEC(trap48, 48, STS_TG32, 3);
	.rept 207;		/* int 49 ~ int 255 */
	TRAPENTRY_NOEC(trap_default, 0, STS_TG32, 3);
	.endr;
	
#endif	/* CHALLENGE1 */


/*
 * Lab 3: Your code here for _alltraps
 */
_alltraps:
	pushl %ds;
	pushl %es;
	pushal;
	movw $GD_KD, %ax;
	movw %ax, %ds;
	movw %ax, %es;
	pushl %esp;
	call trap;

