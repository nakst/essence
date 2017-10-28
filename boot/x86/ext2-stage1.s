[bits 16]
[org 0x7C00]

%define superblock 0x8000
%define temporary_load_buffer 0x9000
%define block_group_descriptor_table 0x10000
%define directory_load_location 0x1000
%define stage2_load_location 0x1000
%define inode_table_load_buffer 0x20000
;%define magic_breakpoint xchg bx,bx
%define magic_breakpoint 

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

	; Load the next sector
	mov	cx,1
	mov	eax,1
	mov	edi,0x7E00
	call	load_sectors

	; Load the superblock
	mov	eax,2
	mov	cx,1
	mov	edi,superblock
	call	load_sectors

	; Check the version number in the superblock is valid
	mov	eax,[superblock + 76]
	mov	si,error_unsupported_ext2_version
	cmp	eax,1
	jl	error

	; Load the block group descriptor table
	xor	edx,edx
	mov	eax,[superblock + 4]
	div	dword [superblock + 32]
	shr	eax,4
	mov	cx,ax
	mov	edi,block_group_descriptor_table
	mov	eax,4
	inc	cx
	call	load_sectors

	; Load the root directory
	mov	eax,2
	mov	edi,directory_load_location
	call	load_inode

	; Scan the root directory for the operating system folder and load it
	sub	edi,directory_load_location
	mov	ax,directory_load_location / 16
	mov	fs,ax
	mov	si,directory_name
	xor	bx,bx
	call	scan_directory
	mov	edi,directory_load_location
	call	load_inode

	; Scan the root directory for the bootloader and load it
	sub	edi,directory_load_location
	mov	ax,directory_load_location / 16
	mov	fs,ax
	mov	si,file_name
	xor	bx,bx
	call	scan_directory
	mov	edi,stage2_load_location
	call	load_inode

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

directory_name: db "os",0
file_name: db "boot",0

drive_number: db 0
partition_entry: db 0

read_structure: ; Data for the extended read calls
	dw	0x10
	dw	1
	dd 	temporary_load_buffer
	.lba:	dq 0

error_cannot_read_disk: db "Error: The disk could not be read. (S1)",0

times (0x200 - ($-$$)) nop

error_unsupported_ext2_version: db "Error: The version of Ext2 on the disk is not supported. (S1)",0
error_file_too_large: db "Error: The file was too large to be loaded. (S1)",0
error_cannot_find_file: db "Error: A file necessary for booting could not found. (S1)",0

load_inode:
	; Load inode EAX to EDI
	magic_breakpoint
	
	; Calculate the block group and block group index
	xor	edx,edx
	dec	eax
	div	dword [superblock + 40]

	; Find the inode table entry
	mov	bx,block_group_descriptor_table / 16
	mov	fs,bx
	mov	ebx,eax
	shl	ebx,5
	mov	eax,[fs:bx + 8]
	call	block_to_sector

	; Load the inode table
	mov	ecx,edx
	shr	ecx,2
	add	eax,ecx
	mov	ecx,1
	push	edi
	mov	edi,inode_table_load_buffer
	push	edx
	call	load_sectors
	pop	edx
	pop	edi

	; Find the inode in the table
	mov	ax,inode_table_load_buffer / 16
	mov	fs,ax
	and	edx,3
	shl	edx,7
	mov	ebx,edx

	; Work out the number of blocks to load
	mov	eax,[fs:bx + 4]
	push	ebx
	push	ecx
	push	edx
	mov	ecx,[superblock + 24]
	mov	ebx,0x400
	shl	ebx,cl
	mov	[.bytes_per_block],ebx
	xor	edx,edx
	div	dword [.bytes_per_block]
	or	edx,edx
	jz	.done
	inc	eax
	jmp	.done
	.bytes_per_block: dd 0
	.done:
	pop	edx
	pop	ecx
	pop	ebx

	; Make sure the file isn't too large
	mov	ecx,eax
	mov	si,error_file_too_large
	cmp	eax,12
	jge	error

	; Get the number of bytes in a block in EAX
	mov	eax,1
	call	block_to_sector
	shl	eax,9

	; Load every direct block needed
	add	bx,40
	.loop:
	pusha
	mov	cx,ax
	shr	cx,9
	mov	eax,[fs:bx]
	call	block_to_sector
	call	load_sectors
	popa
	add	bx,4
	add	edi,eax
	loop	.loop

	ret

block_to_sector:
	push	ebx
	push	ecx
	push	edx

	; Convert the block in EAX to a sector
	mov	ecx,[superblock + 24]
	mov	ebx,2
	shl	ebx,cl
	mov	[.sectors_per_block],ebx
	mul	dword [.sectors_per_block]

	pop	edx
	pop	ecx
	pop	ebx
	ret

	.sectors_per_block: dd 0

scan_directory:
	; Scan directory FS:BX for file SI (zero terminated), with limit DI, and BX 0, returning the inode in EAX
	push	si
	push	bx

	; Compare the filenames
	xor	cx,cx
	mov	cl,[fs:bx + 6]
	mov	edx,[fs:bx]
	add	bx,8
	.char_loop:
	lodsb
	or	al,al
	jz	.try_next
	cmp	al,[fs:bx]
	jne	.try_next
	inc	bx
	loop	.char_loop
	lodsb
	or	al,al
	jnz	.try_next

	; Return the inode
	mov	eax,edx
	pop	bx
	pop	si
	ret

	; Go to the next directory entry
	.try_next:
	pop	bx
	pop	si
	mov	ax,[fs:bx + 4]
	sub	di,ax
	add	bx,ax
	push	si
	mov	si,error_cannot_find_file
	or	di,di
	jz	error
	pop	si
	jmp	scan_directory

times (0x400 - ($-$$)) nop
