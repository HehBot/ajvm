#include "class.h"
#include "loader.h"
#include "util.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    Class_t const* class;

    size_t ip; // points to next code byte to be processed
    uint8_t const* code;

    Value_t* locals;

    size_t sp; // points to top of the stack (-1 for empty stack)
    Value_t* stack;
} Frame_t;

// expansion macro for enum value definition
#define ENUM_VALUE(name, assign) name assign,
// expansion macro for enum to string conversion
#define ENUM_CASE(name, assign) \
    case name:                  \
        return #name;
// declare the access function and define enum values
#define DECLARE_ENUM(EnumType, ENUM_DEF) \
    enum EnumType {                      \
        ENUM_DEF(ENUM_VALUE)             \
    };
// define the access function names
#define DEFINE_ENUM_STRINGER(EnumType, ENUM_DEF) \
    char const* get_string(enum EnumType value)  \
    {                                            \
        switch (value) {                         \
            ENUM_DEF(ENUM_CASE)                  \
        default:                                 \
            return ""; /* handle input error */  \
        }                                        \
    }
#define OPCODE_ENUM(XX)       \
    XX(NOP, = 0x00)           \
    XX(ACONST_NULL, = 0x01)   \
    XX(ICONST_M1, = 0x02)     \
    XX(ICONST_0, )            \
    XX(ICONST_1, )            \
    XX(ICONST_2, )            \
    XX(ICONST_3, )            \
    XX(ICONST_4, )            \
    XX(ICONST_5, )            \
    XX(LCONST_0, )            \
    XX(LCONST_1, )            \
    XX(FCONST_0, )            \
    XX(FCONST_1, )            \
    XX(DCONST_0, )            \
    XX(DCONST_1, )            \
    XX(BIPUSH, = 0x10)        \
    XX(SIPUSH, = 0x11)        \
    XX(ILOAD, = 0x15)         \
    XX(LLOAD, )               \
    XX(FLOAD, )               \
    XX(DLOAD, )               \
    XX(ALOAD, )               \
    XX(ILOAD_0, )             \
    XX(ILOAD_1, )             \
    XX(ILOAD_2, )             \
    XX(ILOAD_3, )             \
    XX(LLOAD_0, )             \
    XX(LLOAD_1, )             \
    XX(LLOAD_2, )             \
    XX(LLOAD_3, )             \
    XX(FLOAD_0, )             \
    XX(FLOAD_1, )             \
    XX(FLOAD_2, )             \
    XX(FLOAD_3, )             \
    XX(DLOAD_0, )             \
    XX(DLOAD_1, )             \
    XX(DLOAD_2, )             \
    XX(DLOAD_3, )             \
    XX(ALOAD_0, )             \
    XX(ALOAD_1, )             \
    XX(ALOAD_2, )             \
    XX(ALOAD_3, )             \
    XX(ISTORE, = 0x36)        \
    XX(LSTORE, )              \
    XX(FSTORE, )              \
    XX(DSTORE, )              \
    XX(ASTORE, )              \
    XX(ISTORE_0, )            \
    XX(ISTORE_1, )            \
    XX(ISTORE_2, )            \
    XX(ISTORE_3, )            \
    XX(LSTORE_0, )            \
    XX(LSTORE_1, )            \
    XX(LSTORE_2, )            \
    XX(LSTORE_3, )            \
    XX(FSTORE_0, )            \
    XX(FSTORE_1, )            \
    XX(FSTORE_2, )            \
    XX(FSTORE_3, )            \
    XX(DSTORE_0, )            \
    XX(DSTORE_1, )            \
    XX(DSTORE_2, )            \
    XX(DSTORE_3, )            \
    XX(ASTORE_0, )            \
    XX(ASTORE_1, )            \
    XX(ASTORE_2, )            \
    XX(ASTORE_3, )            \
    XX(POP, = 0x57)           \
    XX(DUP, = 0x59)           \
    XX(SWAP, = 0x5f)          \
    XX(IADD, = 0x60)          \
    XX(LADD, )                \
    XX(FADD, )                \
    XX(DADD, )                \
    XX(ISUB, )                \
    XX(LSUB, )                \
    XX(FSUB, )                \
    XX(DSUB, )                \
    XX(IMUL, )                \
    XX(LMUL, )                \
    XX(FMUL, )                \
    XX(DMUL, )                \
    XX(IDIV, )                \
    XX(LDIV, )                \
    XX(FDIV, )                \
    XX(DDIV, )                \
    XX(IREM, )                \
    XX(LREM, )                \
    XX(FREM, )                \
    XX(DREM, )                \
    XX(INEG, )                \
    XX(LNEG, )                \
    XX(FNEG, )                \
    XX(DNEG, )                \
    XX(ISHL, )                \
    XX(LSHL, )                \
    XX(ISHR, )                \
    XX(LSHR, )                \
    XX(IUSHR, )               \
    XX(LUSHR, )               \
    XX(IAND, )                \
    XX(LAND, )                \
    XX(IOR, )                 \
    XX(LOR, )                 \
    XX(IXOR, )                \
    XX(LXOR, )                \
    XX(I2L, )                 \
    XX(I2F, )                 \
    XX(I2D, )                 \
    XX(L2D, )                 \
    XX(L2I, )                 \
    XX(L2F, )                 \
    XX(F2I, )                 \
    XX(F2L, )                 \
    XX(F2D, )                 \
    XX(D2I, )                 \
    XX(D2L, )                 \
    XX(D2F, )                 \
    XX(I2B, )                 \
    XX(I2C, )                 \
    XX(I2S, )                 \
    XX(IRETURN, = 0xac)       \
    XX(LRETURN, )             \
    XX(FRETURN, )             \
    XX(DRETURN, )             \
    XX(ARETURN, )             \
    XX(RETURN, )              \
    XX(GETSTATIC, = 0xb2)     \
    XX(PUTSTATIC, )           \
    XX(GETFIELD, )            \
    XX(PUTFIELD, )            \
    XX(INVOKEVIRTUAL, = 0xb6) \
    XX(INVOKESPECIAL, )       \
    XX(INVOKESTATIC, )        \
    XX(NEW, = 0xbb)
DECLARE_ENUM(opcode, OPCODE_ENUM)
DEFINE_ENUM_STRINGER(opcode, OPCODE_ENUM)

void print_stack(Value_t* stack, size_t sp)
{
    debugf("[ ");
    if (sp + 1 != 0)
        for (size_t i = 0; i <= sp; ++i)
            switch (stack[i].type) {
            case I:
                debugf("I:%d ", stack[i].i);
                break;
            case L:
                debugf("L:%ld ", stack[i].l);
                break;
            case F:
                debugf("F:%f ", stack[i].f);
                break;
            case D:
                debugf("D:%lf ", stack[i].d);
                break;
            case A:
                debugf("A:%p ", stack[i].a);
                break;
            case ARR:
                panicf("unimplemented");
            }
    debugf("]");
}

struct desc_info {
    uint16_t nr_args : 15;
    uint16_t returns : 1;
};

static struct desc_info parse_desc(char const* desc)
{
    struct desc_info info = { 0, 0 };
    char const* p = desc;
    ++p;
    for (; *p != ')'; ++p) {
        info.nr_args++;
        while (*p == '[')
            ++p;
        if (*p == 'L') {
            ++p;
            while (*p != ';')
                ++p;
        }
    }
    ++p;
    if (*p != 'V')
        info.returns = 1;
    return info;
}

Value_t exec(Frame_t* f);
Value_t call_method(Class_t* c, Method_t* m, Value_t const* args, size_t nr_args)
{
    Value_t ret;
    debugf("Executing function %s.%s (max stack %lu)\n", c->name, m->name, m->max_stack);

    if (m->flags & ACC_NATIVE) {
        if (!strcmp(c->name, "java/io/PrintStream") && !strcmp(m->name, "println"))
            fprintf((FILE*)args[0].a, "%d\n", args[1].i);
        else if (!strcmp(c->name, "java/lang/Object") && !strcmp(m->name, "<init>")) {
        }
    } else {
        Value_t* locals = malloc(sizeof(Value_t) * m->max_locals);
        Value_t* stack = malloc(sizeof(Value_t) * m->max_stack);

#ifndef NDEBUG
        // to placate valgrind
        memset(locals, 0, sizeof(Value_t) * m->max_locals);
        memset(stack, 0, sizeof(Value_t) * m->max_stack);
#endif

        debugf("nr args: %lu\n", nr_args);

        memmove(locals, args, nr_args * sizeof(args[0]));

        Frame_t f = {
            .class = c,
            .ip = 0,
            .code = m->code,
            .locals = locals,
            .sp = -1,
            .stack = stack,
        };
        ret = exec(&f);

        free(locals);
        free(stack);
    }

    debugf("Exiting function %s.%s\n", c->name, m->name);

    return ret;
}

static inline Value_t makeI(int32_t i)
{
    return (Value_t) { .type = I, .i = i };
}
static inline Value_t makeL(int64_t l)
{
    return (Value_t) { .type = L, .l = l };
}
static inline Value_t makeF(float f)
{
    return (Value_t) { .type = F, .f = f };
}
static inline Value_t makeD(double d)
{
    return (Value_t) { .type = D, .d = d };
}
static inline Value_t makeA(void* a)
{
    return (Value_t) { .type = A, .a = a };
}

size_t get_size(enum ValueType t)
{
    switch (t) {
    case I:
    case F:
        return 4;
    case L:
    case D:
    case A:
        return 8;
    case ARR:
        return 12;
    default:
        __builtin_unreachable();
    }
}
enum ValueType get_value_type(char desc)
{
    switch (desc) {
    case 'I':
        return I;
    case 'F':
        return F;
    case 'J':
        return L;
    case 'D':
        return D;
    case 'L':
        return A;
    case '[':
        return ARR;
    default:
        panicf("wtf");
    }
}

Value_t exec(Frame_t* f)
{
    Const_t* constant_pool_list = f->class->constant_pool.list;
    Value_t* stack = f->stack;
    Value_t* locals = f->locals;
    size_t sp = f->sp;
    uint8_t const* code = f->code;
    size_t ip = f->ip;

    print_stack(stack, sp);
    debugf("\n");

    while (1) {
        enum opcode op = (enum opcode)code[ip++];
        debugf("%s\n", get_string(op));

        switch (op) {
        case NOP:
            break;
        case ACONST_NULL:
            stack[++sp] = makeA(NULL);
            break;
        case ICONST_M1:
        case ICONST_0:
        case ICONST_1:
        case ICONST_2:
        case ICONST_3:
        case ICONST_4:
        case ICONST_5:
            stack[++sp] = makeI((int32_t)op - (int32_t)ICONST_0);
            break;
        case LCONST_0:
            stack[++sp] = makeL(0l);
            break;
        case LCONST_1:
            stack[++sp] = makeL(1l);
            break;
        case FCONST_0:
            stack[++sp] = makeF(0.0f);
            break;
        case FCONST_1:
            stack[++sp] = makeF(1.0f);
            break;
        case DCONST_0:
            stack[++sp] = makeD(0.0);
            break;
        case DCONST_1:
            stack[++sp] = makeD(1.0);
            break;
        case BIPUSH:
            stack[++sp] = makeI((int8_t)code[ip]);
            ip++;
            break;
        case SIPUSH:
            stack[++sp] = makeI((int16_t)u2_from_big_endian(*(uint16_t*)&code[ip]));
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
            stack[++sp] = locals[op - ILOAD_0];
            break;
        case ISTORE_0:
        case ISTORE_1:
        case ISTORE_2:
        case ISTORE_3:
            locals[op - ISTORE_0] = stack[sp--];
            break;
        case LLOAD_0:
        case LLOAD_1:
        case LLOAD_2:
        case LLOAD_3:
            stack[++sp] = locals[op - LLOAD_0];
            break;
        case LSTORE_0:
        case LSTORE_1:
        case LSTORE_2:
        case LSTORE_3:
            locals[op - LSTORE_0] = stack[sp--];
            break;
        case FLOAD_0:
        case FLOAD_1:
        case FLOAD_2:
        case FLOAD_3:
            stack[++sp] = locals[op - FLOAD_0];
            break;
        case FSTORE_0:
        case FSTORE_1:
        case FSTORE_2:
        case FSTORE_3:
            locals[op - FSTORE_0] = stack[sp--];
            break;
        case DLOAD_0:
        case DLOAD_1:
        case DLOAD_2:
        case DLOAD_3:
            stack[++sp] = locals[op - DLOAD_0];
            break;
        case DSTORE_0:
        case DSTORE_1:
        case DSTORE_2:
        case DSTORE_3:
            locals[op - DSTORE_0] = stack[sp--];
            break;
        case ALOAD_0:
        case ALOAD_1:
        case ALOAD_2:
        case ALOAD_3:
            stack[++sp] = locals[op - ALOAD_0];
            break;
        case ASTORE_0:
        case ASTORE_1:
        case ASTORE_2:
        case ASTORE_3:
            locals[op - ASTORE_0] = stack[sp--];
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
            stack[sp - 1] = makeI(result);
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
            stack[sp - 1] = makeL(result);
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
            stack[sp - 2] = makeF(result);
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
            stack[sp - 1] = makeD(result);
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
            stack[sp] = makeL((int64_t)stack[sp].i);
            break;
        case I2F:
            stack[sp] = makeF((float)stack[sp].i);
            break;
        case I2D:
            stack[sp] = makeD((double)stack[sp].i);
            break;
        case L2I:
            stack[sp] = makeI((int32_t)stack[sp].l);
            break;
        case L2F:
            stack[sp] = makeF((float)stack[sp].l);
            break;
        case L2D:
            stack[sp] = makeD((double)stack[sp].l);
            break;
        case F2I:
            stack[sp] = makeI((int32_t)stack[sp].f);
            break;
        case F2L:
            stack[sp] = makeL((int64_t)stack[sp].f);
            break;
        case F2D:
            stack[sp] = makeD((double)stack[sp].f);
            break;
        case D2I:
            stack[sp] = makeI((int32_t)stack[sp].d);
            break;
        case D2L:
            stack[sp] = makeL((int64_t)stack[sp].d);
            break;
        case D2F:
            stack[sp] = makeF((float)stack[sp].d);
            break;
        case I2B:
            stack[sp] = makeI((int32_t)((uint32_t)stack[sp].i & 0xff));
            break;
        case I2C:
            stack[sp] = makeI((uint32_t)((uint32_t)stack[sp].i & 0xff));
            break;
        case I2S:
            stack[sp] = makeI((int32_t)((uint32_t)stack[sp].i & 0xffff));
            break;
        case IRETURN:
        case LRETURN:
        case FRETURN:
        case DRETURN:
        case ARETURN:
            return stack[sp];
        case RETURN:
            return makeI(0);

        case GETSTATIC: {
            size_t s = (uint16_t)u2_from_big_endian(*(uint16_t*)&code[ip]);
            ip += 2;
            fieldref_t fieldref = resolve_fieldref(constant_pool_list, s);
            stack[++sp] = fieldref.f->static_val;
        } break;
        case PUTSTATIC: {
            size_t s = (uint16_t)u2_from_big_endian(*(uint16_t*)&code[ip]);
            ip += 2;
            fieldref_t fieldref = resolve_fieldref(constant_pool_list, s);
            fieldref.f->static_val = stack[sp--];
        } break;
        case GETFIELD: {
            size_t s = (uint16_t)u2_from_big_endian(*(uint16_t*)&code[ip]);
            ip += 2;
            fieldref_t fieldref = resolve_fieldref(constant_pool_list, s);

            void* a = (uint8_t*)stack[sp].a + fieldref.f->offset;
            Value_t v = { .type = get_value_type(fieldref.f->desc[0]) };
            memmove(&v.type + 1, a, get_size(v.type));
            stack[sp] = v;
        } break;
        case PUTFIELD: {
            size_t s = (uint16_t)u2_from_big_endian(*(uint16_t*)&code[ip]);
            ip += 2;
            fieldref_t fieldref = resolve_fieldref(constant_pool_list, s);

            void* a = (uint8_t*)stack[sp - 1].a + fieldref.f->offset;
            Value_t v = stack[sp];
            memmove(a, &v.type + 1, get_size(v.type));
            sp -= 2;
        } break;

        case NEW: {
            size_t s = (uint16_t)u2_from_big_endian(*(uint16_t*)&code[ip]);
            ip += 2;
            Class_t* c = load_class(resolve_constant(constant_pool_list, s));
            stack[++sp] = makeA(malloc(c->size));
        } break;
        case INVOKEVIRTUAL:
        case INVOKESPECIAL: {
            size_t s = (uint16_t)u2_from_big_endian(*(uint16_t*)&code[ip]);
            ip += 2;
            methodref_t methodref = resolve_methodref(constant_pool_list, s);

            struct desc_info info = parse_desc(methodref.m->desc);
            info.nr_args++;

            Value_t* args = &stack[sp - info.nr_args + 1];

            Value_t ret = call_method(methodref.c, methodref.m, args, info.nr_args);
            sp -= info.nr_args;
            if (info.returns)
                stack[++sp] = ret;
        } break;
        case INVOKESTATIC: {
            size_t s = (uint16_t)u2_from_big_endian(*(uint16_t*)&code[ip]);
            ip += 2;
            methodref_t methodref = resolve_methodref(constant_pool_list, s);

            struct desc_info info = parse_desc(methodref.m->desc);

            Value_t* args = &stack[sp - info.nr_args + 1];

            Value_t ret = call_method(methodref.c, methodref.m, args, info.nr_args);
            sp -= info.nr_args;
            if (info.returns)
                stack[++sp] = ret;
        } break;
        default:
            errorf("Unrecognised opcode 0x%x", op);
        }

        print_stack(stack, sp);
        debugf("\n");
    }
}

int main(int argc, char** argv)
{
    if (argc != 2)
        errorf("Usage: %s <Main class>\n", argv[0]);

    load_init();

    Class_t* c = load_class(argv[1]);
    Method_t* main_method = get_method(c, "main");

    call_method(c, main_method, NULL, 0);

    load_end();

    return 0;
}
