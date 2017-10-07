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
