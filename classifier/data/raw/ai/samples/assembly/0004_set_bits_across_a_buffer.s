.text
.globl count_set_bits
.type count_set_bits, @function

count_set_bits:
    xorq    %rax, %rax
    testq   %rsi, %rsi
    jz      .done

.next_element:
    movq    (%rdi), %rcx
    addq    $8, %rdi
    decq    %rsi

.count_bits:
    testq   %rcx, %rcx
    jz      .element_done
    leaq    -1(%rcx), %rdx
    andq    %rdx, %rcx
    incq    %rax
    jmp     .count_bits

.element_done:
    testq   %rsi, %rsi
    jnz     .next_element

.done:
    ret

.size count_set_bits, .-count_set_bits

