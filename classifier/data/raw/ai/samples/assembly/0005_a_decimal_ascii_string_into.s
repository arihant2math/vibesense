.section .rodata
input:
    .asciz "12345"

.section .bss
.lcomm output, 32

.section .text
.globl _start
.type _start, @function

_start:
    lea input(%rip), %rdi
    call parse_decimal

    mov %rax, %rdi
    lea output+31(%rip), %rsi
    movb $'\n', (%rsi)
    dec %rsi
    mov $10, %rcx

.convert:
    xor %rdx, %rdx
    div %rcx
    add $'0', %dl
    mov %dl, (%rsi)
    dec %rsi
    test %rax, %rax
    jnz .convert

    inc %rsi
    lea output+32(%rip), %rdx
    sub %rsi, %rdx

    mov $1, %rax
    mov $1, %rdi
    syscall

    mov $60, %rax
    xor %rdi, %rdi
    syscall

.type parse_decimal, @function
parse_decimal:
    xor %rax, %rax
    xor %rcx, %rcx

.parse:
    movzbq (%rdi), %rdx
    cmp $'0', %dl
    jb .done
    cmp $'9', %dl
    ja .done

    imul $10, %rax, %rax
    sub $'0', %dl
    add %rdx, %rax

    inc %rdi
    jmp .parse

.done:
    ret

.section .note.GNU-stack,"",@progbits
