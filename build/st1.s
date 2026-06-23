.intel_syntax noprefix

.global _start
_start:
	call	main
	mov	rdi, rax
	mov	rax, 60
	syscall

.global getX
getX:
	push	rbp
	mov	rbp, rsp
	mov	r10, rdi
	mov	rax, r10
	mov	rcx, 0
	mov	rax, QWORD PTR [rax + rcx*8]
	mov	r11, rax
	mov	rax, r11
	mov	rsp, rbp
	pop	rbp
	ret
.global main
main:
	push	r13
	push	r12
	push	rbp
	mov	rbp, rsp
	sub	rsp, 16
	lea	rax, [rbp-8]
	mov	r12, rax
	mov	rax, r12
	mov	rcx, 0
	mov	rdx, 7
	mov	QWORD PTR [rax + rcx*8], rdx
	mov	rax, r12
	mov	rcx, 1
	mov	rdx, 8
	mov	QWORD PTR [rax + rcx*8], rdx
	mov	rdi, r12
	call	getX
	mov	r13, rax
	mov	rax, r13
	mov	rsp, rbp
	pop	rbp
	pop	r12
	pop	r13
	ret
