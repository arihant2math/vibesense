.text
.globl compare_memory
.type compare_memory, @function
compare_memory:
    testq   %rdx, %rdx
    je      .equal

.loop:
    movzbq  (%rdi), %rax
    movzbq  (%rsi), %rcx
    cmpq    %rcx, %rax
    jne     .different

    incq    %rdi
    incq    %rsi
    decq    %rdx
    jne     .loop

.equal:
    xorl    %eax, %eax
    ret

.different:
    jb      .less
    movl    $1, %eax
    ret

.less:
    movl    $-1, %eax
    ret

.size compare_memory, .-compare_memory
