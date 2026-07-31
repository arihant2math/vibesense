.text
.globl factorial
.type factorial, @function

factorial:
    testq   %rdi, %rdi
    js      .overflow

    movq    %rdi, %rcx
    movq    $1, %rax

.loop:
    cmpq    $1, %rcx
    jbe     .done

    movq    %rax, %r8
    movq    $-1, %rax
    xorq    %rdx, %rdx
    divq    %rcx
    cmpq    %r8, %rax
    jb      .overflow

    movq    %r8, %rax
    imulq   %rcx, %rax
    decq    %rcx
    jmp     .loop

.done:
    ret

.overflow:
    movq    $-1, %rax
    ret

.size factorial, .-factorial
