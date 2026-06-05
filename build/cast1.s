.intel_syntax noprefix

.global _start
_start:
	call	main
	mov	rdi, rax
	mov	rax, 60
	syscall

.global main
main:
	push	rbp
	mov	rbp, rsp
	sub	rsp, 32
	xor	eax, eax
	mov	QWORD PTR [rbp-8], rax
	mov	rax, QWORD PTR [rbp-8]
	mov	QWORD PTR [rbp-16], rax
	mov	rax, QWORD PTR [rbp-16]
	cmp	rax, 0
	setne	al
	movzx	rax, al
	mov	QWORD PTR [rbp-24], rax
	mov	rax, QWORD PTR [rbp-24]
	mov	QWORD PTR [rbp-32], rax
	mov	rax, QWORD PTR [rbp-32]
	mov	rsp, rbp
	pop	rbp
	ret
