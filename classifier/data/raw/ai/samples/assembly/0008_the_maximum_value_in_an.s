.section .data
array:
    .long 12, 45, 7, 89, 23, 56
length:
    .long 6
format:
    .asciz "Maximum value: %d\n"

.section .text
.globl main
.extern printf

main:
    pushq %rbp
    movq %rsp, %rbp

    leaq array(%rip), %rcx
    movl length(%rip), %edx
    movl (%rcx), %eax
    addq $4, %rcx
    decl %edx

find_max:
    testl %edx, %edx
    jz print_result

    movl (%rcx), %esi
    cmpl %eax, %esi
    jle next_element
    movl %esi, %eax

next_element:
    addq $4, %rcx
    decl %edx
    jmp find_max

print_result:
    movl %eax, %esi
    leaq format(%rip), %rdi
    xorl %eax, %eax
    call printf

    xorl %eax, %eax
    leave
    ret.section .data
array:
    .long 12, 45, 7, 89, 23, 56
length:
    .long 6
format:
    .asciz "Maximum value: %d\n"

.section .text
.globl main
.extern printf

main:
    pushq %rbp
    movq %rsp, %rbp

    leaq array(%rip), %rcx
    movl length(%rip), %edx
    movl (%rcx), %eax
    addq $4, %rcx
    decl %edx

find_max:
    testl %edx, %edx
    jz print_result

    movl (%rcx), %esi
    cmpl %eax, %esi
    jle next_element
    movl %esi, %eax

next_element:
    addq $4, %rcx
    decl %edx
    jmp find_max

print_result:
    movl %eax, %esi
    leaq format(%rip), %rdi
    xorl %eax, %eax
    call printf

    xorl %eax, %eax
    leave
    ret
