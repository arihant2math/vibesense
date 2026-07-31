.section .data
array:
    .quad 1, 2, 3, 4, 5
array_len:
    .quad 5

.section .text
.globl main
.type main, @function
main:
    pushq %rbp
    movq %rsp, %rbp

    leaq array(%rip), %rdi
    movq array_len(%rip), %rsi
    call sum_array

    movq %rax, %rdi
    movl $60, %eax
    syscall

.type sum_array, @function
sum_array:
    xorq %rax, %rax
    testq %rsi, %rsi
    jz .done

.loop:
    addq (%rdi), %rax
    addq $8, %rdi
    decq %rsi
    jnz .loop

.done:
    ret

.size sum_array, .-sum_array
.size main, .-main
