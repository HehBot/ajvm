#include "class.h"
#include "loader.h"
#include "util.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef union {
    int32_t i;
    float f;
    void* a;

    int64_t l;
    double d;
} Value_t;

typedef struct {
    Class_t const* class;

    size_t ip; // points to next code byte to be processed
    uint8_t const* code;

    Value_t* locals;

    size_t sp; // points to top of the stack (-1 for empty stack)
    Value_t* stack;
} Frame_t;

enum opcode {
    NOP = 0x00,

    ACONST_NULL = 0x01,
    ICONST_M1 = 0x02,
    ICONST_0,
    ICONST_1,
    ICONST_2,
    ICONST_3,
    ICONST_4,
    ICONST_5,
    LCONST_0,
    LCONST_1,
    FCONST_0,
    FCONST_1,
    DCONST_0,
    DCONST_1,

    BIPUSH = 0x10,
    SIPUSH = 0x11,

    ILOAD = 0x15,
    LLOAD,
    FLOAD,
    DLOAD,
    ALOAD,
    ILOAD_0,
    ILOAD_1,
    ILOAD_2,
    ILOAD_3,
    LLOAD_0,
    LLOAD_1,
    LLOAD_2,
    LLOAD_3,
    FLOAD_0,
    FLOAD_1,
    FLOAD_2,
    FLOAD_3,
    DLOAD_0,
    DLOAD_1,
    DLOAD_2,
    DLOAD_3,
    ALOAD_0,
    ALOAD_1,
    ALOAD_2,
    ALOAD_3,

    ISTORE = 0x36,
    LSTORE,
    FSTORE,
    DSTORE,
    ASTORE,
    ISTORE_0,
    ISTORE_1,
    ISTORE_2,
    ISTORE_3,
    LSTORE_0,
    LSTORE_1,
    LSTORE_2,
    LSTORE_3,
    FSTORE_0,
    FSTORE_1,
    FSTORE_2,
    FSTORE_3,
    DSTORE_0,
    DSTORE_1,
    DSTORE_2,
    DSTORE_3,
    ASTORE_0,
    ASTORE_1,
    ASTORE_2,
    ASTORE_3,

    POP = 0x57,
    DUP = 0x59,
    SWAP = 0x5f,

    IADD = 0x60,
    LADD,
    FADD,
    DADD,
    ISUB,
    LSUB,
    FSUB,
    DSUB,
    IMUL,
    LMUL,
    FMUL,
    DMUL,
    IDIV,
    LDIV,
    FDIV,
    DDIV,
    IREM,
    LREM,
    FREM,
    DREM,
    INEG,
    LNEG,
    FNEG,
    DNEG,
    ISHL,
    LSHL,
    ISHR,
    LSHR,
    IUSHR,
    LUSHR,
    IAND,
    LAND,
    IOR,
    LOR,
    IXOR,
    LXOR,
    I2L,
    I2F,
    I2D,
    L2D,
    L2I,
    L2F,
    F2I,
    F2L,
    F2D,
    D2I,
    D2L,
    D2F,
    I2B,
    I2C,
    I2S,

    IRETURN = 0xac,
    LRETURN,
    FRETURN,
    DRETURN,
    ARETURN,
};

void print_stack(Value_t* stack, size_t sp)
{
    debugf("[ ");
    if (sp + 1 != 0)
        for (size_t i = 0; i <= sp; ++i)
            debugf("%u ", stack[i].i);
    debugf("]");
}

Value_t exec(Frame_t* f)
{
    Value_t* stack = f->stack;
    Value_t* locals = f->locals;
    size_t sp = f->sp;
    uint8_t const* code = f->code;
    size_t ip = f->ip;
    while (1) {
        enum opcode op = (enum opcode)code[ip++];

        debugf("OP:0x%02x Stack: ", op);
        print_stack(stack, sp);
        debugf("\n");

        switch (op) {
        case NOP:
            break;
        case ACONST_NULL:
            stack[++sp].a = NULL;
            break;
        case ICONST_M1:
        case ICONST_0:
        case ICONST_1:
        case ICONST_2:
        case ICONST_3:
        case ICONST_4:
        case ICONST_5:
            stack[++sp].i = (int32_t)op - (int32_t)ICONST_0;
            break;
        case LCONST_0:
            stack[++sp].l = 0l;
            break;
        case LCONST_1:
            stack[++sp].l = 1l;
            break;
        case FCONST_0:
            stack[++sp].f = 0.0f;
            break;
        case FCONST_1:
            stack[++sp].f = 1.0f;
            break;
        case DCONST_0:
            stack[++sp].d = 0.0;
            break;
        case DCONST_1:
            stack[++sp].d = 1.0;
            break;
        case BIPUSH:
            stack[++sp].i = (int8_t)code[ip];
            ip++;
            break;
        case SIPUSH:
            stack[++sp].i = (int16_t)u2_from_big_endian(*(uint16_t*)&code[ip]);
            ip += 2;
            break;
        case ILOAD:
        case LLOAD:
        case FLOAD:
        case DLOAD:
        case ALOAD:
            stack[++sp] = locals[code[ip]];
            ip++;
            break;
        case ISTORE:
        case LSTORE:
        case FSTORE:
        case DSTORE:
        case ASTORE:
            locals[code[ip]] = stack[sp--];
            ip++;
            break;
        case ILOAD_0:
        case ILOAD_1:
        case ILOAD_2:
        case ILOAD_3:
            stack[++sp].i = locals[op - ILOAD_0].i;
            break;
        case ISTORE_0:
        case ISTORE_1:
        case ISTORE_2:
        case ISTORE_3:
            locals[op - ISTORE_0].i = stack[sp--].i;
            break;
        case LLOAD_0:
        case LLOAD_1:
        case LLOAD_2:
        case LLOAD_3:
            stack[++sp].l = locals[op - LLOAD_0].l;
            break;
        case LSTORE_0:
        case LSTORE_1:
        case LSTORE_2:
        case LSTORE_3:
            locals[op - LSTORE_0].l = stack[sp--].l;
            break;
        case FLOAD_0:
        case FLOAD_1:
        case FLOAD_2:
        case FLOAD_3:
            stack[++sp].f = locals[op - FLOAD_0].f;
            break;
        case FSTORE_0:
        case FSTORE_1:
        case FSTORE_2:
        case FSTORE_3:
            locals[op - FSTORE_0].f = stack[sp--].f;
            break;
        case DLOAD_0:
        case DLOAD_1:
        case DLOAD_2:
        case DLOAD_3:
            stack[++sp].d = locals[op - DLOAD_0].d;
            break;
        case DSTORE_0:
        case DSTORE_1:
        case DSTORE_2:
        case DSTORE_3:
            locals[op - DSTORE_0].d = stack[sp--].d;
            break;
        case ALOAD_0:
        case ALOAD_1:
        case ALOAD_2:
        case ALOAD_3:
            stack[++sp].a = locals[op - ALOAD_0].a;
            break;
        case ASTORE_0:
        case ASTORE_1:
        case ASTORE_2:
        case ASTORE_3:
            locals[op - ASTORE_0].a = stack[sp--].a;
            break;
        case POP:
            sp--;
            break;
        case DUP:
            stack[sp + 1] = stack[sp];
            ++sp;
            break;
        case SWAP: {
            Value_t v = stack[sp];
            stack[sp - 1] = v;
            stack[sp] = stack[sp - 1];
        } break;
        case IADD:
        case ISUB:
        case IMUL:
        case IDIV:
        case IREM:
        case ISHL:
        case ISHR:
        case IUSHR:
        case IAND:
        case IOR:
        case IXOR: {
            int32_t a = stack[sp - 1].i, b = stack[sp].i, result;
            switch (op) {
            case IADD:
                result = a + b;
                break;
            case ISUB:
                result = a - b;
                break;
            case IMUL:
                result = a * b;
                break;
            case IDIV:
                result = a / b;
                break;
            case IREM:
                result = a % b;
                break;
            case ISHL:
                result = a << (uint8_t)(b & 0x1f);
                break;
            case ISHR:
                result = a >> (uint8_t)(b & 0x1f);
                break;
            case IUSHR:
                result = ((uint32_t)a) >> (uint8_t)(b & 0x1f);
                break;
            case IAND:
                result = a & b;
                break;
            case IOR:
                result = a | b;
                break;
            case IXOR:
                result = a ^ b;
                break;
            default:
                __builtin_unreachable();
            }
            stack[sp - 1].i = result;
            sp--;
        } break;
        case LADD:
        case LSUB:
        case LMUL:
        case LDIV:
        case LREM:
        case LSHL:
        case LSHR:
        case LUSHR:
        case LAND:
        case LOR:
        case LXOR: {
            int64_t a = stack[sp - 1].l, b = stack[sp].l, result;
            switch (op) {
            case LADD:
                result = a + b;
                break;
            case LSUB:
                result = a - b;
                break;
            case LMUL:
                result = a * b;
                break;
            case LDIV:
                result = a / b;
                break;
            case LREM:
                result = a % b;
                break;
            case LSHL:
                result = a << (uint8_t)(b & 0x3f);
                break;
            case LSHR:
                result = a >> (uint8_t)(b & 0x3f);
                break;
            case LUSHR:
                result = ((uint64_t)a) >> (uint8_t)(b & 0x3f);
                break;
            case LAND:
                result = a & b;
                break;
            case LOR:
                result = a | b;
                break;
            case LXOR:
                result = a ^ b;
                break;
            default:
                __builtin_unreachable();
            }
            stack[sp - 1].l = result;
            sp--;
        } break;
        case FADD:
        case FSUB:
        case FMUL:
        case FDIV:
        case FREM: {
            float a = stack[sp - 1].f, b = stack[sp].f, result;
            switch (op) {
            case FADD:
                result = a + b;
                break;
            case FSUB:
                result = a - b;
                break;
            case FMUL:
                result = a * b;
                break;
            case FDIV:
                result = a / b;
                break;
            case FREM:
                result = (float)fmod(a, b);
                break;
            default:
                __builtin_unreachable();
            }
            stack[sp - 2].f = result;
            sp--;
        } break;
        case DADD:
        case DSUB:
        case DMUL:
        case DDIV:
        case DREM: {
            double a = stack[sp - 1].d, b = stack[sp].d, result;
            switch (op) {
            case DADD:
                result = a + b;
                break;
            case DSUB:
                result = a - b;
                break;
            case DMUL:
                result = a * b;
                break;
            case DDIV:
                result = a / b;
                break;
            case DREM:
                result = fmod(a, b);
                break;
            default:
                __builtin_unreachable();
            }
            stack[sp - 1].d = result;
            sp--;
        } break;
        case INEG:
            stack[sp].i = -stack[sp].i;
            break;
        case LNEG:
            stack[sp].l = -stack[sp].l;
            break;
        case FNEG:
            stack[sp].f = -stack[sp].f;
            break;
        case DNEG:
            stack[sp].d = -stack[sp].d;
            break;
        case I2L:
            stack[sp].l = (int64_t)stack[sp].i;
            break;
        case I2F:
            stack[sp].f = (float)stack[sp].i;
            break;
        case I2D:
            stack[sp].d = (double)stack[sp].i;
            break;
        case L2I:
            stack[sp].i = (int32_t)stack[sp].l;
            break;
        case L2F:
            stack[sp].f = (float)stack[sp].l;
            break;
        case L2D:
            stack[sp].d = (double)stack[sp].l;
            break;
        case F2I:
            stack[sp].i = (int32_t)stack[sp].f;
            break;
        case F2L:
            stack[sp].l = (int64_t)stack[sp].f;
            break;
        case F2D:
            stack[sp].d = (double)stack[sp].f;
            break;
        case D2I:
            stack[sp].i = (int32_t)stack[sp].d;
            break;
        case D2L:
            stack[sp].l = (int64_t)stack[sp].d;
            break;
        case D2F:
            stack[sp].f = (float)stack[sp].d;
            break;
        case I2B:
            stack[sp].i = (int32_t)((uint32_t)stack[sp].i & 0xff);
            break;
        case I2C:
            stack[sp].i = (uint32_t)((uint32_t)stack[sp].i & 0xff);
            break;
        case I2S:
            stack[sp].i = (int32_t)((uint32_t)stack[sp].i & 0xffff);
            break;
        case IRETURN:
        case LRETURN:
        case FRETURN:
        case DRETURN:
        case ARETURN:
            return stack[sp--];
        default:
            errorf("Unrecognised opcode 0x%x", op);
        }
    }
}

int main(int argc, char** argv)
{
    if (argc != 2)
        errorf("Usage: %s <Main class>\n", argv[0]);

    Class_t* c = load_class(argv[1]);

    Method_t* main_method = NULL;
    for (size_t i = 0; i < c->methods.size; ++i) {
        if (strcmp(c->methods.list[i].name, "main") == 0) {
            main_method = &c->methods.list[i];
            break;
        }
    }
    if (main_method == NULL)
        errorf("unable to find main method in class %s", argv[1]);

    Value_t* locals = malloc(sizeof(Value_t) * main_method->attrs.list[0].attr_code.max_locals);
    Frame_t f = {
        c,
        0,
        main_method->attrs.list[0].attr_code.code,
        locals,
        -1,
        malloc(sizeof(Value_t) * main_method->attrs.list[0].attr_code.max_stack),
    };
    printf("%u\n", exec(&f).i);

    return 0;
}
