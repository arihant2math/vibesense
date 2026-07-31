.section .text
.globl parse_decimal
.type parse_decimal, @function

# int parse_decimal(const char *input, int64_t *output)
# Returns:
#   0 = success
#   1 = invalid input
#   2 = integer overflow
#   3 = null pointer
parse_decimal:
    testq   %rdi, %rdi
    jz      .null_error
    testq   %rsi, %rsi
    jz      .null_error

.skip_leading:
    movzbq  (%rdi), %rcx
    cmpb    $' ', %cl
    je      .consume_leading
    cmpb    $'\t', %cl
    je      .consume_leading
    cmpb    $'\n', %cl
    je      .consume_leading
    cmpb    $'\r', %cl
    je      .consume_leading
    cmpb    $'\v', %cl
    je      .consume_leading
    cmpb    $'\f', %cl
    je      .consume_leading
    jmp     .sign

.consume_leading:
    incq    %rdi
    jmp     .skip_leading

.sign:
    xorq    %r8, %r8
    movq    $7, %r11
    cmpb    $'-', %cl
    jne     .check_plus
    movq    $1, %r8
    movq    $8, %r11
    incq    %rdi
    jmp     .digits

.check_plus:
    cmpb    $'+', %cl
    jne     .digits
    incq    %rdi

.digits:
    xorq    %rax, %rax
    xorq    %rdx, %rdx

.digit_loop:
    movzbq  (%rdi), %rcx
    cmpb    $'0', %cl
    jb      .after_digits
    cmpb    $'9', %cl
    ja      .after_digits

    movq    %rcx, %r9
    subq    $'0', %r9

    movq    $-922337203685477580, %r10
    cmpq    %r10, %rax
    jl      .overflow
    jne     .safe_multiply
    cmpq    %r11, %r9
    ja      .overflow

.safe_multiply:
    imulq   $10, %rax, %rax
    subq    %r9, %rax
    movq    $1, %rdx
    incq    %rdi
    jmp     .digit_loop

.after_digits:
    testq   %rdx, %rdx
    jz      .invalid

.trailing:
    movzbq  (%rdi), %rcx
    testb   %cl, %cl
    jz      .finish
    cmpb    $' ', %cl
    je      .consume_trailing
    cmpb    $'\t', %cl
    je      .consume_trailing
    cmpb    $'\n', %cl
    je      .consume_trailing
    cmpb    $'\r', %cl
    je      .consume_trailing
    cmpb    $'\v', %cl
    je      .consume_trailing
    cmpb    $'\f', %cl
    je      .consume_trailing
    jmp     .invalid

.consume_trailing:
    incq    %rdi
    jmp     .trailing

.finish:
    testq   %r8, %r8
    jnz     .store_result
    negq    %rax

.store_result:
    movq    %rax, (%rsi)
    xorl    %eax, %eax
    ret

.invalid:
    movl    $1, %eax
    ret

.overflow:
    movl    $2, %eax
    ret

.null_error:
    movl    $3, %eax
    ret

.size parse_decimal, .-parse_decimal.section .note.GNU-stack,"",@progbits
