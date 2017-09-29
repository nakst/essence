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
