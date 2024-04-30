section .data
	SYS_READ equ 0
	SYS_WRITE equ 1
	STDIN equ 0
	STDOUT equ 1
	NL equ 10
	SPS equ 32
	SYS_EXIT equ 60
	dec_val dd 10
	err_prompt db "not a valid instruction!!",10,0
	err_prompt_len equ $ - err_prompt
	bye db "bye",10,0
	bin_str db "0b"
	oct_str db "0o"
	hex_str db "0x"
section .bss
	num_string resb 64 
	num_binary resb 130
	num_octal resb 44 
	num_hex resb 34
	type resb 1 ;0 for bin, 1 for octal, 2 for hex

section .text
	global _start
_start:
	mov rax, SYS_READ
	mov rdi, STDIN
	mov rsi, num_string
	mov rdx, 64 
	syscall
	
	mov rdi, num_string
	call check_quit
	call stoi
	call num_to_binary
	call num_to_hex
	call num_to_oct
	
	mov rax, SYS_WRITE
	mov rdi, STDOUT
	cmp byte[type], 0
	jne .oct
	mov rsi, bin_str
	mov rdx, 2
	syscall
	mov rsi, num_binary
.oct:
	cmp byte[type], 1
	jne .hex
	mov rsi, oct_str
	mov rdx, 2
	syscall
	mov rsi, num_octal
.hex:
	cmp byte[type], 2
	jne .end
	mov rsi, hex_str 
	mov rdx, 2
	syscall
	mov rsi, num_hex
.end:
	mov rax, SYS_WRITE
	mov rdi, STDOUT
	call find_start
	call strlen
	syscall
	jmp _start	


	;function find_start
	;save the start in %rsi
find_start:
	movzx rdx, byte[rsi]
	cmp rdx, '0'
	jne .done
	inc rsi
	jmp find_start
.done:
	movzx rdx, byte[rsi]
	cmp rdx, NL
	jne .ret
	dec rsi
.ret:
	ret


check_quit:
	movzx rdx, byte[rdi]
	movzx rbx, byte[rdi+1]
	cmp rdx, 'q'
	jne .done
	cmp rbx, NL
	jne .done
	call exit
.done:
	ret	


	;function stoi
	;parameter %rdi as the num_string
	;no return value. the value is saved in r8(higher), r9(lower)
stoi:
	xor rcx, rcx
	xor r8, r8
	xor r9, r9
.nxt:
	movzx rdx, byte[rdi+rcx]
	cmp rdx, SPS
	jz .done
	sub rdx, '0'
	cmp rdx, 9
	jg error
	cmp rdx, 0
	jl error
		;no error, execute step 1
	mov rsi, 1
	shl rsi, 63
	and rsi, r9
	shl r8, 1
	test rsi, rsi
	jz .zero
	add r8, 1
.zero:
	shl r9, 1
		;step 2
	xor r10, r10
	xor r11, r11
	mov r10, r8
	mov r11, r9
		;step 3
	call lshiftr10r11
	call lshiftr10r11
		;step 4
	add r8, r10
	add r9, r11
	jnc .carry
	add r8, 1
.carry:	
		;step 5
	add r9, rdx 
	jnc .carry1
	add r8, 1
.carry1:
	inc rcx
	jmp .nxt
.done:
	inc rcx
	movzx rdx, byte[rdi+rcx]
	cmp rdx, 'o'
	je .oct
	cmp rdx, 'h'
	je .hex
	cmp rdx, 'b'
	jne error
	mov byte[type], 0
	jmp .end
.oct:
	mov byte[type], 1
	jmp .end
.hex:
	mov byte[type], 2
.end:
	inc rcx
	movzx rdx, byte[rdi+rcx]
	cmp rdx, NL
	jne error
	ret


lshiftr10r11:
	mov rsi, 1
	shl rsi, 63
	and rsi, r11
	shl r10, 1
	test rsi, rsi
	jz .zero
	add r10, 1
.zero:
	shl r11, 1
	ret


	;function num_to_binary
	;no parameter, the num integer read from r8, r9 
	;no return value, the binary string stored in num_binary
num_to_binary:
	mov rsi, num_binary
	lea rax, [rsi+129]
	mov byte[rax], 0
	dec rax
	mov byte[rax], NL
	mov rcx, 1
	mov rdx, 0 ;counter
.nxt:
	dec rax
	inc rdx
	cmp rdx, 65
	jne .not_refresh
	mov rcx, 1
.not_refresh:
	xor rdi, rdi
	mov rdi, rcx
	cmp rdx, 65
	jl .r9
	and rdi, r8
	jmp .r8
.r9:	
	and rdi, r9
.r8:
	test rdi, rdi
	je .iszero
	mov byte[rax], '1'
	jmp .else
.iszero:
	mov byte[rax], '0'
.else:
	cmp rax, rsi
	je .ret
	shl rcx, 1
	jmp .nxt
.ret:
	ret	


num_to_hex:
 	mov rsi, num_binary
	mov rcx, num_hex
	lea r9, [rcx+32]
	xor rdi, rdi
.loop:
	mov rdx, 1
	cmp rdi, 10
	jl .number
	add rdi, 87 
	jmp .alpha
.number:
	add rdi, '0'
.alpha:
	mov byte[rcx], dil 
	xor rdi, rdi
	cmp rcx, r9
	je .ret
	inc rcx
.loop1:	
	cmp rdx, 5
	je .loop
	mov rax, 1
	shl eax, 4
	mov r10, rcx ;save rcx
	mov rcx, rdx
	shr eax, cl
	mov rcx, r10 ;restore rcx 
	movzx r8, byte[rsi]
	sub r8, '0'
	imul r8d, eax
	add rdi, r8
	inc rdx
	inc rsi
	jmp .loop1	
.ret:
	inc rcx
	mov byte[rcx], NL
	ret


num_to_oct:
 	mov rsi, num_binary
	lea rsi, [rsi+2]
	mov rcx, num_octal
	lea r9, [rcx+42]
	xor rdi, rdi
.loop:
	mov rdx, 1
	add rdi, '0'
	mov byte[rcx], dil 
	xor rdi, rdi
	cmp rcx, r9
	je .ret
	inc rcx
.loop1:	
	cmp rdx, 4 
	je .loop
	mov rax, 1
	shl eax, 3
	mov r10, rcx ;save rcx
	mov rcx, rdx
	shr eax, cl
	mov rcx, r10 ;restore rcx 
	movzx r8, byte[rsi]
	sub r8, '0'
	imul r8d, eax
	add rdi, r8
	inc rdx
	inc rsi
	jmp .loop1	
.ret:
	inc rcx
	mov byte[rcx], NL
	ret


	;function strlen
	;rsi -> rdx
strlen:
	mov rcx, rsi
	xor rdx, rdx
.loop:
	cmp byte[rcx], NL 
	je .ret
	inc rcx
	inc rdx
	jmp .loop
.ret:
	inc rdx
	ret


error:
	mov rax, SYS_WRITE
	mov rdi, STDOUT
	mov rsi, err_prompt
	mov rdx, err_prompt_len
	syscall
	jmp _start
exit:
	mov rax, SYS_WRITE
	mov rdi, STDOUT
	mov rsi, bye
	mov rdx, 5
	syscall
	mov rax, SYS_EXIT
	xor rdi, rdi
	syscall	
