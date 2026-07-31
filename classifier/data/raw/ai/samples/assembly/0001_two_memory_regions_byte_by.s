.text
.globl compare_regions
.type compare_regions, @function

compare_regions:
    testq   %rdx, %rdx
    je      .equal

.loop:
    movzbl  (%rdi), %eax
    movzbl  (%rsi), %ecx
    cmpb    %cl, %al
    jne     .different

    incq    %rdi
    incq    %rsi
    decq    %rdx
    jne     .loop

.equal:
    xorl    %eax, %eax
    ret

.different:
    subl    %ecx, %eax
    ret

.size compare_regions, .-compare_regions
.section .note.GNU-stack,"",@progbits
