[bits 64]

[section .bss]

align 16

%define stack_size 16384
stack: resb stack_size

%define idt_size 4096
idt_data: resb idt_size

%define cpu_local_storage_size 8192
; Array of pointers to the CPU local states
[global cpu_local_storage]
cpu_local_storage: resb cpu_local_storage_size

[section .data]

idt:
	.limit: dw idt_size - 1
	.base:  dq idt_data

cpu_local_storage_index:
	dq 0

[global physicalMemoryRegions]
physicalMemoryRegions:
	dq 0xFFFFFF0000060000 
[global physicalMemoryRegionsCount]
physicalMemoryRegionsCount:
	dq 0
[global physicalMemoryRegionsPagesCount]
physicalMemoryRegionsPagesCount:
	dq 0
[global physicalMemoryOriginalPagesCount]
physicalMemoryOriginalPagesCount:
	dq 0
[global physicalMemoryRegionsIndex]
physicalMemoryRegionsIndex:
	dq 0
[global physicalMemoryHighest]
physicalMemoryHighest:
	dq 0

[global pagingNXESupport]
pagingNXESupport:
	dd 1
[global pagingPCIDSupport]
pagingPCIDSupport:
	dd 1
[global pagingSMEPSupport]
pagingSMEPSupport:
	dd 1
[global pagingTCESupport]
pagingTCESupport:
	dd 1
[global simdSSE3Support]
simdSSE3Support:
	dd 1
[global simdSSSE3Support]
simdSSSE3Support:
	dd 1

align 16
[global processorGDTR]
processorGDTR:
	dq 0
	dq 0

[section .text]

[global _start]
_start:
	mov	rax,0x63
	mov	fs,ax
	mov	gs,ax

	; Load the installation ID.
[extern installationID]
	mov	rbx,installationID
	mov	rax,[0x7FF0]
	mov	[rbx],rax
	mov	rax,[0x7FF8]
	mov	[rbx + 8],rax

	; Unmap the identity paging the bootloader used
	mov	rax,0xFFFFFFFFFFFFF000
	mov	qword [rax],0
	mov	rax,cr3
	mov	cr3,rax

	; Install a stack
	mov	rsp,stack + stack_size

SetupCOM1:
	; Setup the serial COM1 port for debug output.
	mov	dx,0x3F8 + 1
	mov	al,0x00
	out	dx,al
	mov	dx,0x3F8 + 3
	mov	al,0x80
	out	dx,al
	mov	dx,0x3F8 + 0
	mov	al,0x03
	out	dx,al
	mov	dx,0x3F8 + 1
	mov	al,0x00
	out	dx,al
	mov	dx,0x3F8 + 3
	mov	al,0x03
	out	dx,al
	mov	dx,0x3F8 + 2
	mov	al,0xC7
	out	dx,al
	mov	dx,0x3F8 + 4
	mov	al,0x0B
	out	dx,al

InstallIDT:
	; Remap the ISRs sent by the PIC to 0x20 - 0x2F
	; Even though we'll mask the PIC to use the APIC,
	; we have to do this so that the spurious interrupts
	; are set to a sane vector range.
	mov	al,0x11
	out	0x20,al
	mov	al,0x11
	out	0xA0,al
	mov	al,0x20
	out	0x21,al
	mov	al,0x28
	out	0xA1,al
	mov	al,0x04
	out	0x21,al
	mov	al,0x02
	out	0xA1,al
	mov	al,0x01
	out	0x21,al
	mov	al,0x01
	out	0xA1,al
	mov	al,0x00
	out	0x21,al
	mov	al,0x00
	out	0xA1,al

	; Install the interrupt handlers
%macro INSTALL_INTERRUPT_HANDLER 1
	mov	rbx,(%1 * 16) + idt_data
	mov	rdx,InterruptHandler%1
	call	InstallInterruptHandler
%endmacro
%assign i 0
%rep 256
	INSTALL_INTERRUPT_HANDLER i
%assign i i+1
%endrep

	; Save the location of the bootstrap GDT
	mov	rcx,processorGDTR
	sgdt	[rcx]

MemoryCalculations:
	; Work out basic information about the physical memory map we got from the bootloader
	mov	rdi,0xFFFFFF0000060000 - 0x10
	mov	rsi,0xFFFFFF0000060000
	xor	rax,rax
	xor	r8,r8
	.loop:
	add	rdi,0x10
	mov	r9,[rdi + 8]
	shl	r9,12
	add	r9,[rdi]
	cmp	r9,r8
	jb	.lower
	mov	r8,r9
	.lower:
	add	rax,[rdi + 8]
	cmp	qword [rdi],0
	jne	.loop
	mov	rbx,[rdi + 8]
	sub	rax,rbx
	sub	rdi,rsi
	shr	rdi,4
	mov	rsi,physicalMemoryRegionsCount
	mov	[rsi],rdi
	mov	rsi,physicalMemoryRegionsPagesCount
	mov	[rsi],rax
	mov	rsi,physicalMemoryOriginalPagesCount
	mov	[rsi],rbx
	mov	rsi,physicalMemoryHighest
	mov	[rsi],r8

DisablePIC:
	; Disable the PIC by masking all its interrupts, as we're going to use the APIC instead.
	; For some reason, it'll still generate spurious interrupts, so we'll have to ignore those.
	mov	al,0xFF
	out	0xA1,al
	out	0x21,al

StartKernel:
	; First stage of processor initilisation
	call	SetupProcessor1

	; Call the KernelMain function
	and	rsp,~0xF
	extern	KernelMain
	call	KernelMain

	; This function shouldn't return
	; ...let's crash.
	int	0

SetupProcessor1:
EnableCPUFeatures:
	; Enable no-execute support, if available
	mov	eax,0x80000001
	cpuid
	and	edx,1 << 20
	shr	edx,20
	mov	rax,pagingNXESupport
	and	[rax],edx
	cmp	edx,0
	je	.no_paging_nxe_support
	mov	ecx,0xC0000080
	rdmsr
	or	eax,1 << 11
	wrmsr
	.no_paging_nxe_support:

	; Enable SMEP support, if available
	; This prevents the kernel from executing userland pages
	; TODO Test this: neither Bochs or Qemu seem to support it?
	xor	eax,eax
	cpuid
	cmp	eax,7
	jb	.no_smep_support
	mov	eax,7
	xor	ecx,ecx
	cpuid
	and	ebx,1 << 7
	shr	ebx,7
	mov	rax,pagingSMEPSupport
	and	[rax],ebx
	cmp	ebx,0
	je	.no_smep_support
	mov	word [rax],2
	mov	rax,cr4
	or	rax,1 << 20
	mov	cr4,rax
	.no_smep_support:

	; Enable PCID support, if available
	mov	eax,1
	xor	ecx,ecx
	cpuid
	and	ecx,1 << 17
	shr	ecx,17
	mov	rax,pagingPCIDSupport
	and	[rax],ecx
	cmp	ecx,0
	je	.no_pcid_support
	mov	rax,cr4
	or	rax,1 << 17
	mov	cr4,rax
	.no_pcid_support:

	; Enable global pages
	mov	rax,cr4
	or	rax,1 << 7
	mov	cr4,rax

	; Enable TCE support, if available
	mov	eax,0x80000001
	xor	ecx,ecx
	cpuid
	and	ecx,1 << 17
	shr	ecx,17
	mov	rax,pagingTCESupport
	and	[rax],ecx
	cmp	ecx,0
	je	.no_tce_support
	mov	ecx,0xC0000080
	rdmsr
	or	eax,1 << 15
	wrmsr
	.no_tce_support:

	; Enable MMX, SSE and SSE2
	; These features are all guaranteed to be present on a x86_64 CPU
	mov	rax,cr0
	mov	rbx,cr4
	and	rax,~4
	or	rax,2
	or	rbx,512 + 1024
	mov	cr0,rax
	mov	cr4,rbx

	; Detect SSE3 and SSSE3, if available.
	mov	eax,1
	cpuid
	test	ecx,1 << 0
	jnz	.has_sse3
	mov	rax,simdSSE3Support
	and	byte [rax],0
	.has_sse3:
	test	ecx,1 << 9
	jnz	.has_ssse3
	mov	rax,simdSSSE3Support
	and	byte [rax],0
	.has_ssse3:

	; Enable system-call extensions (SYSCALL and SYSRET).
	mov	ecx,0xC0000080
	rdmsr
	or	eax,1
	wrmsr
	add	ecx,1
	rdmsr
	mov	edx,0x005B0048
	wrmsr
	add	ecx,1
	mov	rdx,SyscallEntry
	mov	rax,rdx
	shr	rdx,32
	wrmsr
	add	ecx,2
	rdmsr
	mov	eax,(1 << 10) | (1 << 9) ; Clear direction and interrupt flag when we enter ring 0.
	wrmsr

SetupCPULocalStorage:
	mov	ecx,0xC0000100
	mov	rax,cpu_local_storage
	mov	rdx,cpu_local_storage
	shr	rdx,32
	mov	rdi,cpu_local_storage_index
	add	rax,[rdi]
	add	qword [rdi],32 ; Space for 4 8-byte values at fs:0 - fs:31
	wrmsr

LoadIDTR:
	; Load the IDTR
	mov	rax,idt
	lidt	[rax]
	sti

EnableAPIC:
	; Enable the APIC!
	; Since we're on AMD64, we know that the APIC will be present.
	mov	ecx,0x1B
	rdmsr
	and	eax,~0xFFF
	mov	edi,eax
	test	eax,1 << 8
	jne	$
	or	eax,0x900
	wrmsr

	; Set the spurious interrupt vector to 0xFF
	mov	rax,0xFFFFFF00000000F0
	add	rax,rdi
	mov	ebx,[rax]
	or	ebx,0x1FF
	mov	[rax],ebx

	; Use the flat processor addressing model
	mov	rax,0xFFFFFF00000000E0
	add	rax,rdi
	mov	dword [rax],0xFFFFFFFF

	; Make sure that no external interrupts are masked
	xor	rax,rax
	mov	cr8,rax

	ret

SyscallEntry:
	mov	rsp,[fs:8]
	sti

	mov	ax,0x50
	mov	ds,ax
	mov	es,ax

	; Preserve RCX, R11, R12 and RBX.
	push	rcx
	push	r11
	push	r12
	push	rbx

	; Arguments in RDI, RSI, RDX, R8, R9. (RCX contains return address).
	; Return value in RAX.
	[extern Syscall]
	mov	rbx,rsp
	and	rsp,~0xF
	call	Syscall
	mov	rsp,rbx

	; Disable maskable interrupts.
	cli

	; Return to long mode. (Address in RCX).
	push	rax
	mov	ax,0x63
	mov	ds,ax
	mov	es,ax
	pop	rax
	pop	rbx
	pop	r12
	pop	r11
	pop	rcx
	db	0x48 
	sysret

[global ProcessorFakeTimerInterrupt]
ProcessorFakeTimerInterrupt:
	int	0x40
	ret

[global ProcessorDisableInterrupts]
ProcessorDisableInterrupts:
	mov	rax,14
	mov	cr8,rax
	; cli
	sti
	ret

[global ProcessorEnableInterrupts]
ProcessorEnableInterrupts:
	; WARNING: Changing this mechanism also requires update in x86_64.cpp, when deciding if we should re-enable interrupts on exception.

	mov	rax,0
	mov	cr8,rax
	sti
	ret

[global ProcessorAreInterruptsEnabled]
ProcessorAreInterruptsEnabled:
	pushf	
	pop	rax
	and	rax,0x200
	shr	rax,9

	mov	rdx,cr8
	cmp	rdx,0
	je	.done
	mov	rax,0
	.done:

	; pushf
	; pop	rax
	; and	rax,0x200
	; shr	rax,9
	ret

[global ProcessorHalt]
ProcessorHalt:
	cli
	hlt
	jmp	ProcessorHalt

[global ProcessorOut8]
ProcessorOut8:
	mov	rdx,rdi
	mov	rax,rsi
	out	dx,al
	ret

[global ProcessorIn8]
ProcessorIn8:
	mov	rdx,rdi
	xor	rax,rax
	in	al,dx
	ret

[global ProcessorOut16]
ProcessorOut16:
	mov	rdx,rdi
	mov	rax,rsi
	out	dx,ax
	ret

[global ProcessorIn16]
ProcessorIn16:
	mov	rdx,rdi
	xor	rax,rax
	in	ax,dx
	ret

[global ProcessorOut32]
ProcessorOut32:
	mov	rdx,rdi
	mov	rax,rsi
	out	dx,eax
	ret

[global ProcessorIn32]
ProcessorIn32:
	mov	rdx,rdi
	xor	rax,rax
	in	eax,dx
	ret

[global ProcessorInvalidatePage]
ProcessorInvalidatePage:
	invlpg	[rdi]
	ret

[global ProcessorIdle]
ProcessorIdle:
	sti
	hlt
	jmp	ProcessorIdle

[global GetLocalStorage]
GetLocalStorage:
	mov	rax,[fs:0]
	ret

[global GetCurrentThread]
GetCurrentThread:
	mov	rax,[fs:16]
	ret

[global ProcessorSetLocalStorage]
ProcessorSetLocalStorage:
	mov	[fs:0],rdi
	ret

InstallInterruptHandler:
	mov	word [rbx + 0],dx
	mov	word [rbx + 2],0x48
	mov	word [rbx + 4],0x8E00 
	shr	rdx,16
	mov	word [rbx + 6],dx
	shr	rdx,16
	mov	qword [rbx + 8],rdx
	ret

%macro INTERRUPT_HANDLER 1
InterruptHandler%1:
	push	dword 0 ; A fake error code
	push	dword %1 ; The interrupt number
	jmp	ASMInterruptHandler
%endmacro

%macro INTERRUPT_HANDLER_EC 1
InterruptHandler%1:
	; The CPU already pushed an error code
	push	dword %1 ; The interrupt number
	jmp	ASMInterruptHandler
%endmacro

INTERRUPT_HANDLER 0
INTERRUPT_HANDLER 1
INTERRUPT_HANDLER 2
INTERRUPT_HANDLER 3
INTERRUPT_HANDLER 4
INTERRUPT_HANDLER 5
INTERRUPT_HANDLER 6
INTERRUPT_HANDLER 7
INTERRUPT_HANDLER_EC 8
INTERRUPT_HANDLER 9
INTERRUPT_HANDLER_EC 10
INTERRUPT_HANDLER_EC 11
INTERRUPT_HANDLER_EC 12
INTERRUPT_HANDLER_EC 13
INTERRUPT_HANDLER_EC 14
INTERRUPT_HANDLER 15
INTERRUPT_HANDLER 16
INTERRUPT_HANDLER_EC 17
INTERRUPT_HANDLER 18
INTERRUPT_HANDLER 19
INTERRUPT_HANDLER 20
INTERRUPT_HANDLER 21
INTERRUPT_HANDLER 22
INTERRUPT_HANDLER 23
INTERRUPT_HANDLER 24
INTERRUPT_HANDLER 25
INTERRUPT_HANDLER 26
INTERRUPT_HANDLER 27
INTERRUPT_HANDLER 28
INTERRUPT_HANDLER 29
INTERRUPT_HANDLER 30
INTERRUPT_HANDLER 31

%assign i 32
%rep 224
INTERRUPT_HANDLER i
%assign i i+1
%endrep

ASMInterruptHandler:
	cld

	push	rax
	push	rbx
	push	rcx
	push	rdx
	push	rsi
	push	rdi
	push	rbp
	push	r8
	push	r9
	push	r10
	push	r11
	push	r12
	push	r13
	push	r14
	push	r15

	mov	rax,cr8
	push	rax

	mov	rax,0x123456789ABCDEF
	push	rax

	mov	rbx,rsp
	and	rsp,~0xF
	fxsave	[rsp - 512]
	mov	rsp,rbx
	sub	rsp,512 + 16
	
	xor	rax,rax
	mov	ax,ds
	push	rax
	mov	ax,0x10
	mov	ds,ax
	mov	es,ax
	mov	rax,cr2
	push	rax

	mov	rdi,rsp
	mov	rbx,rsp
	and	rsp,~0xF
	extern	InterruptHandler
	call	InterruptHandler
	mov	rsp,rbx

ReturnFromInterruptHandler:
	add	rsp,8
	pop	rax
	mov	ds,ax
	mov	es,ax

	add	rsp,512 + 16
	mov	rbx,rsp
	and	rbx,~0xF
	fxrstor	[rbx - 512]

	pop	rax
	mov	rbx,0x123456789ABCDEF
	cmp	rax,rbx
	jne	$

	cli	
	pop	rax
	mov	cr8,rax

	pop	r15
	pop	r14
	pop	r13
	pop	r12
	pop	r11
	pop	r10
	pop	r9
	pop	r8
	pop	rbp
	pop	rdi
	pop	rsi
	pop	rdx
	pop	rcx
	pop	rbx
	pop	rax

	add	rsp,16
	iretq

[global ProcessorSetAddressSpace]
ProcessorSetAddressSpace:
	mov	rax,cr3
	cmp	rax,rdi
	je	.cont
	mov	cr3,rdi
	.cont:
	ret

[global ProcessorGetAddressSpace]
ProcessorGetAddressSpace:
	mov	rax,cr3
	ret

[extern PostContextSwitch]
[global DoContextSwitch]
DoContextSwitch:
	cli
	mov	[fs:16],rcx
	mov	[fs:8],rdx
	mov	rax,cr3
	cmp	rax,rsi
	je	.cont
	mov	cr3,rsi
	.cont:
	mov	rsp,rdi
	call	PostContextSwitch
	jmp	ReturnFromInterruptHandler

[global ProcessorMagicBreakpoint]
ProcessorMagicBreakpoint:
	xchg	bx,bx
[global ProcessorBreakpointHelper]
ProcessorBreakpointHelper:
	ret

[global ProcessorReadCR3]
ProcessorReadCR3:
	mov	rax,cr3
	ret

[global SSSE3Framebuffer32To24Copy]
SSSE3Framebuffer32To24Copy:
 	; RDX - Pixels / 4
 	; RDI - Destination
 	; RSI - Source
 
 	mov	rax,.shuffle_data
 	lddqu	xmm2,[rax]
 	.loop:
 	lddqu	xmm1,[rsi]
 	pshufb	xmm1,xmm2
 	movdqu	[rdi],xmm1
 	add	rdi,12
 	add	rsi,16
 	sub	rdx,1
 	jnz	.loop
 	ret
 
 	.shuffle_data: db 0, 1, 2, 4, 5, 6, 8, 9, 10, 12, 13, 14, -1, -1, -1, -1

[global ProcessorDebugOutputByte]
ProcessorDebugOutputByte:
	mov	dx,0x3F8 + 5
	.WaitRead:
	in	al,dx
	and	al,0x20
	cmp	al,0
	je	.WaitRead
	mov	dx,0x3F8 + 0
	mov	rax,rdi
	out	dx,al
	ret

[global ProcessorReadTimeStamp]
ProcessorReadTimeStamp:
	rdtsc
	shl	rdx,32
	or	rax,rdx
	ret

[global ProcessorInstallTSS]
ProcessorInstallTSS:
	push	rbx

	; Set the location of the TSS in the GDT.
	mov	rax,rdi
	mov	rbx,rsi
	mov	[rax + 56 + 2],bx
	shr	rbx,16
	mov	[rax + 56 + 4],bl
	shr	rbx,8
	mov	[rax + 56 + 7],bl
	shr	rbx,8
	mov	[rax + 56 + 8],rbx

	; Flush the GDT.
	mov	rax,gdt_data.gdt2
	mov	rdx,[rax]
	mov	[rax],rdi
	mov	rdi,gdt_data.gdt
	lgdt	[rdi]
	mov	[rax],rdx

	; Flush the TSS.
	mov	ax,0x38
	ltr	ax

	pop	rbx
	ret

[global ProcessorAPStartup]
[bits 16]
ProcessorAPStartup: ; This function must be less than 4KB in length (see kernel/acpi.cpp)
	mov	ax,0x1000
	mov	ds,ax
	mov	byte [0xFC0],1 ; Indicate we've started.
	mov	eax,[0xFF0]
	mov	cr3,eax
	lgdt	[0x1000 + gdt_data.gdt - gdt_data]
	mov	eax,cr0
	or	eax,1
	mov	cr0,eax
	jmp	0x8:dword (.pmode - ProcessorAPStartup + 0x10000)
[bits 32]
	.pmode:
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
	jmp	0x48:(.start_64_bit_mode - ProcessorAPStartup + 0x10000)
[bits 64]
	.start_64_bit_mode:
	mov	rax,.start_64_bit_mode2
	jmp	rax
	.start_64_bit_mode2:
	mov	rax,0x50
	mov	ds,rax
	mov	es,rax
	mov	ss,rax
	mov	rax,0x63
	mov	fs,rax
	mov	gs,rax
	lgdt	[0x10FE0]
	mov	rsp,[0x10FD0]
	call	SetupProcessor1
	[extern SetupProcessor2]
	call	SetupProcessor2
	mov	byte [0x10FC0],2 ; Indicate the BSP can start the next processor.
	[extern KernelAPMain]
	and	rsp,~0xF
	call	KernelAPMain
	jmp	$

[global gdt_data]
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
	.gdt2:		dq 0x11000

