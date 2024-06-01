#include "class.h"
#include "loader.h"
#include "util.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void _Noreturn error(char const* restrict msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    exit(1);
}

typedef struct {
    int32_t l;
} Local_t;
typedef struct {
    int32_t s;
} Stack_t;

typedef struct {
    Class_t const* class;
    size_t ip;
    uint8_t const* code;
    Local_t* locals;
    size_t sp; // points to first free entry on stack
    Stack_t* stack;
} Frame_t;

enum opcode {
    ILOAD_0 = 26,
    ILOAD_1 = 27,
    ILOAD_2 = 28,
    ILOAD_3 = 29,
    IADD = 96,
    IRETURN = 172,
};

void print_stack(Stack_t* stack, size_t sp)
{
    printf("[ ");
    for (size_t i = 0; i < sp; ++i)
        printf("%u ", stack[i].s);
    printf("]");
}

uint32_t exec(Frame_t* f)
{
    Stack_t* stack = f->stack;
    Local_t* locals = f->locals;
    while (1) {
        uint8_t op = f->code[f->ip];

        printf("OP:0x%02x Stack: ", op);
        print_stack(stack, f->sp);
        printf("\n");

        switch (op) {
        case ILOAD_0:
        case ILOAD_1:
        case ILOAD_2:
        case ILOAD_3:
            stack[f->sp++].s = locals[op - ILOAD_0].l;
            break;
        case IADD: {
            uint32_t a = stack[f->sp - 1].s, b = stack[f->sp - 2].s;
            stack[f->sp - 2].s = a + b;
            f->sp--;
        } break;
        case IRETURN: {
            uint32_t v = stack[f->sp - 1].s;
            f->sp--;
            return v;
        }
        }
        f->ip++;
    }
}

int main(int argc, char** argv)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <Java classfile>\n", argv[0]);
        return 1;
    }

    Class_t c = load_class(argv[1]);
    print_class(&c);

    Method_t* method = &c.methods.list[1];
    Local_t* locals = malloc(sizeof(Local_t) * method->attrs.list[0].attr_code.max_locals);
    locals[0].l = 2;
    locals[1].l = 3;
    Frame_t f = {
        &c,
        0,
        method->attrs.list[0].attr_code.code,
        locals,
        0,
        malloc(sizeof(Stack_t) * method->attrs.list[0].attr_code.max_stack),
    };
    printf("%u\n", exec(&f));

    return 0;
}
