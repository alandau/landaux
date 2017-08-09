.globl _start
_start:
	call main
	movl $3, %eax
	int $0x30
	int3

.section .note.GNU-stack,"",%progbits
