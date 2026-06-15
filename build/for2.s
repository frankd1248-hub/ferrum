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
	sub	rsp, 96
	lea	rax, [.LCs0 + rip]
	mov	QWORD PTR [rbp-8], rax
	xor	eax, eax
	mov	QWORD PTR [rbp-16], rax
	mov	rax, QWORD PTR [rbp-16]
	mov	QWORD PTR [rbp-24], rax
	xor	eax, eax
	mov	QWORD PTR [rbp-32], rax
	mov	rax, QWORD PTR [rbp-32]
	mov	QWORD PTR [rbp-40], rax
cond_0:
	mov	rax, 2
	mov	QWORD PTR [rbp-48], rax
	mov	rax, QWORD PTR [rbp-40]
	mov	rbx, QWORD PTR [rbp-48]
	cmp	rax, rbx
	setl	al
	movzx	rax, al
	mov	QWORD PTR [rbp-56], rax
	cmp	QWORD PTR [rbp-56], 0
	jne	body_1
	jmp	end_2
body_1:
	mov	rax, QWORD PTR [rbp-8]
	mov	rcx, QWORD PTR [rbp-40]
	movzx	eax, BYTE PTR [rax + rcx]
	mov	QWORD PTR [rbp-64], rax
	mov	rax, QWORD PTR [rbp-24]
	mov	rbx, QWORD PTR [rbp-64]
	add	rax, rbx
	mov	QWORD PTR [rbp-72], rax
	mov	rax, QWORD PTR [rbp-72]
	mov	QWORD PTR [rbp-24], rax
	mov	rax, 1
	mov	QWORD PTR [rbp-80], rax
	mov	rax, QWORD PTR [rbp-40]
	mov	rbx, QWORD PTR [rbp-80]
	add	rax, rbx
	mov	QWORD PTR [rbp-88], rax
	mov	rax, QWORD PTR [rbp-88]
	mov	QWORD PTR [rbp-40], rax
	jmp	cond_0
end_2:
	mov	rax, QWORD PTR [rbp-24]
	mov	rsp, rbp
	pop	rbp
	ret
.section .rodata
.text
.section .rodata
.LCs0_len:
	.quad 2
.LCs0:
	.string "Hi"
.text
