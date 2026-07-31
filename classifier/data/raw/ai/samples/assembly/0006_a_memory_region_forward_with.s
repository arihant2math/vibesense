.text
.globl memmove
.type memmove, @function
memmove:
    movq    %rdi, %rax
    testq   %rdx, %rdx
    je      .Ldone

    cmpq    %rsi, %rdi
    jb      .Lforward
    je      .Lforward

    movq    %rdi, %r8
    subq    %rsi, %r8
    cmpq    %rdx, %r8
    jae     .Lforward

    leaq    -1(%rdi,%rdx), %rdi
    leaq    -1(%rsi,%rdx), %rsi
    movq    %rdx, %rcx
    std
    rep movsb
    cld
    ret

.Lforward:
    movq    %rdx, %rcx
    rep movsb

.Ldone:
    ret
.size memmove, .-memmove
