bits 16
org 0x7C00
BEGIN:

	jmp 0:start
start:
	mov ax, 0
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	mov bp, BEGIN
	mov sp, BEGIN
	sti
	cld

	mov ax, 0x0e41
	int 0x10
	jmp $

times 446 - $ + $$ db 0
; 	MBR partition table will be here
times 510 - $ + $$ db 0
dw 0xAA55
END:
