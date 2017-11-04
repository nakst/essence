[bits 16]
[org 0x1000]

; This is missing any filesystem specific macros.
%define vesa_info 0x7000
%define os_installation_identifier 0x7FF0
%define temporary_load_buffer 0x9000
%define page_directory 0x40000 
%define page_directory_length 0x20000
%define memory_map 0x60000
%define indirect_block_buffer 0x64000
; %define magic_breakpoint xchg bx,bx
%define magic_breakpoint

start:
	cmp	dword [page_table_allocation_location],0x2000
	je	.set_stack

	.set_stack:
	mov	esp,0x7C00

	; Save information passed from stage 1
	mov	[drive_number],dl
	mov	[partition_entry],si

	; @FilesystemSpecific
	FilesystemInitialise

check_pci:	
	; Check the computer has PCI
	mov	ax,0xB101
	xor	edi,edi
	int	0x1A
	mov	si,error_no_pci
	jc	error
	or	ah,ah
	jnz	error

enable_video_mode:
	; Uncomment to disable video mode setting.
	; jmp	check_cpuid

	; TODO Proper video mode checking/selection.
	mov	ax,vesa_info >> 4
	mov	es,ax
	xor	di,di
	mov	ax,0x4F01
;%define video_mode 274
;%define video_mode 277
%define video_mode 280
;%define video_mode 283
;%define video_mode 287
	mov	cx,video_mode | (1 << 14)
	int	0x10
	mov	si,error_could_not_set_video_mode
	cmp	ax,0x4F
	jne	error
	mov	ax,0x4F02
	mov	bx,video_mode | (1 << 14)
	int	0x10
	cmp	ax,0x4F
	mov	si,error_could_not_set_video_mode
	jne	error

check_cpuid:
	; Check the CPU has CPUID
	mov	dword [24],.no_cpuid
	mov	eax,0
	cpuid
	jmp	.has_cpuid
	.no_cpuid:
	mov	si,error_no_cpuid
	jmp	error
	.has_cpuid:

check_msr:
	; Check the CPU has MSRs
	mov	dword [24],.no_msr
	mov	ecx,0xC0000080
	rdmsr
	jmp	.has_msr
	.no_msr:
	mov	si,error_no_msr
	jmp	error
	.has_msr:

enable_a20:
	; Enable the A20 line, if necessary
	cli
	call	check_a20
	jc	.a20_enabled
	mov	ax,0x2401
	int	0x15
	call	check_a20
	jc	.a20_enabled
	mov	si,error_cannot_enable_a20_line
	jmp	error
	.a20_enabled:
	sti

identity_paging:
	; Map the first 4MB to itself for the bootloader to do work in protected mode
	mov	eax,page_directory / 16
	mov	es,ax

	; Clear the page directory
	xor	eax,eax
	mov	ecx,0x400
	xor	di,di
	rep	stosd

	; Recursive map the directory
	mov	dword [es:0x3FF * 4],page_directory | 3

	; Put the first table in the directory
	mov	dword [es:0],(page_directory + 0x1000) | 3

	; Fill the table
	mov	edi,0x1000
	mov	cx,0x400
	mov	eax,3
	.loop:
	mov	[es:edi],eax
	add	edi,4
	add	eax,0x1000
	loop	.loop
	
	; Set the pointer to the page directory
	mov	eax,page_directory
	mov	cr3,eax

load_gdt:
	; Load the GDT
	lgdt	[gdt_data.gdt]

load_memory_map:
	; Load the memory map
	xor	ebx,ebx

	; Set FS to access the memory map
	mov	ax,0
	mov	es,ax
	mov	ax,memory_map / 16
	mov	fs,ax

	; Loop through each memory map entry
	.loop:
	mov	di,.entry
	mov	edx,0x534D4150
	mov	ecx,24
	mov	eax,0xE820
	mov	byte [.acpi],1
	int	0x15
	jc	.finished

	; Check the BIOS call worked
	cmp	eax,0x534D4150
	jne	.fail

	; Check if this is usuable memory
	cmp	dword [.type],1
	jne	.try_next
	cmp	dword [.size],0
	je	.try_next
	cmp	dword [.acpi],0
	je	.try_next

	; Check that the region is big enough
	mov	eax,[.size]
	and	eax,~0x3FFF
	or	eax,eax
	jz	.try_next

	; Check that the base is above 1MB
	cmp	dword [.base + 4],0
	jne	.base_good
	cmp	dword [.base],0x100000
	jl	.try_next
	.base_good:

	; Align the base to the nearest page
	mov	eax,[.base]
	and	eax,0xFFF
	or	eax,eax
	jz	.base_aligned
	mov	eax,[.base]
	and	eax,~0xFFF
	add	eax,0x1000
	mov	[.base],eax
	sub	dword [.size],0x1000
	sbb	dword [.size + 4],0
	.base_aligned:

	; Align the size to the nearest page
	mov	eax,[.size]
	and	eax,~0xFFF
	mov	[.size],eax

	; Convert the size from bytes to 4KB pages
	mov	eax,[.size]
	shr	eax,12
	push	ebx
	mov	ebx,[.size + 4]
	shl	ebx,20
	add	eax,ebx
	pop	ebx
	mov	[.size],eax
	mov	dword [.size + 4],0

	; Store the entry
	push	ebx
	mov	ebx,[.pointer]
	mov	eax,[.base]
	mov	[fs:bx],eax
	mov	eax,[.base + 4]
	mov	[fs:bx + 4],eax
	mov	eax,[.size]
	mov	[fs:bx + 8],eax
	add	[.total_memory],eax
	mov	eax,[.size + 4]
	adc	[.total_memory + 4],eax
	mov	[fs:bx + 12],eax
	add	dword [.pointer],16
	pop	ebx

	; Continue to the next entry
	.try_next:
	or	ebx,ebx
	jnz	.loop

	; Make sure that there were enough entries
	.finished:
	mov	eax,[.pointer]
	shr	eax,4
	or	eax,eax
	jz	.fail

	; Clear the base value for the entry after last
	mov	ebx,[.pointer]
	mov	dword [fs:bx],0
	mov	dword [fs:bx + 4],0

	; Store the total memory
	mov	eax,[.total_memory]
	mov	dword [fs:bx + 8],eax
	mov	eax,[.total_memory + 4]
	mov	dword [fs:bx + 12],eax

	; Load the kernel!
	jmp	load_kernel

	; Display an error message if we could not load the memory map
	.fail:
	mov	si,error_could_not_get_memory_map
	jmp	error

	.pointer:	dd 0
	.entry: 
		.base:	dq 0
		.size:	dq 0
		.type:	dd 0
		.acpi:	dd 0
	.total_memory:	dq 0

	; @FilesystemSpecific
	FilesystemGetKernelSize

allocate_kernel_buffer:
	; Switch to protected mode
	push	ds
	push	es
	push	ss
	cli
	mov	eax,cr0
	or	eax,0x80000001
	mov	cr0,eax
	jmp	0x8:.pmode

	; Set the data segment registers
	[bits 32]
	.pmode:
	mov	ax,0x10
	mov	ds,ax
	mov	es,ax
	mov	ss,ax

	; Work out the size of memory we'll allocate
	mov	ecx,[kernel_size]
	shr	ecx,12
	inc	ecx
	mov	edx,ecx
	shl	edx,12

	; For every memory region
	xor	ebx,ebx
	.memory_region_loop:

	; Is this the region starting at 1MB?
	mov	eax,[ebx + memory_map + 4]
	or	eax,eax
	jnz	.try_next_memory_region
	mov	eax,[ebx + memory_map]
	cmp	eax,0x100000
	jne	.try_next_memory_region

	; Check the region has enough pages remaining
	mov	eax,[ebx + memory_map + 8]
	cmp	eax,ecx
	jl	.try_next_memory_region

	; Remove ECX pages from the region
	mov	eax,[ebx + memory_map + 0]
	mov	[kernel_buffer],eax
	add	eax,edx
	mov	[ebx + memory_map + 0],eax
	sub	dword [ebx + memory_map + 8],ecx
	sbb	dword [ebx + memory_map + 12],0

	jmp	.found_buffer

	; Go to the next memory region
	.try_next_memory_region:
	add	ebx,16
	mov	eax,[load_memory_map.pointer]
	cmp	ebx,eax
	jne	.memory_region_loop
	mov	si,error_no_memory
	jmp	error_32

	.found_buffer:
	; Switch to 16-bit mode
	mov	eax,cr0
	and	eax,0x7FFFFFFF
	mov	cr0,eax
	jmp	0x18:.rmode

	; Switch to real mode
	[bits 16]
	.rmode:
	mov	eax,cr0
	and	eax,0x7FFFFFFE
	mov	cr0,eax
	jmp	0x0:.finish

	; Go to error
	.finish:
	pop	ss
	pop	es
	pop	ds

	; @FilesystemSpecific
	FilesystemLoadKernelIntoBuffer

setup_elf:
	; Switch to protected mode
	cli
	mov	eax,cr0
	or	eax,0x80000001
	mov	cr0,eax
	jmp	0x8:.pmode

	; Set the data segment registers
	[bits 32]
	.pmode:
	mov	ax,0x10
	mov	ds,ax
	mov	es,ax
	mov	ss,ax

	; Check the ELF data is correct
	mov	ebx,[kernel_buffer]
	mov	esi,error_bad_kernel
	cmp	dword [ebx + 0],0x464C457F
	jne	error_32
	cmp	byte [ebx + 4],2
	je	setup_elf_64
	cmp	byte [ebx + 4],1
	jne	error_32
	cmp	byte [ebx + 5],1
	jne	error_32
	cmp	byte [ebx + 7],0
	jne	error_32
	cmp	byte [ebx + 16],2
	jne	error_32
	cmp	byte [ebx + 18],3
	jne	error_32

	; Find the program header
	mov	eax,ebx
	mov	ebx,[eax + 28]
	add	ebx,eax

	; ECX = entries, EDX = size of entry
	movzx	ecx,word [eax + 44]
	movzx	edx,word [eax + 42]

	; Loop through each program header
	.loop_program_headers:
	push	eax
	push	ecx
	push	edx
	push	ebx

	; Only deal with load segments
	mov	eax,[ebx]
	cmp	eax,1
	jne	.next_entry

	; Work out how many physical pages we need to allocate
	mov	ecx,[ebx + 20]
	shr	ecx,12
	inc	ecx

	; Get the starting virtual address
	mov	eax,[ebx + 8]
	mov	[.target_page],eax

	; For every frame in the segment
	.frame_loop:
	xor	ebx,ebx

	; For every memory region
	.memory_region_loop:

	; Check the region is below 4GB
	mov	eax,[ebx + memory_map + 4]
	or	eax,eax
	jnz	.try_next_memory_region

	; Check the region has enough pages remaining
	mov	eax,[ebx + memory_map + 8]
	or	eax,eax
	jz	.try_next_memory_region

	; Remove one page from the region
	mov	eax,[ebx + memory_map + 0]
	mov	[.physical_page],eax
	add	eax,0x1000
	mov	[ebx + memory_map + 0],eax
	sub	dword [ebx + memory_map + 8],1
	sbb	dword [ebx + memory_map + 12],0

	jmp	.found_physical_page

	; Go to the next memory region
	.try_next_memory_region:
	add	ebx,16
	mov	eax,[load_memory_map.pointer]
	cmp	ebx,eax
	jne	.memory_region_loop
	mov	si,error_no_memory
	jmp	error_32

	; Map the page into virtual memory
	.found_physical_page:
	mov	eax,[.target_page]

	; Check if we need to create a new page table
	shr	eax,20
	mov	ebx,[page_directory + eax]
	or	ebx,ebx
	jnz	.write_page_table_entry

	.create_page_table:
	; Work out where to put the table
	mov	edi,[page_table_allocation_location]
	add	dword [page_table_allocation_location],0x1000
	add	edi,page_directory

	; Insert the table in the directory
	mov	ebx,edi
	or	ebx,3
	mov	[page_directory + eax],ebx

	; Clear the table
	push	ecx
	mov	ecx,0x400
	push	eax
	mov	eax,0
	rep	stosd
	pop	eax
	pop	ecx

	; Invalidate the directory
	mov	eax,cr3
	mov	cr3,eax

	.write_page_table_entry:
	; Get the address of the table
	mov	eax,[page_directory + eax]
	and	eax,~0xFFF

	; Work out the offset into the table
	mov	edx,[.target_page]
	shr	edx,10
	and	edx,0xFFF
	add	eax,edx

	; Put the entry into the table
	mov	ebx,[.physical_page]
	or	ebx,3
	mov	[eax],ebx
	mov	eax,[.target_page]
	invlpg	[eax]

	; Go to the next frame
	add	dword [.target_page],0x1000
	dec	ecx
	jnz	.frame_loop

	; Restore the pointer to the segment
	pop	ebx
	push	ebx

	; Clear the memory
	mov	ecx,[ebx + 20]
	xor	eax,eax
	mov	edi,[ebx + 8]
	rep	stosb

	; Copy the memory
	mov	ecx,[ebx + 16]
	mov	esi,[ebx + 4]
	add	esi,[kernel_buffer]
	mov	edi,[ebx + 8]
	rep	movsb

	; Go to the next entry
	.next_entry:
	pop	ebx
	pop	edx
	pop	ecx
	pop	eax

	add	ebx,edx
	dec	ecx
	or	ecx,ecx
	jnz	.loop_program_headers

	jmp	run_kernel

	.target_page: dd 0
	.physical_page: dd 0

run_kernel:
	; Get the start address of the kernel
	mov	ebx,[kernel_buffer]
	mov	ecx,[ebx + 24]

	; Let the kernel use the memory that was used to store the executable
	xor	eax,eax
	mov	ebx,[load_memory_map.pointer]
	mov	[memory_map + ebx + 4],eax
	mov	[memory_map + ebx + 12],eax
	mov	[memory_map + ebx + 16],eax
	mov	[memory_map + ebx + 20],eax
	mov	eax,[memory_map + ebx + 8]
	mov	[memory_map + ebx + 24],eax
	mov	eax,[memory_map + ebx + 12]
	mov	[memory_map + ebx + 28],eax
	mov	eax,[kernel_buffer]
	mov	[memory_map + ebx],eax
	mov	eax,[kernel_size]
	shr	eax,12
	mov	[memory_map + ebx + 8],eax

	; Map the first MB at 0xFF000000 --> 0xFF100000
	mov	eax,[0xFFFFF000]
	mov	[0xFFFFFFF0],eax
	mov	eax,cr3
	mov	cr3,eax

	; Use the new linear address of the GDT
	add	dword [gdt_data.gdt2],0xFF000000
	lgdt	[gdt_data.gdt]

	; Execute the kernel's _start function
	jmp	ecx

setup_elf_64:
	; Check that the processor is 64-bit
	mov	ecx,0x80000001
	cpuid
	mov	esi,error_no_long_mode
	test	eax,0x20000000
	jnz	error_32

	; Disable paging
	mov	eax,cr0
	and	eax,0x7FFFFFFF
	mov	cr0,eax

	; Identity map the first 4MB
	mov	dword [page_table_allocation_location],0x5000
	mov	ecx,page_directory_length
	xor	eax,eax
	mov	edi,page_directory 
	rep 	stosb
	mov	dword [page_directory + 0x1000 - 8],page_directory | 3
	mov	dword [page_directory],(page_directory + 0x1000) | 7
	mov	dword [page_directory + 0x1000],(page_directory + 0x2000) | 7
	mov	dword [page_directory + 0x2000],(page_directory + 0x3000) | 7
	mov	dword [page_directory + 0x2000 + 8],(page_directory + 0x4000) | 7
	mov	edi,page_directory + 0x3000
	mov	eax,0x000003
	mov	ebx,0x200003
	mov	ecx,0x400
	.identity_loop:
	mov	[edi],eax
	add	edi,8
	add	eax,0x1000
	loop	.identity_loop
	mov	eax,page_directory
	mov	cr3,eax

	; Enable long mode
	mov	eax,cr4
	or	eax,32
	mov	cr4,eax
	mov	ecx,0xC0000080
	rdmsr
	or	eax,256
	wrmsr
	mov	eax,cr0
	or	eax,0x80000000
	mov	cr0,eax

	; Go to 64-bit mode
	jmp	0x48:.start_64_bit_mode
[bits 64]
	.start_64_bit_mode:

	mov	rax,0x50
	mov	ds,rax
	mov	es,rax
	mov	ss,rax
	
	; Check the ELF data is correct
	mov	rbx,[kernel_buffer]
	mov	rsi,error_bad_kernel
	cmp	byte [rbx + 5],1
	jne	error_64
	cmp	byte [rbx + 7],0
	jne	error_64
	cmp	byte [rbx + 16],2
	jne	error_64
	cmp	byte [rbx + 18],0x3E
	jne	error_64

	; Find the program headers
	; RAX = ELF header, RBX = program headers
	mov	rax,rbx
	mov	rbx,[rax + 32]
	add	rbx,rax

	; ECX = entries, EDX = size of entry
	movzx	rcx,word [rax + 56]
	movzx	rdx,word [rax + 54]

	; Loop through each program header
	.loop_program_headers:
	push	rax
	push	rcx
	push	rdx
	push	rbx

	; Only deal with load segments
	mov	eax,[rbx]
	cmp	eax,1
	jne	.next_entry

	; Work out how many physical pages we need to allocate
	mov	rcx,[rbx + 40]
	shr	rcx,12
	inc	rcx

	; Get the starting virtual address
	mov	rax,[rbx + 16]
	shl	rax,16
	shr	rax,16
	mov	[.target_page],rax

	; For every frame in the segment
	.frame_loop:
	xor	rbx,rbx

	; For every memory region
	.memory_region_loop:

	; Check the region has enough pages remaining
	mov	rax,[rbx + memory_map + 8]
	or	rax,rax
	jz	.try_next_memory_region

	; Remove one page from the region
	mov	rax,[rbx + memory_map + 0]
	mov	[.physical_page],rax
	add	rax,0x1000
	mov	[rbx + memory_map + 0],rax
	sub	qword [rbx + memory_map + 8],1

	jmp	.found_physical_page

	; Go to the next memory region
	.try_next_memory_region:
	add	rbx,16
	mov	eax,[load_memory_map.pointer]
	cmp	ebx,eax
	jne	.memory_region_loop
	mov	si,error_no_memory
	jmp	error_64

	; Map the page into virtual memory
	.found_physical_page:
	; Make sure we have a PDP
	mov	rax,[.target_page]
	shr	rax,39
	mov	rbx,[0xFFFFFFFFFFFFF000 + rax * 8]
	cmp	rbx,0
	jne	.has_pdp
	mov	rbx,[page_table_allocation_location]
	add	rbx,page_directory
	or	rbx,7
	mov	[0xFFFFFFFFFFFFF000 + rax * 8],rbx
	add	qword [page_table_allocation_location],0x1000
	mov	rax,cr3
	mov	cr3,rax
	.has_pdp:

	; Make sure we have a PD
	mov	rax,[.target_page]
	shr	rax,30
	mov	rbx,[0xFFFFFFFFFFE00000 + rax * 8]
	cmp	rbx,0
	jne	.has_pd
	mov	rbx,[page_table_allocation_location]
	add	rbx,page_directory
	or	rbx,7
	mov	[0xFFFFFFFFFFE00000 + rax * 8],rbx
	add	qword [page_table_allocation_location],0x1000
	mov	rax,cr3
	mov	cr3,rax
	.has_pd:

	; Make sure we have a PT
	mov	rax,[.target_page]
	shr	rax,21
	mov	rbx,[0xFFFFFFFFC0000000 + rax * 8]
	cmp	rbx,0
	jne	.has_pt
	mov	rbx,[page_table_allocation_location]
	add	rbx,page_directory
	or	rbx,7
	mov	[0xFFFFFFFFC0000000 + rax * 8],rbx
	add	qword [page_table_allocation_location],0x1000
	mov	rax,cr3
	mov	cr3,rax
	.has_pt:

	; Map the page!
	mov	rax,[.target_page]
	shr	rax,12
	mov	rbx,[.physical_page]
	or	rbx,0x103
	shl	rax,3
	xor	r8,r8
	mov	r8,0xFFFFFF80
	shl	r8,32
	add	rax,r8
	mov	[rax],rbx
	mov	rbx,[.target_page]
	invlpg	[rbx]

	; Go to the next frame
	add	qword [.target_page],0x1000
	dec	rcx
	jnz	.frame_loop

	; Restore the pointer to the segment
	pop	rbx
	push	rbx

	; Clear the memory
	mov	rcx,[rbx + 40]
	xor	rax,rax
	mov	rdi,[rbx + 16]
	rep	stosb

	; Copy the memory
	mov	rcx,[rbx + 32]
	mov	rsi,[rbx + 8]
	add	rsi,[kernel_buffer]
	mov	rdi,[ebx + 16]
	rep	movsb

	; Go to the next entry
	.next_entry:
	pop	rbx
	pop	rdx
	pop	rcx
	pop	rax

	add	rbx,rdx
	dec	rcx
	or	rcx,rcx
	jnz	.loop_program_headers

	jmp	run_kernel64

	.target_page: dq 0
	.physical_page: dq 0

run_kernel64:
	; Get the start address of the kernel
	mov	rbx,[kernel_buffer]
	mov	rcx,[rbx + 24]

	; Let the kernel use the memory that was used to store the executable
	xor	eax,eax
	mov	ebx,[load_memory_map.pointer]
	mov	[memory_map + ebx + 4],eax
	mov	[memory_map + ebx + 12],eax
	mov	[memory_map + ebx + 16],eax
	mov	[memory_map + ebx + 20],eax
	mov	eax,[memory_map + ebx + 8]
	mov	[memory_map + ebx + 24],eax
	mov	eax,[memory_map + ebx + 12]
	mov	[memory_map + ebx + 28],eax
	mov	eax,[kernel_buffer]
	mov	[memory_map + ebx],eax
	mov	eax,[kernel_size]
	shr	eax,12
	mov	[memory_map + ebx + 8],eax

	; Map the first MB at 0xFFFFFF0000000000 --> 0xFFFFFF0000100000 
	mov	rax,[0xFFFFFFFFFFFFF000]
	mov	[0xFFFFFFFFFFFFFFF0],rax
	mov	rax,cr3
	mov	cr3,rax

	; Use the new linear address of the GDT
	mov	rax,0xFFFFFF0000000000
	add	qword [gdt_data.gdt2],rax
	lgdt	[gdt_data.gdt]

	; Execute the kernel's _start function
	jmp	rcx

error_64:
	jmp	far [.error_32_ind]
	.error_32_ind: dq error_32
		       dd 8

[bits 32]
error_32:
	; Switch to 16-bit mode
	mov	eax,cr0
	and	eax,0x7FFFFFFF
	mov	cr0,eax
	jmp	0x18:.rmode

	; Switch to real mode
	[bits 16]
	.rmode:
	mov	eax,cr0
	and	eax,0x7FFFFFFE
	mov	cr0,eax
	jmp	0x0:.finish

	; Go to error
	.finish:
	mov	ax,0
	mov	ds,ax
	mov	es,ax
	mov	ss,ax
	jmp	error

check_a20:
	; Set the carry flag if the A20 line is enabled
	mov	ax,0
	mov	es,ax
	mov	ax,0xFFFF
	mov	fs,ax
	mov	byte [es:0x600],0
	mov	byte [fs:0x610],0xFF
	cmp	byte [es:0x600],0xFF
	je	.enabled
	stc
	ret
	.enabled: 
	clc
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

gdt_data:
	.null_entry:	dq 0
	.code_entry:	dd 0xFFFF	; 0x08
			db 0
			dw 0xCF9A
			db 0
	.data_entry:	dd 0xFFFF	; 0x10
			db 0
			dw 0xCF92
			db 0
	.code_entry_16:	dd 0xFFFF	; 0x18
			db 0
			dw 0x0F9A
			db 0
	.data_entry_16:	dd 0xFFFF	; 0x20
			db 0
			dw 0x0F92
			db 0
	.user_code:	dd 0xFFFF	; 0x2B
			db 0
			dw 0xCFFA
			db 0
	.user_data:	dd 0xFFFF	; 0x33
			db 0
			dw 0xCFF2
			db 0
	.tss:		dd 0x68		; 0x38
			db 0
			dw 0xE9
			db 0
			dq 0
	.code_entry64:	dd 0xFFFF	; 0x48
			db 0
			dw 0xAF9A
			db 0
	.data_entry64:	dd 0xFFFF	; 0x50
			db 0
			dw 0xAF92
			db 0
	.user_code64:	dd 0xFFFF	; 0x5B
			db 0
			dw 0xAFFA
			db 0
	.user_data64:	dd 0xFFFF	; 0x63
			db 0
			dw 0xAFF2
			db 0
	.user_code64c:	dd 0xFFFF	; 0x6B
			db 0
			dw 0xAFFA
			db 0
	.gdt:		dw (gdt_data.gdt - gdt_data - 1)
	.gdt2:		dq gdt_data

	; @FilesystemSpecific
	FilesystemSpecificCode

load_sectors: 
	; Load CX sectors from sector EAX to EDI
	pushad
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
	call	move_sector_to_target

	; Go to the next sector
	popad
	add	edi,0x200
	inc	eax
	loop	load_sectors
	ret

	; Move data from the temporary load buffer to EDI
move_sector_to_target:
	push	ss
	push	ds
	push	es

	; Switch to protected mode
	cli
	mov	eax,cr0
	or	eax,0x80000001
	mov	cr0,eax
	jmp	0x8:.pmode

	; Set the data segment registers
	[bits 32]
	.pmode:
	mov	ax,0x10
	mov	ds,ax
	mov	es,ax
	mov	ss,ax

	; Copy the data
	mov	ecx,0x200
	mov	esi,temporary_load_buffer
	rep	movsb

	; Switch to 16-bit mode
	mov	eax,cr0
	and	eax,0x7FFFFFFF
	mov	cr0,eax
	jmp	0x18:.rmode

	; Switch to real mode
	[bits 16]
	.rmode:
	mov	eax,cr0
	and	eax,0x7FFFFFFE
	mov	cr0,eax
	jmp	0x0:.finish

	; Return to load_sectors
	.finish:
	pop	es
	pop	ds
	pop	ss
	sti
	ret

read_structure: 
	; Data for the extended read calls
	dw	0x10
	dw	1
	dd 	temporary_load_buffer
	.lba:	dq 0

drive_number: db 0
partition_entry: dw 0

page_table_allocation_location: dq 0x2000

kernel_buffer: dq 0
kernel_size: dq 0

error_cannot_enable_a20_line: db "Error: Cannot enable the A20 line",0
error_could_not_get_memory_map: db "Error: Could not get the memory map from the BIOS",0
error_no_pci: db "Error: Could not find the PCI bus",0
error_no_cpuid: db "Error: CPU does not have the CPUID instruction",0
error_no_msr: db "Error: CPU does not have the RDMSR instruction",0
error_cannot_find_file: db "Error: A file necessary for booting could not found",0
error_cannot_read_disk: db "Error: The disk could not be read.",0
error_file_too_large: db "Error: The file was too large to be loaded (more than 256KB).",0
error_kernel_too_large: db "Error: The kernel was too large for the 3MB buffer.",0
error_bad_kernel: db "Error: Invalid or unsupported kernel ELF format.",0
error_no_memory: db "Error: Not enough memory to load kernel"
error_no_long_mode: db "Error: The kernel is compiled for a 64-bit processor but the current processor is 32-bit only.",0
error_could_not_set_video_mode: db "Error: Could not set video mode 1024x768x24.",0

reached_end: db "Reached end of stage 2 bootloader.",0
