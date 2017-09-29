[bits 16]
[org 0x600]

start:
	; Setup segment registers and the stack
	cli
	mov	ax,0
	mov	ds,ax
	mov	es,ax
	mov	fs,ax
	mov	gs,ax
	mov	ss,ax
	mov	sp,0x7C00
	sti

	; Clear the screen
	mov	ax,0
	int	0x10
	mov	ax,3
	int	0x10

	; Relocate to 0x600
	cld
	mov	si,0x7C00
	mov	di,0x600
	mov	cx,0x200
	rep	movsb
	jmp	0x0:find_partition

drive_number: db 0

find_partition:
	; Save the drive number
	mov	byte [drive_number],dl

	; Find the bootable flag (0x80)
	mov	bx,partition_entry_1
	cmp	byte [bx], 0x80
	je	found_partition
	mov	bx,partition_entry_2
	cmp	byte [bx], 0x80
	je	found_partition
	mov	bx,partition_entry_3
	cmp	byte [bx], 0x80
	je	found_partition
	mov	bx,partition_entry_4
	cmp	byte [bx], 0x80
	je	found_partition

	; No bootable partition
	mov	si,error_no_bootable_partition
	jmp	error

found_partition:
	; Load the first sector of the partition at 0x7C00
	push	bx
	mov	eax,[bx + 8]
	mov	[read_structure.lba],eax
	mov	ah,0x42
	mov	dl,[drive_number]
	push	dx
	mov	si,read_structure
	int	0x13

	; Check for an error
	mov	si,error_cannot_read_disk
	jc	error

	; Jump to the partition's boot sector
	pop	dx
	pop	si
	jmp	0x0:0x7C00

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

read_structure: ; Data for the extended read calls
	dw	0x10
	dw	1
	dd	0x7C00
	.lba:	dq 0

error_cannot_read_disk: db "Error: The disk could not be read. (MBR)",0
error_no_bootable_partition: db "Error: No bootable partition could be found on the disk.",0

times (0x1b4 - ($-$$)) nop

disk_identifier: times 10 db 0
partition_entry_1: times 16 db 0
partition_entry_2: times 16 db 0
partition_entry_3: times 16 db 0
partition_entry_4: times 16 db 0

dw 0xAA55
