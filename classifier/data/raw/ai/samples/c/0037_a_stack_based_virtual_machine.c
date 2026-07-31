#include <stdio.h>
#include <stdlib.h>

typedef enum {
    OP_PUSH,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_PRINT,
    OP_HALT
} Opcode;

typedef struct {
    Opcode opcode;
    int operand;
} Instruction;

#define STACK_SIZE 256

typedef struct {
    int stack[STACK_SIZE];
    int sp;
    int ip;
} VM;

static void push(VM *vm, int value) {
    if (vm->sp >= STACK_SIZE) {
        fprintf(stderr, "Error: stack overflow\n");
        exit(EXIT_FAILURE);
    }
    vm->stack[vm->sp++] = value;
}

static int pop(VM *vm) {
    if (vm->sp <= 0) {
        fprintf(stderr, "Error: stack underflow\n");
        exit(EXIT_FAILURE);
    }
    return vm->stack[--vm->sp];
}

static void execute(VM *vm, const Instruction *program) {
    for (;;) {
        Instruction instruction = program[vm->ip++];

        switch (instruction.opcode) {
            case OP_PUSH:
                push(vm, instruction.operand);
                break;

            case OP_ADD: {
                int b = pop(vm);
                int a = pop(vm);
                push(vm, a + b);
                break;
            }

            case OP_SUB: {
                int b = pop(vm);
                int a = pop(vm);
                push(vm, a - b);
                break;
            }

            case OP_MUL: {
                int b = pop(vm);
                int a = pop(vm);
                push(vm, a * b);
                break;
            }

            case OP_DIV: {
                int b = pop(vm);
                int a = pop(vm);
                if (b == 0) {
                    fprintf(stderr, "Error: division by zero\n");
                    exit(EXIT_FAILURE);
                }
                push(vm, a / b);
                break;
            }

            case OP_PRINT:
                printf("%d\n", pop(vm));
                break;

            case OP_HALT:
                return;

            default:
                fprintf(stderr, "Error: unknown opcode\n");
                exit(EXIT_FAILURE);
        }
    }
}

int main(void) {
    const Instruction program[] = {
        { OP_PUSH, 10 },
        { OP_PUSH, 5 },
        { OP_ADD, 0 },
        { OP_PUSH, 3 },
        { OP_MUL, 0 },
        { OP_PUSH, 5 },
        { OP_SUB, 0 },
        { OP_PRINT, 0 },
        { OP_HALT, 0 }
    };

    VM vm = { .sp = 0, .ip = 0 };
    execute(&vm, program);

    return 0;
}
