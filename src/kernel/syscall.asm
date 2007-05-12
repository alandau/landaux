%define MAX_SYSCALLS 256

%macro ENTRY 1
EXTERN %1
dd %1
%endmacro

GLOBAL sys_call_table
EXTERN sys_ni_syscall

ALIGN 4

sys_call_table:
ENTRY printk
ENTRY sys_fork
ENTRY sys_exec
ENTRY sys_exit

%rep MAX_SYSCALLS-($-sys_call_table)/4
ENTRY sys_ni_syscall
%endrep
