.globl _start
_start:
	call main
	mov $3, %eax
	syscall
	int3

.section .note.GNU-stack,"",%progbits
