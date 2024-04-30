global myPrint

section .text

myPrint:	
.loop:
	mov al, byte[rdi]
	test al, al
	jz .end
	mov rax, 1
	mov rsi, rdi
	mov rdi, 1
	mov rdx, 1
	syscall
	mov rdi, rsi
	inc rdi
	jmp .loop
.end:
	ret
