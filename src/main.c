#include "class.h"
#include "loader.h"
#include "opcode.h"
#include "util.h"

#include <argp.h>
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
void print_stack(Value_t* stack, size_t sp)
{
    debugfc(BOLD BLUE, "[ ");
    if (sp + 1 != 0)
        for (size_t i = 0; i <= sp; ++i)
            switch (stack[i].type) {
            case I:
                debugfc(BOLD BLUE, "I:%d ", stack[i].i);
                break;
            case L:
                debugfc(BOLD BLUE, "L:%ld ", stack[i].l);
                break;
            case F:
                debugfc(BOLD BLUE, "F:%f ", stack[i].f);
                break;
            case D:
                debugfc(BOLD BLUE, "D:%lf ", stack[i].d);
                break;
            case A:
                debugfc(BOLD BLUE, "A:%p ", stack[i].a);
                break;
            case ARR:
                panicf("unimplemented");
            }
    debugfc(BOLD BLUE, "]");
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
Value_t call_method(Method_t* m, Value_t const* args, size_t nr_args)
{
    Value_t ret;
    debugfc(BOLD YELLOW, "Entering function %s.%s (max stack %lu)\n", m->c->name, m->name, m->max_stack);

    if (m->flags & ACC_NATIVE) {
        if (!strcmp(m->c->name, "java/io/PrintStream") && !strcmp(m->name, "println")) {
            struct heh {
                Method_t** vtable;
                FILE* f;
            };
            fprintf(((struct heh*)args[0].a)->f, "%d\n", args[1].i);
        } else if (!strcmp(m->c->name, "java/lang/Object") && !strcmp(m->name, "<init>")) {
        }
    } else {
        Value_t* locals = malloc(sizeof(Value_t) * m->max_locals);
        Value_t* stack = malloc(sizeof(Value_t) * m->max_stack);

        // to placate valgrind
        {
            extern int debug;
            if (debug) {
                memset(locals, 0, sizeof(Value_t) * m->max_locals);
                memset(stack, 0, sizeof(Value_t) * m->max_stack);
            }
        }

        debugfc(BOLD YELLOW, "nr args: %lu\n", nr_args);
        memmove(locals, args, nr_args * sizeof(args[0]));

        Frame_t f = {
            .class = m->c,
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

    debugfc(BOLD YELLOW, "Exiting function %s.%s\n", m->c->name, m->name);

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

static inline Value_t get_value(void* a, enum ValueType type)
{
    switch (type) {
    case I:
        return makeI(*(int32_t*)a);
    case F:
        return makeF(*(float*)a);
    case A:
        return makeA(*(void**)a);
    case L:
        return makeL(*(int64_t*)a);
    case D:
        return makeD(*(double*)a);
    case ARR:
        panicf("unimplemented");
    default:
        __builtin_unreachable();
    }
}
static inline void set_value(void* a, Value_t v)
{
    switch (v.type) {
    case I:
        *(int32_t*)a = v.i;
        break;
    case F:
        *(float*)a = v.f;
        break;
    case A:
        *(void**)a = v.a;
        break;
    case L:
        *(int64_t*)a = v.l;
        break;
    case D:
        *(double*)a = v.d;
        break;
    case ARR:
        panicf("unimplemented");
    default:
        __builtin_unreachable();
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
            Field_t* f = resolve_fieldref(constant_pool_list, s);
            stack[++sp] = f->static_val;
        } break;
        case PUTSTATIC: {
            size_t s = (uint16_t)u2_from_big_endian(*(uint16_t*)&code[ip]);
            ip += 2;
            Field_t* f = resolve_fieldref(constant_pool_list, s);
            f->static_val = stack[sp--];
        } break;
        case GETFIELD: {
            size_t s = (uint16_t)u2_from_big_endian(*(uint16_t*)&code[ip]);
            ip += 2;
            Field_t* f = resolve_fieldref(constant_pool_list, s);

            void* a = (uint8_t*)stack[sp].a + f->offset;
            stack[sp] = get_value(a, get_value_type(f->desc[0]));
        } break;
        case PUTFIELD: {
            size_t s = (uint16_t)u2_from_big_endian(*(uint16_t*)&code[ip]);
            ip += 2;
            Field_t* f = resolve_fieldref(constant_pool_list, s);

            void* a = (uint8_t*)stack[sp - 1].a + f->offset;
            set_value(a, stack[sp]);
            sp -= 2;
        } break;

        case NEW: {
            size_t s = (uint16_t)u2_from_big_endian(*(uint16_t*)&code[ip]);
            ip += 2;
            Class_t* c = load_class(resolve_constant(constant_pool_list, s));
            Value_t v = makeA(malloc(c->size));
            *(Method_t***)v.a = c->vtable;
            stack[++sp] = v;
        } break;
        case INVOKEVIRTUAL: {
            size_t s = (uint16_t)u2_from_big_endian(*(uint16_t*)&code[ip]);
            ip += 2;

            Method_t* m = resolve_methodref(constant_pool_list, s);

            struct desc_info info = parse_desc(m->desc);
            info.nr_args++;

            Value_t* args = &stack[sp - info.nr_args + 1];

            // vtabe lookup
            m = (*(Method_t***)args[0].a)[m->vtable_offset];

            Value_t ret = call_method(m, args, info.nr_args);
            sp -= info.nr_args;
            if (info.returns)
                stack[++sp] = ret;
        } break;
        case INVOKESPECIAL: {
            size_t s = (uint16_t)u2_from_big_endian(*(uint16_t*)&code[ip]);
            ip += 2;
            Method_t* m = resolve_methodref(constant_pool_list, s);

            struct desc_info info = parse_desc(m->desc);
            info.nr_args++;

            Value_t* args = &stack[sp - info.nr_args + 1];

            Value_t ret = call_method(m, args, info.nr_args);
            sp -= info.nr_args;
            if (info.returns)
                stack[++sp] = ret;
        } break;
        case INVOKESTATIC: {
            size_t s = (uint16_t)u2_from_big_endian(*(uint16_t*)&code[ip]);
            ip += 2;
            Method_t* m = resolve_methodref(constant_pool_list, s);

            struct desc_info info = parse_desc(m->desc);

            Value_t* args = &stack[sp - info.nr_args + 1];

            Value_t ret = call_method(m, args, info.nr_args);
            sp -= info.nr_args;
            if (info.returns)
                stack[++sp] = ret;
        } break;
        default:
            errorf("unrecognised opcode 0x%x", op);
        }

        print_stack(stack, sp);
        debugf("\n");
    }
}

int main(int argc, char** argv)
{
    struct cmd_args cmd_args = parse_cmd_args(argc, argv);

    load_init();

    Class_t* c = load_class(cmd_args.main_class);
    Method_t* main_method = get_method(c, "main");

    call_method(main_method, NULL, 0);

    load_end();

    return 0;
}
