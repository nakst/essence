[section .text]

[global _OSSyscall]
_OSSyscall:
	push	r12
	push	r11
	push	rcx
	mov	r12,rsp
	syscall
	mov	rsp,r12
	pop	rcx
	pop	r11
	pop	r12
	ret

[global sqrt]
sqrt:
	sqrtsd	xmm0,xmm0
	ret

[global OSFPInitialise]
OSFPInitialise:
	mov	rax,.init
	ldmxcsr	[rax]
	ret
.init:	dd	0x00001FC0

[global _setjmp]
_setjmp:
	mov	[rdi + 0x00],rsp
	mov	[rdi + 0x08],rbp
	mov	[rdi + 0x10],rbx
	mov	[rdi + 0x18],r12
	mov	[rdi + 0x20],r13
	mov	[rdi + 0x28],r14
	mov	[rdi + 0x30],r15
	mov	rax,[rsp]
	mov	[rdi + 0x38],rax
	xor	rax,rax
	ret

[global _longjmp]
_longjmp:
	mov	rsp,[rdi + 0x00]
	mov	rbp,[rdi + 0x08]
	mov	rbx,[rdi + 0x10]
	mov	r12,[rdi + 0x18]
	mov	r13,[rdi + 0x20]
	mov	r14,[rdi + 0x28]
	mov	r15,[rdi + 0x30]
	mov	rax,[rdi + 0x38]
	mov	[rsp],rax
	mov	rax,rsi
	cmp	rax,0
	jne	.return
	mov	rax,1
	.return:
	ret
