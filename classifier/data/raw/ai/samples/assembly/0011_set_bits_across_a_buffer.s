.text
.globl count_set_bits
.type count_set_bits, @function

count_set_bits:
    xorq    %rax, %rax
    movq    %rsi, %rcx
    shrq    $3, %rcx
    jz      .Ltail

.Lqword:
    popcntq  (%rdi), %rdx
    addq    %rdx, %rax
    addq    $8, %rdi
    decq    %rcx
    jnz     .Lqword

.Ltail:
    andq    $7, %rsi
    jz      .Ldone

.Lbyte:
    movzbl  (%rdi), %edx
    popcntq %rdx, %rdx
    addq    %rdx, %rax
    incq    %rdi
    decq    %rsi
    jnz     .Lbyte

.Ldone:
    ret

.size count_set_bits, .-count_set_bits
.section .note.GNU-stack,"",@progbits
