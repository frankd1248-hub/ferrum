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
	mov	rax, 1
	mov	QWORD PTR [rbp-8], rax
	mov	rax, QWORD PTR [rbp-8]
	mov	QWORD PTR [rbp-16], rax
	mov	rax, QWORD PTR [rbp-16]
	mov	QWORD PTR [rbp-24], rax
	mov	rax, QWORD PTR [rbp-24]
	mov	rsp, rbp
	pop	rbp
	ret
