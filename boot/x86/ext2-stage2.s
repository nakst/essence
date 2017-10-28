%macro FilesystemInitialise 0
%define superblock 0x8000
%define block_group_descriptor_table 0x10000
%define inode_table_load_buffer 0x20000
%define directory_load_location 0x30000
%endmacro

%macro FilesystemGetKernelSize 0
load_kernel:
	; Load the root directory
	mov	eax,2
	mov	edi,directory_load_location
	call	load_inode

	; Scan the root directory for the operating system folder
	sub	edi,directory_load_location
	mov	ax,directory_load_location / 16
	mov	fs,ax
	mov	si,directory_name
	xor	bx,bx
	call	scan_directory

	; Load the operating system folder
	mov	edi,directory_load_location
	call	load_inode

	; Scan the root directory for the kernel
	sub	edi,directory_load_location
	mov	ax,directory_load_location / 16
	mov	fs,ax
	mov	si,file_name
	xor	bx,bx
	call	scan_directory

	; Get the size of the kernel
	push	eax
	mov	edi,-1
	call	load_inode
	mov	[kernel_size],eax

	mov	si,error_kernel_too_large
	cmp	eax,0x300000
	jg	error
%endmacro

%macro FilesystemLoadKernelIntoBuffer 0
actually_load_kernel:
	; Load the kernel
	mov	edi,[kernel_buffer]
	pop	eax
	call	load_inode
%endmacro

%macro FilesystemSpecificCode 0
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

load_inode:
	; Load inode EAX to EDI
	mov	[.stack_position],esp
	mov	[.destination],edi
	
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
	cmp	dword [.destination],-1
	je	.finish ; Early exit to return length if no destination
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

	; Store the number of blocks to load
	mov	ecx,eax
	mov	[.blocks_remaining],ecx

	; Get the number of sectors in a block
	mov	eax,1
	call	block_to_sector
	mov	[.sectors_per_block],eax

	; Load direct blocks
	mov	cx,12
	add	bx,40
	.loop:
	mov	eax,[fs:bx]
	call	.load_block0
	add	bx,4
	loop	.loop

	; Load indirect blocks
	mov	eax,[fs:bx]
	pusha
	call	block_to_sector
	mov	ecx,[.sectors_per_block]
	mov	edi,indirect_block_buffer
	call	load_sectors
	mov	ebx,indirect_block_buffer / 16
	mov	gs,bx
	xor	ebx,ebx
	mov	ecx,[.bytes_per_block]
	shr	ecx,2
	.loop1:
	mov	eax,[gs:bx]
	call	.load_block0
	add	ebx,4
	loop	.loop1
	popa

	; The file was too large
	mov	si,error_file_too_large
	jmp	error

	; Load block EAX
	.load_block0:
	pusha
	call 	block_to_sector
	mov	ecx,[.sectors_per_block]
	mov	edi,[.destination]
	call	load_sectors
	mov	ecx,[.bytes_per_block]
	add	[.destination],ecx
	dec	dword [.blocks_remaining]
	jz	.finish
	popa
	ret

	; Restore the stack position and return
	.finish:
	mov	esp,[.stack_position]
	ret

	.stack_position: dd 0
	.destination: dd 0
	.blocks_remaining: dd 0
	.sectors_per_block: dd 0

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

directory_name: db "os",0
file_name: db "kernel",0
%endmacro
