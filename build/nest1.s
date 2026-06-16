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
	sub	rsp, 144
	xor	eax, eax
	mov	QWORD PTR [rbp-8], rax
	mov	rax, QWORD PTR [rbp-8]
	mov	QWORD PTR [rbp-16], rax
	xor	eax, eax
	mov	QWORD PTR [rbp-24], rax
	mov	rax, QWORD PTR [rbp-24]
	mov	QWORD PTR [rbp-32], rax
cond_0:
	mov	rax, 3
	mov	QWORD PTR [rbp-40], rax
	mov	rax, QWORD PTR [rbp-32]
	mov	rbx, QWORD PTR [rbp-40]
	cmp	rax, rbx
	setl	al
	movzx	rax, al
	mov	QWORD PTR [rbp-48], rax
	cmp	QWORD PTR [rbp-48], 0
	jne	body_1
	jmp	end_2
body_1:
	xor	eax, eax
	mov	QWORD PTR [rbp-56], rax
	mov	rax, QWORD PTR [rbp-56]
	mov	QWORD PTR [rbp-64], rax
cond_3:
	mov	rax, 3
	mov	QWORD PTR [rbp-72], rax
	mov	rax, QWORD PTR [rbp-64]
	mov	rbx, QWORD PTR [rbp-72]
	cmp	rax, rbx
	setl	al
	movzx	rax, al
	mov	QWORD PTR [rbp-80], rax
	cmp	QWORD PTR [rbp-80], 0
	jne	body_4
	jmp	end_5
body_4:
	mov	rax, 1
	mov	QWORD PTR [rbp-88], rax
	mov	rax, QWORD PTR [rbp-64]
	mov	rbx, QWORD PTR [rbp-88]
	cmp	rax, rbx
	sete	al
	movzx	rax, al
	mov	QWORD PTR [rbp-96], rax
	cmp	QWORD PTR [rbp-96], 0
	jne	then_6
	jmp	end_8
then_6:
	jmp	end_5
	jmp	end_8
end_8:
	mov	rax, 1
	mov	QWORD PTR [rbp-104], rax
	mov	rax, QWORD PTR [rbp-16]
	mov	rbx, QWORD PTR [rbp-104]
	add	rax, rbx
	mov	QWORD PTR [rbp-112], rax
	mov	rax, QWORD PTR [rbp-112]
	mov	QWORD PTR [rbp-16], rax
	mov	rax, 1
	mov	QWORD PTR [rbp-120], rax
	mov	rax, QWORD PTR [rbp-64]
	mov	rbx, QWORD PTR [rbp-120]
	add	rax, rbx
	mov	QWORD PTR [rbp-128], rax
	mov	rax, QWORD PTR [rbp-128]
	mov	QWORD PTR [rbp-64], rax
	jmp	cond_3
end_5:
	mov	rax, 1
	mov	QWORD PTR [rbp-136], rax
	mov	rax, QWORD PTR [rbp-32]
	mov	rbx, QWORD PTR [rbp-136]
	add	rax, rbx
	mov	QWORD PTR [rbp-144], rax
	mov	rax, QWORD PTR [rbp-144]
	mov	QWORD PTR [rbp-32], rax
	jmp	cond_0
end_2:
	mov	rax, QWORD PTR [rbp-16]
	mov	rsp, rbp
	pop	rbp
	ret
