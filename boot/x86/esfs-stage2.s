%macro FilesystemInitialise 0
%define superblock 0x8000
%define kernel_file_entry 0x8800
%endmacro

%macro FilesystemGetKernelSize 0
load_kernel:
	; Load the superblock.
	mov	eax,16
	mov	edi,superblock
	mov	cx,1
	call	load_sectors
	mov	ax,superblock / 16
	mov	fs,ax

	; Check the signature.
	mov	eax,[fs:0]
	cmp	eax,0x65737345
	mov	si,ErrorBadFilesystem
	jne	error

	; Save the OS installation identifier.
	mov	eax,[fs:132]
	mov	[os_installation_identifier + 0],eax
	mov	eax,[fs:136]
	mov	[os_installation_identifier + 4],eax
	mov	eax,[fs:140]
	mov	[os_installation_identifier + 8],eax
	mov	eax,[fs:144]
	mov	[os_installation_identifier + 12],eax

	; Load the kernel's file entry.
	xor	eax,eax
	mov	ax,[fs:112]
	mov	edi,kernel_file_entry
	mov	cx,1
	call	load_sectors

	; Find the data stream.
FindDataStreamLoop:
	mov	bx,[KernelDataStreamPosition]
	mov	eax,[fs:bx]
	cmp	ax,0xDA2A
	je	SaveKernelSize
	shr	eax,16
	add	[KernelDataStreamPosition],ax
	jmp	FindDataStreamLoop

	; Save the size of the kernel.
SaveKernelSize:
	mov	eax,[fs:bx + 8]
	mov	[kernel_size],eax
	mov	si,error_kernel_too_large
	cmp	eax,0x300000
	jg	error
%endmacro

%macro FilesystemLoadKernelIntoBuffer 0
	magic_breakpoint

	; Check that the kernel is stored as DATA_INDIRECT file.
CheckForDataIndirect:
	mov	ax,superblock / 16
	mov	fs,ax
	mov	bx,[KernelDataStreamPosition]
	mov	ax,[fs:bx+5]
	mov	si,ErrorUnexpectedFileProblem
	cmp	al,1
	jne	error
	
	; Load each extent from the file.
LoadEachExtent:
	mov	edi,[kernel_buffer]
	mov	si,[fs:bx+6]
	add	bx,16

	; Load the blocks.
	.ExtentLoop:
	mov	eax,[fs:56]
	shr	eax,9
	mov	cx,[fs:bx+8]
	mul	cx
	mov	cx,ax
	mov	eax,[fs:bx]
	call	load_sectors

	; Go to the next extent.
	add	bx,16
	sub	si,1
	cmp	si,0
	jne	.ExtentLoop
%endmacro

%macro FilesystemSpecificCode 0
KernelDataStreamPosition: dw 0x830
ErrorBadFilesystem: db "Invalid boot EsFS volume.",0
ErrorUnexpectedFileProblem: db "The kernel file could not be loaded.",0
%endmacro
