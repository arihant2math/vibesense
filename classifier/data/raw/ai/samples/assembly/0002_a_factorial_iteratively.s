.section .rodata
invalid_msg:
    .ascii "error: expected an integer from 0 to 20\n"
invalid_len = . - invalid_msg

.section .bss
    .lcomm input, 257
    .lcomm output, 32

.section .text
.globl _start

_start:
    mov $0, %rax
    mov $0, %rdi
    lea input(%rip), %rsi
    mov $256, %rdx
    syscall

    test %rax, %rax
    jle invalid_input

    lea input(%rip), %r12
    add %rax, %r12
    movb $0, (%r12)

    lea input(%rip), %rsi

skip_leading:
    cmp %r12, %rsi
    jae invalid_input
    movzbq (%rsi), %rax
    cmp $' ', %al
    je consume_leading
    cmp $'\t', %al
    je consume_leading
    cmp $'\n', %al
    je consume_leading
    cmp $'\r', %al
    je consume_leading
    jmp sign_check

consume_leading:
    inc %rsi
    jmp skip_leading

sign_check:
    cmpb $'+', (%rsi)
    jne digit_start
    inc %rsi

digit_start:
    xor %r14d, %r14d
    xor %r15d, %r15d

parse_digit:
    cmp %r12, %rsi
    jae digits_done

    movzbq (%rsi), %rax
    cmp $'0', %al
    jb digits_done
    cmp $'9', %al
    ja digits_done

    sub $'0', %al
    movzbq %al, %rcx

    cmp $2, %r14
    ja invalid_input
    jne accumulate
    test %rcx, %rcx
    jne invalid_input

accumulate:
    imul $10, %r14, %r14
    add %rcx, %r14
    inc %r15
    inc %rsi
    jmp parse_digit

digits_done:
    test %r15, %r15
    jz invalid_input

skip_trailing:
    cmp %r12, %rsi
    jae calculate
    movzbq (%rsi), %rax
    cmp $' ', %al
    je consume_trailing
    cmp $'\t', %al
    je consume_trailing
    cmp $'\n', %al
    je consume_trailing
    cmp $'\r', %al
    je consume_trailing
    jmp invalid_input

consume_trailing:
    inc %rsi
    jmp skip_trailing

calculate:
    mov $1, %rax
    mov $2, %rcx

factorial_loop:
    cmp %r14, %rcx
    ja factorial_done
    imul %rcx, %rax
    inc %rcx
    jmp factorial_loop

factorial_done:
    lea output+31(%rip), %rsi
    movb $'\n', (%rsi)
    dec %rsi
    mov $1, %r8

    test %rax, %rax
    jnz convert_loop
    movb $'0', (%rsi)
    dec %rsi
    inc %r8
    jmp write_result

convert_loop:
    xor %rdx, %rdx
    mov $10, %rbx
    div %rbx
    addb $'0', %dl
    mov %dl, (%rsi)
    dec %rsi
    inc %r8
    test %rax, %rax
    jnz convert_loop

write_result:
    inc %rsi
    mov $1, %rax
    mov $1, %rdi
    mov %r8, %rdx
    syscall

    mov $60, %rax
    xor %rdi, %rdi
    syscall

invalid_input:
    mov $1, %rax
    mov $2, %rdi
    lea invalid_msg(%rip), %rsi
    mov $invalid_len, %rdx
    syscall

    mov $60, %rax
    mov $1, %rdi
    syscall
