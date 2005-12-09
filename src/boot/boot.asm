[BITS 16]
CPU 386

MAX_KERNEL_SECS equ 64


jmp start

bsOEMName       db 'Landaux '
bpbBytesPerSec  dw 512
bpbSecPerClus   db 1
bpbRsrvdSecCnt  dw 1
bpbNumFATs      db 2
bpbRootEntCnt   dw 224
bpbSectorCount  dw 2880
bpbMedia        db 0F0h
bpbFATSize      dw 9
bpbSecPerTrack  dw 18
bpbNumHeads     dw 2
bpbHiddenSec    dd 0
bpbTotSec32     dd 0

bsBootDev       db 0
bsReserved      db 0
bsBootSig       db 29h
bsVolumeID      dd 0
bsVolumeLabel   db 'LANDAUXDISK'
bsFileSysType   db 'FAT12   '

hello_msg db 'Booting Landaux...',13,10,0

start:
mov ax, 0x07C0
mov ds, ax

mov ax, 0x9000
mov ss, ax
mov sp, 0xFFFF - (510-go)
mov es, ax

; print banner
mov si, hello_msg
call print

; relocate bootloader
cld
mov si, go
mov di, 0xFFFF - (510-go)
mov cx, - (510-go)
neg cx
rep movsb

mov ax, 0x0100
mov es, ax	; load kernel here

jmp 0x9000:0xFFFF - (510-go)


go:
; load sector
mov si, 510 - MAX_KERNEL_SECS*2
cld
xor bx, bx
.again:
lodsw
test ax, ax
jz .end
mov cx, [si]
add si, 2
call bios_read
add bx, 512
jmp .again
.end:

; jump to kernel
mov dl, [bsBootDev]
mov ax, es
shl ax, 4
push word 0
push ax
retf

; prints str at ds:si
print:
        mov ah, 0x0E
	mov bx, 0x0007
	cld
.print:
	lodsb
	test al, al
	jz .end
	int 0x10
	jmp .print
.end:
	ret

; reads cl sectors starting from ax into memory (es:bx)
bios_read:
	push ax
.again:
	push cx
	xor dx, dx
	div word [bpbSecPerTrack]
	inc dx
	mov cl, dl	; cl = sector
	xor dx, dx
	div word [bpbNumHeads]
	; ax = track
	; dx = head
	xchg al, ah
	or cx, ax	; cx = track and sector
	mov dh, dl	; dh = head
	mov dl, [bsBootDev]
	pop ax		; al = no. of sectors
	mov ah, 0x02	; read
	int 0x13
	pop ax
	jc .again
	ret
