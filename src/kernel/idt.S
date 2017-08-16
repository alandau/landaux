#include <arch.h>
#include <errno.h>
#include <sched.h>

/* Every entry is 8 bytes long */

/* Exception without error code */
/* Subtract 128 to make the push instruction always 2 byte long */
#define EX(x)			\
	push $0;		\
	push $(x-128);		\
	jmp vec_common;		\
	.align 16;

/* Exception with error code */
#define EXE(x)			\
	push $(x-128);		\
	jmp vec_common;		\
	.align 16;

/* For non-exceptions */
#define VEC(x) EX(x)

.align 16
.global vector_entry_points_start
vector_entry_points_start:
/* Hardware exceptions */
EX(0)	EX(1)	EX(2)	EX(3)	EX(4)	EX(5)	EX(6)	EX(7)
EXE(8)	EX(9)	EXE(10)	EXE(11)	EXE(12)	EXE(13)	EXE(14)	EX(15)
EX(16)	EXE(17)	EX(18)	EX(19)	EX(20)	EX(21)	EX(22)	EX(23)
EX(24)	EX(25)	EX(26)	EX(27)	EX(28)	EX(29)	EX(30)	EX(31)
/* Other vectors */
VEC(0x20)	VEC(0x21)	VEC(0x22)	VEC(0x23)	VEC(0x24)	VEC(0x25)	VEC(0x26)	VEC(0x27)
VEC(0x28)	VEC(0x29)	VEC(0x2a)	VEC(0x2b)	VEC(0x2c)	VEC(0x2d)	VEC(0x2e)	VEC(0x2f)
VEC(0x30)	VEC(0x31)	VEC(0x32)	VEC(0x33)	VEC(0x34)	VEC(0x35)	VEC(0x36)	VEC(0x37)
VEC(0x38)	VEC(0x39)	VEC(0x3a)	VEC(0x3b)	VEC(0x3c)	VEC(0x3d)	VEC(0x3e)	VEC(0x3f)
VEC(0x40)	VEC(0x41)	VEC(0x42)	VEC(0x43)	VEC(0x44)	VEC(0x45)	VEC(0x46)	VEC(0x47)
VEC(0x48)	VEC(0x49)	VEC(0x4a)	VEC(0x4b)	VEC(0x4c)	VEC(0x4d)	VEC(0x4e)	VEC(0x4f)
VEC(0x50)	VEC(0x51)	VEC(0x52)	VEC(0x53)	VEC(0x54)	VEC(0x55)	VEC(0x56)	VEC(0x57)
VEC(0x58)	VEC(0x59)	VEC(0x5a)	VEC(0x5b)	VEC(0x5c)	VEC(0x5d)	VEC(0x5e)	VEC(0x5f)
VEC(0x60)	VEC(0x61)	VEC(0x62)	VEC(0x63)	VEC(0x64)	VEC(0x65)	VEC(0x66)	VEC(0x67)
VEC(0x68)	VEC(0x69)	VEC(0x6a)	VEC(0x6b)	VEC(0x6c)	VEC(0x6d)	VEC(0x6e)	VEC(0x6f)
VEC(0x70)	VEC(0x71)	VEC(0x72)	VEC(0x73)	VEC(0x74)	VEC(0x75)	VEC(0x76)	VEC(0x77)
VEC(0x78)	VEC(0x79)	VEC(0x7a)	VEC(0x7b)	VEC(0x7c)	VEC(0x7d)	VEC(0x7e)	VEC(0x7f)
VEC(0x80)	VEC(0x81)	VEC(0x82)	VEC(0x83)	VEC(0x84)	VEC(0x85)	VEC(0x86)	VEC(0x87)
VEC(0x88)	VEC(0x89)	VEC(0x8a)	VEC(0x8b)	VEC(0x8c)	VEC(0x8d)	VEC(0x8e)	VEC(0x8f)
VEC(0x90)	VEC(0x91)	VEC(0x92)	VEC(0x93)	VEC(0x94)	VEC(0x95)	VEC(0x96)	VEC(0x97)
VEC(0x98)	VEC(0x99)	VEC(0x9a)	VEC(0x9b)	VEC(0x9c)	VEC(0x9d)	VEC(0x9e)	VEC(0x9f)
VEC(0xa0)	VEC(0xa1)	VEC(0xa2)	VEC(0xa3)	VEC(0xa4)	VEC(0xa5)	VEC(0xa6)	VEC(0xa7)
VEC(0xa8)	VEC(0xa9)	VEC(0xaa)	VEC(0xab)	VEC(0xac)	VEC(0xad)	VEC(0xae)	VEC(0xaf)
VEC(0xb0)	VEC(0xb1)	VEC(0xb2)	VEC(0xb3)	VEC(0xb4)	VEC(0xb5)	VEC(0xb6)	VEC(0xb7)
VEC(0xb8)	VEC(0xb9)	VEC(0xba)	VEC(0xbb)	VEC(0xbc)	VEC(0xbd)	VEC(0xbe)	VEC(0xbf)
VEC(0xc0)	VEC(0xc1)	VEC(0xc2)	VEC(0xc3)	VEC(0xc4)	VEC(0xc5)	VEC(0xc6)	VEC(0xc7)
VEC(0xc8)	VEC(0xc9)	VEC(0xca)	VEC(0xcb)	VEC(0xcc)	VEC(0xcd)	VEC(0xce)	VEC(0xcf)
VEC(0xd0)	VEC(0xd1)	VEC(0xd2)	VEC(0xd3)	VEC(0xd4)	VEC(0xd5)	VEC(0xd6)	VEC(0xd7)
VEC(0xd8)	VEC(0xd9)	VEC(0xda)	VEC(0xdb)	VEC(0xdc)	VEC(0xdd)	VEC(0xde)	VEC(0xdf)
VEC(0xe0)	VEC(0xe1)	VEC(0xe2)	VEC(0xe3)	VEC(0xe4)	VEC(0xe5)	VEC(0xe6)	VEC(0xe7)
VEC(0xe8)	VEC(0xe9)	VEC(0xea)	VEC(0xeb)	VEC(0xec)	VEC(0xed)	VEC(0xee)	VEC(0xef)
VEC(0xf0)	VEC(0xf1)	VEC(0xf2)	VEC(0xf3)	VEC(0xf4)	VEC(0xf5)	VEC(0xf6)	VEC(0xf7)
VEC(0xf8)	VEC(0xf9)	VEC(0xfa)	VEC(0xfb)	VEC(0xfc)	VEC(0xfd)	VEC(0xfe)	VEC(0xff)
.global vector_entry_points_end
vector_entry_points_end:

#define PUSH_REGS \
	push %r15; push %r14; push %r13; push %r12; push %r11; push %r10; push %r9; push %r8; \
	push %rdi; push %rsi; push %rbp; /*push %rsp; */ push %rbx; push %rdx; push %rcx; push %rax
#define POP_REGS \
	pop %rax; pop %rcx; pop %rdx; pop %rbx; /* pop %rsp; */ pop %rbp; pop %rsi; pop %rdi; \
	pop %r8; pop %r9; pop %r10; pop %r11; pop %r12; pop %r13; pop %r14; pop %r15

vec_common:
PUSH_REGS
mov %rsp, %rdi
call do_vector
.global ret_from_interrupt
ret_from_interrupt:
bt $FLAG_NEED_RESCHED, global_flags
jnc 1f
cmpq $USER_CS, 72(%rsp)
jne 1f
call schedule
1:
POP_REGS
add $16, %rsp	/* Pop vector and error_code */
iretq


.global kernel_thread_start
kernel_thread_start:
sti
pop %rdi	/* get argument */
ret		/* jmp to read thread function */

.global system_call
system_call:
	mov %rsp, swapgs_user_rsp
	mov swapgs_kernel_rsp, %rsp
	push $USER_DS
	push swapgs_user_rsp
	push %r11	/* user rflags */
	push $USER_CS
	push %rcx	/* user rip */
	push $0		/* error code */
	push $0		/* vector */
	PUSH_REGS
	sti

	cmp $NUM_SYSCALLS, %rax
	jae 1f
	/*
         * Syscall args are in rdi, rsi, rdx, r10, r8, r9. To call a function,
         * all are in place except for r10 which should be copied to rcx.
         */
	mov %r10, %rcx
	call *sys_call_table(, %rax, 8)
2:
	mov %rax, (%rsp)
.global ret_from_fork
ret_from_fork:
	cli
	bt $FLAG_NEED_RESCHED, global_flags
	jnc 3f
	call schedule
3:
	POP_REGS
	add $16, %rsp	/* error code and vector */
	pop %rcx	/* user rip */
	add $8, %rsp	/* user cs */
	pop %r11	/* user rflags */
	pop %rsp
	sysretq
1:
	mov $-ENOSYS, %rax
	jmp 2b

.data
.align 8
.global swapgs_area
swapgs_area:
.global swapgs_kernel_rsp
swapgs_kernel_rsp: .quad 0
swapgs_user_rsp: .quad 0


.text
#define ENTRY(x) .extern x; .quad x

.align 8

sys_call_table:
ENTRY(printk)
ENTRY(sys_fork)
ENTRY(sys_exec)
ENTRY(sys_exit)
ENTRY(sys_getpid)
ENTRY(sys_pause)
ENTRY(sys_yield)

NUM_SYSCALLS = (. - sys_call_table) / 8
