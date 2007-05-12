[BITS 16]
CPU 386
EXTERN kernel_start

%macro SAVE_ALL 0
; eax is saved in int_common
push gs
push fs
push es
push ds
push ebp
push edi
push esi
push edx
push ecx
push ebx
%endmacro

%macro RESTORE_ALL 0
pop ebx
pop ecx
pop edx
pop esi
pop edi
pop ebp
pop ds
pop es
pop fs
pop gs
pop eax
%endmacro

KERNEL_CS equ 0x0008
KERNEL_DS equ 0x0010
LDT_SEL   equ 0x0018

USER_CS   equ 0x000F
USER_DS   equ 0x0017


; we load the kernel at virtual C0000000 and physical 1000
; thus the segment base must be 40001000

%define SEG_BASE			0x40001000
%define KERNEL_VIRT_ADDR	0xC0000000
%define KERNEL_PHYS_ADDR	0x1000
%define PHYS_MEM_VIRT_ADDR	0xF0000000


EXTERN page_dir
EXTERN kernel_page_table
EXTERN first_mb_table
;KER_PAGE_TABLE	equ	0x101000	; 1MB + 4KB
;FIRST_MB	equ	0x102000	; 1MB + 8KB
;ALL_MEM_TABLES	equ	0x103000	; 1MB + 12KB

EXTERN global_flags
EXTERN schedule

GLOBAL entry
entry:
cli
mov ax, 0xB800
mov fs, ax
mov byte [fs:0], '1'


; load GDT, LDT and IDT (assumes cs=0)
mov ebx, SEG_BASE+gdtr
lgdt [cs:ebx]
mov ebx, SEG_BASE+idtr
lidt [cs:ebx]
; patch LDT descriptor with the address of the LDT segment
mov ebx, SEG_BASE+ldt_des
mov eax, SEG_BASE+ldt
mov [cs:ebx+2], ax
shr eax, 16
mov [cs:ebx+4], al
mov [cs:ebx+7], ah
mov byte [fs:0], '2'

; switch to PM
mov eax, cr0
or al, 1
mov cr0, eax
mov byte [fs:0], '3'

;jmp KERNEL_CS:start32
o32
db 0xEA
dd SEG_BASE+start32
dw KERNEL_CS

GLOBAL ldt_des, tss_des

align 4
gdtr dw gdt_size
     dd SEG_BASE+gdt
gdt     dw 0,0,0,0
code32  dw 0xFFFF,0x0000
        db 0x00,0x9A,0xCF,0x00
data32  dw 0xFFFF,0x0000
        db 0x00,0x92,0xCF,0x00
ldt_des dw ldt_size,0
        db 0,0x82,0x00,0
tss_des dw 0x0067,0
        db 0,0x89,0x00,0
gdt_size equ $-gdt-1

align 4
ldt     dw 0,0,0,0
usrcode dw 0xFFFF,0x0000
        db 0x00,0xFA,0xCF,0x00
usrdata dw 0xFFFF,0x0000
        db 0x00,0xF2,0xCF,0x00
ldt_size equ $-ldt-1




; in PM!!!
[BITS 32]
GLOBAL setup_idtr
setup_idtr:
;mov eax, idtr
lidt [idtr]
ret

start32:
mov ax, LDT_SEL
lldt ax

mov byte [fs:0], '4'
xor eax, eax
mov ax, KERNEL_DS		; data descriptor
mov ds, ax
mov es, ax
mov fs, ax
mov gs, ax
mov byte [fs:0xB8000], '5'
mov ax, KERNEL_DS
mov ss, ax
; initialize esp after paging is enabled
mov eax, [SEG_BASE+data_magic]
cmp eax, DS_MAGIC
jne $

; everything loaded successfully!!

; enable A20
mov dl, byte [0x500]
mov byte [0x500], 0

in al, 0x92
or al, 0x02
out 0x92, al

mov dh, byte [0x100500]
mov byte [0x100500], 1
cmp byte [0x500], 0
jne $
; A20 enabled! restore memory contents
mov byte [0x500], dl
mov byte [0x100500], dh

; zero BSS
EXTERN bss, bss_end
mov edi, SEG_BASE+bss
mov ecx, SEG_BASE+bss_end
sub ecx, edi
shr ecx, 2
cld
xor eax, eax
rep stosd

; initialize paging
; page directory and page table are part of the BSS and thus are already cleared
; cld
; xor eax, eax
; mov edi, PAGE_DIR
; mov ecx, 4096 / 4
; rep stosd
; mov edi, KER_PAGE_TABLE
; mov ecx, 4096 / 4
; rep stosd
; mov edi, FIRST_MB
; mov ecx, 4096 / 4
; rep stosd

; make the pde corresponding to the kernel addresses to point to the page table. 3=ring0 + present + write
mov eax, SEG_BASE+first_mb_table
or eax, 7
mov dword [SEG_BASE+page_dir], eax
mov eax, SEG_BASE+kernel_page_table
or eax, 7
mov dword [SEG_BASE+page_dir + KERNEL_VIRT_ADDR/(4*1024*1024)*4], eax

; map all memory
; mov edi, PAGE_DIR + (PHYS_MEM_VIRT_ADDR / (4*1024*1024) * 4)
; mov eax, ALL_MEM_TABLES | 7
; mov ecx, MEM_SIZE / (4*1024*1024)
; map_page_dir:
; stosd
; add eax, 4096
; loop map_page_dir

; fill in page table to map kernel addresses
EXTERN end
mov edi, SEG_BASE+kernel_page_table
mov eax, KERNEL_PHYS_ADDR | 7
mov ecx, end
sub ecx, KERNEL_VIRT_ADDR - 4095
shr ecx, 12			; ecx = num pages in kernel
map_kernel:
stosd
add eax, 4096
loop map_kernel

; map first 1MB
 mov edi, SEG_BASE+first_mb_table
 mov eax, 0 | 7
 mov ecx, 1024*1024/4096		; ecx = num pages in 1MB
 map_1mb:
 stosd
 add eax, 4096
 loop map_1mb

; map all physical memory to PHYS_MEM
; mov edi, ALL_MEM_TABLES
; mov eax, 0 | 3
; mov ecx, MEM_SIZE / 4096	; ecx = num pages in physical memory
; mem_map:
; stosd
; add eax, 4096
; loop mem_map

; all page tables are now setup, enable paging!
mov eax, SEG_BASE+page_dir
mov cr3, eax
jmp $+2
mov eax, cr0
or eax, 0x80000000
mov cr0, eax
error_here:
jmp $+2
jmp KERNEL_CS:paging_enabled

paging_enabled:
EXTERN idle_task_stack
%define STACK_SIZE 4096
mov esp, idle_task_stack + STACK_SIZE
mov byte [0xb8000], '*'
push ss
push esp
pushf
push cs
call .1
.1:
push 0			; error code
push eax		; to make the stack look like a regular kernel mode stack
SAVE_ALL
call kernel_start
jmp $


%macro setup_int 1
    dw (%1 - $$ + KERNEL_VIRT_ADDR) & 0xFFFF, KERNEL_CS
    db 0,0x8E
    dw (%1 - $$ + KERNEL_VIRT_ADDR) >> 16
%endmacro

%macro setup_syscall 1
    dw (%1 - $$ + KERNEL_VIRT_ADDR) & 0xFFFF, KERNEL_CS
    db 0,0xEF
    dw (%1 - $$ + KERNEL_VIRT_ADDR) >> 16
%endmacro

align 4
idtr dw idt_size
     dd idt
;     dd SEG_BASE+idt
idt:
setup_int int_divide				; 0
setup_int int_debug					; 1
setup_int int_reserved				; 2
setup_int int_breakpoint			; 3
setup_int int_overflow				; 4
setup_int int_bounds				; 5
setup_int int_opcode				; 6
setup_int int_coprocessor_na		; 7
setup_int int_double_fault			; 8
setup_int int_coprocessor_seg		; 9
setup_int int_tss					; 10
setup_int int_segment_na			; 11
setup_int int_stack					; 12
setup_int int_general_prot			; 13
setup_int int_page_fault			; 14
setup_int int_reserved				; 15
setup_int int_coprocessor_err		; 16
; 17-31 are reserved by Intel
%rep (32-17)
setup_int int_reserved
%endrep
; IRQ0-IRQ7
setup_int irq0
setup_int irq1
setup_int irq2
setup_int irq3
setup_int irq4
setup_int irq5
setup_int irq6
setup_int irq7
; IRQ8-IRQ15
setup_int irq8
setup_int irq9
setup_int irq10
setup_int irq11
setup_int irq12
setup_int irq13
setup_int irq14
setup_int irq15

setup_syscall system_call
idt_size equ $-idt-1

DS_MAGIC equ 0x10293847



%macro call_int_no_error 1
EXTERN %1
push 0
push %1
jmp int_common
%endmacro

%macro call_int_error 1
EXTERN %1
push %1
jmp int_common
%endmacro

int_divide:			call_int_no_error do_divide
int_debug: 			call_int_no_error do_debug
int_breakpoint:			call_int_no_error do_breakpoint
int_overflow:			call_int_no_error do_overflow
int_bounds:			call_int_no_error do_bounds
int_opcode:			call_int_no_error do_opcode
int_coprocessor_na:		call_int_no_error do_coprocessor_na
int_double_fault:		call_int_error    do_double_fault
int_coprocessor_seg:		call_int_no_error do_coprocessor_seg
int_tss:			call_int_error    do_tss
int_segment_na:			call_int_error    do_segment_na
int_stack:			call_int_error    do_stack
int_general_prot:		call_int_error    do_general_prot
int_page_fault:			call_int_error do_page_fault
int_coprocessor_err:		call_int_no_error do_coprocessor_err
int_reserved:			call_int_no_error do_reserved



EAX_OFFSET equ 0x28		; offset of eax into the stack

int_common:
SAVE_ALL
xchg eax, [esp+EAX_OFFSET]	; now stack contains eax at offset EAX_OFFSET
call eax			; and eax contains function to call
RESTORE_ALL
add esp, 4			; discard error_code
iret

%macro call_irq 1
push %1
push do_irq
SAVE_ALL
xchg eax, [esp+EAX_OFFSET]	; now stack contains eax at offset EAX_OFFSET
call eax			; and eax contains function to call
jmp ret_from_interrupt
%endmacro

EXTERN do_irq
%assign i 0
%rep 16
irq %+ i: call_irq i
%assign i i+1
%endrep


EXTERN sys_call_table
GLOBAL ret_from_sys_call
%define MAX_SYSCALLS 256

; sys_something(ebx, ecx, edx, esi, edi, ebp)
system_call:
	or dword [global_flags], 1<<1			; in syscall
	push 0		; error code
	push eax
	SAVE_ALL
	cmp eax, MAX_SYSCALLS
	jnb bad_sys
	lea eax, [sys_call_table+4*eax]
	push ebp
	push edi
	push esi
	push edx
	push ecx
	push ebx
	call [eax]
	add esp, 6*4
	mov [esp+EAX_OFFSET], eax
	jmp ret_from_sys_call

bad_sys:
	mov dword [esp+EAX_OFFSET], -1

ret_from_interrupt:
	test dword [global_flags], 1<<1			; in syscall
	jnz restore_all
ret_from_sys_call:
	test dword [global_flags], 1<<0			; need resched
	jz .1
	call schedule
.1:
	and dword [global_flags], ~(1<<1)		; now not in syscall
restore_all:
	RESTORE_ALL
	add esp, 4	; error code
	iret



; stack layout on enter to do_something

; 00	ebx
; 04	ecx
; 08	edx
; 0C	esi
; 10	edi
; 14	ebp
; 18	ds
; 1C	es
; 20	fs
; 24	gs
; 28	eax (&do_something)
; 2C	error_code
; 30	eip
; 34	cs
; 38	eflags
; 3C	oldesp
; 40	oldss





SECTION .data
data_magic dd DS_MAGIC
