.section .rodata
msg:
    .asciz "Hello, world!\n"

.section .text
.globl _start
_start:
    mov $1, %rax
    mov $1, %rdi
    lea msg(%rip), %rsi
    mov $14, %rdx
    syscall

    mov $60, %rax
    xor %rdi, %rdi
    syscall
