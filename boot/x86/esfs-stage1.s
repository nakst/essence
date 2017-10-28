[bits 16]
[org 0x7C00]

%define superblock 0x8000
%define temporary_load_buffer 0x9000
%define block_group_descriptor_table 0x10000
%define directory_load_location 0x1000
%define stage2_load_location 0x1000
%define inode_table_load_buffer 0x20000
%define magic_breakpoint xchg bx,bx
;%define magic_breakpoint 

start:
	; Setup segment registers
	cli
	mov	ax,0
	mov	ds,ax
	mov	es,ax
	mov	fs,ax
	mov	gs,ax
	mov	ss,ax
	mov	sp,0x7C00
	jmp	0x0:.continue
	.continue:
	sti

	; Save the information passed from the MBR
	mov 	[drive_number],dl
	mov 	[partition_entry],si

	; Print a startup message
	mov	si,startup_message
	.loop:
	lodsb
	or	al,al
	jz	.end
	mov	ah,0xE
	int	0x10
	jmp	.loop
	.end:

	; Load the second stage
	mov	cx,15
	mov	eax,1
	mov	edi,stage2_load_location
	call	load_sectors

	mov	dl,[drive_number]
	mov	si,[partition_entry]
	jmp	0:stage2_load_location

load_sectors:
	; Load CX sectors from sector EAX to EDI
	pusha
	push	edi

	; Add the partition offset to EAX
	mov	bx,[partition_entry]
	mov	ebx,[bx + 8]
	add	eax,ebx

	; Load 1 sector
	mov	[read_structure.lba],eax
	mov	ah,0x42
	mov	dl,[drive_number]
	mov	si,read_structure
	int	0x13

	; Check for error
	mov	si,error_cannot_read_disk
	jc	error

	; Copy the data to its destination
	pop	edi
	mov	cx,0x200
	mov	eax,edi
	shr	eax,4
	and	eax,0xF000
	mov	es,ax
	mov	si,temporary_load_buffer
	rep	movsb

	; Go to the next sector
	popa
	add	edi,0x200
	inc	eax
	loop	load_sectors
	ret

error:
	; Print an error message
	lodsb
	or	al,al
	jz	.break
	mov	ah,0xE
	int	0x10
	jmp	error

	; Break indefinitely
	.break:
	cli
	hlt

startup_message: db "Booting operating system...",10,13,0

drive_number: db 0
partition_entry: db 0

read_structure: ; Data for the extended read calls
	dw	0x10
	dw	1
	dd 	temporary_load_buffer
	.lba:	dq 0

error_cannot_read_disk: db "Error: The disk could not be read. (S1)",0

times (0x200 - ($-$$)) nop
