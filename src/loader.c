#include "class.h"
#include "util.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint16_t u2_from_big_endian(uint16_t u)
{
    return ((u & 0xff) << 8 | (u & 0xff00) >> 8);
}
static uint32_t u4_from_big_endian(uint32_t u)
{
    return ((u & 0xff) << 24 | (u & 0xff00) << 8 | (u & 0xff0000) >> 8 | (u & 0xff000000) >> 24);
}

static void* bytes(FILE* f, size_t sz, int is_str)
{
    void* buf = malloc(sz + (is_str ? 1 : 0));
    fread(buf, 1, sz, f);
    if (is_str)
        ((uint8_t*)buf)[sz] = '\0';
    return buf;
}
static uint32_t read_big_endian_u4(FILE* f)
{
    uint32_t a;
    fread(&a, sizeof(a), 1, f);
    return u4_from_big_endian(a);
}
static uint16_t read_big_endian_u2(FILE* f)
{
    uint16_t a;
    fread(&a, sizeof(a), 1, f);
    return u2_from_big_endian(a);
}
static uint8_t read_big_endian_u1(FILE* f)
{
    uint8_t a;
    fread(&a, sizeof(a), 1, f);
    return a;
}

static char const* resolve_constant(Const_t* constant_pool_list, size_t i)
{
    Const_t* c = &constant_pool_list[i - 1];
    switch (c->tag) {
    case CONST_UTF8:
        return c->string;
    case CONST_STRING:
        return resolve_constant(constant_pool_list, c->string_index);
    case CONST_CLASS:
    case CONST_NAME_AND_TYPE:
        return resolve_constant(constant_pool_list, c->name_index);
    default:
        return "";
    }
}

static Const_t* load_constant_pool(FILE* cf, size_t nr)
{
    if (nr == 0)
        return NULL;
    Const_t* list = malloc(sizeof(list[0]) * nr);
    for (size_t i = 0; i < nr; ++i) {
        Const_t c;
        c.tag = read_big_endian_u1(cf);
        switch (c.tag) {
        case CONST_UTF8: {
            size_t l = read_big_endian_u2(cf);
            c.string = bytes(cf, l, 1);
            // printf("%lu \"%s\"\n", i, c.string);
        } break;
        case CONST_CLASS:
            c.name_index = read_big_endian_u2(cf);
            break;
        case CONST_STRING:
            c.string_index = read_big_endian_u2(cf);
            break;
        case CONST_FIELD:
        case CONST_METHOD:
            c.class_index = read_big_endian_u2(cf);
            c.name_and_type_index = read_big_endian_u2(cf);
            break;
        case CONST_NAME_AND_TYPE:
            c.name_index = read_big_endian_u2(cf);
            c.desc_index = read_big_endian_u2(cf);
            break;
        default:
            error("Unsupported constant pool tag: %d", c.tag);
        }
        list[i] = c;
    }
    return list;
}

static char const** load_interfaces(FILE* cf, size_t nr, Const_t* constant_pool_list)
{
    if (nr == 0)
        return NULL;
    char const** list = malloc(sizeof(list[0]) * nr);
    for (size_t i = 0; i < nr; ++i)
        list[i] = resolve_constant(constant_pool_list, read_big_endian_u2(cf));
    return list;
}

static int get_attr_type(char const* t)
{
    if (!strcmp(t, "Code"))
        return ATTR_CODE;
    if (!strcmp(t, "SourceFile"))
        return ATTR_SOURCE_FILE;
    error("Unknown attribute type: %s\n", t);
}
static void interpret_attr(Attr_t* attr, void* attr_buf, Const_t* constant_pool_list)
{
    switch (attr->type) {
    case ATTR_CODE: {
        struct ClassFileAttrCode_Piece_1 {
            uint16_t max_stack;
            uint16_t max_locals;
            uint32_t code_length;
            uint8_t code[];
        };
        struct ClassFileAttrCode_Piece_1* p1 = attr_buf;

        attr->attr_code.max_stack = u2_from_big_endian(p1->max_stack);

        attr->attr_code.max_locals = u2_from_big_endian(p1->max_locals);

        size_t code_length = u4_from_big_endian(p1->code_length);
        attr->attr_code.code_length = code_length;

        void* code = malloc(code_length);
        memmove(code, &p1->code, code_length);
        attr->attr_code.code = code;
    } break;
    case ATTR_SOURCE_FILE: {
        struct ClassFileAttrSourceFile {
            uint16_t index;
        };
        struct ClassFileAttrSourceFile* p = attr_buf;

        attr->attr_source_file.source_file = resolve_constant(constant_pool_list, u2_from_big_endian(p->index));
    } break;
    default:
        panic();
    }
}
static Attr_t* load_attrs(FILE* cf, size_t nr, Const_t* constant_pool_list)
{
    if (nr == 0)
        return NULL;
    Attr_t* attrs = malloc(sizeof(attrs[0]) * nr);
    for (size_t i = 0; i < nr; ++i) {
        char const* attr_name = resolve_constant(constant_pool_list, read_big_endian_u2(cf));
        attrs[i].type = get_attr_type(attr_name);

        size_t size = read_big_endian_u4(cf);
        void* attr_buf = bytes(cf, size, 0);
        interpret_attr(&attrs[i], attr_buf, constant_pool_list);
        free(attr_buf);
    }
    return attrs;
}

static Field_t* load_fields(FILE* cf, size_t nr, Const_t* constant_pool_list)
{
    if (nr == 0)
        return NULL;
    Field_t* fields = malloc(sizeof(fields[0]) * nr);
    for (size_t i = 0; i < nr; ++i) {
        Field_t f;
        f.flags = read_big_endian_u2(cf);
        f.name = resolve_constant(constant_pool_list, read_big_endian_u2(cf));
        f.desc = resolve_constant(constant_pool_list, read_big_endian_u2(cf));
        f.attrs.size = read_big_endian_u2(cf);
        f.attrs.list = load_attrs(cf, f.attrs.size, constant_pool_list);
        fields[i] = f;
    }
    return fields;
}

Class_t load_class(char const* classfile)
{
    FILE* cf = fopen(classfile, "r");
    if (cf == NULL) {
        error("Unable to open file %s for reading", classfile);
    }

    uint32_t magic = read_big_endian_u4(cf);
    printf("Magic: 0x%x\n", magic);

    uint16_t minor_version = read_big_endian_u2(cf), major_version = read_big_endian_u2(cf);
    printf("Version: %d.%d\n", major_version, minor_version);

    Class_t c;

    c.constant_pool.size = read_big_endian_u2(cf) - 1;
    c.constant_pool.list = load_constant_pool(cf, c.constant_pool.size);

    c.flags = read_big_endian_u2(cf);
    c.name = resolve_constant(c.constant_pool.list, read_big_endian_u2(cf));
    c.super = resolve_constant(c.constant_pool.list, read_big_endian_u2(cf));

    c.interfaces.size = read_big_endian_u2(cf);
    c.interfaces.list = load_interfaces(cf, c.interfaces.size, c.constant_pool.list);

    c.fields.size = read_big_endian_u2(cf);
    c.fields.list = load_fields(cf, c.fields.size, c.constant_pool.list);

    c.methods.size = read_big_endian_u2(cf);
    c.methods.list = (Method_t*)load_fields(cf, c.methods.size, c.constant_pool.list);

    c.attrs.size = read_big_endian_u2(cf);
    c.attrs.list = load_attrs(cf, c.attrs.size, c.constant_pool.list);

    return c;
}

static void print_attr(Attr_t const* a, size_t indent)
{
    for (size_t i = 0; i < indent; ++i)
        printf("    ");
    switch (a->type) {
    case ATTR_CODE:
        printf("Code: Max stack=%lu, Max locals=%lu, Bytecode = ", a->attr_code.max_stack, a->attr_code.max_locals);
        for (size_t i = 0; i < a->attr_code.code_length; i++)
            printf("0x%x ", a->attr_code.code[i]);
        break;
    case ATTR_SOURCE_FILE:
        printf("Source file: %s", a->attr_source_file.source_file);
        break;
    default:
        panic();
    }
    printf("\n");
}
static void print_field(Field_t* f, size_t indent)
{
    for (size_t i = 0; i < indent; ++i)
        printf("    ");
    printf("%s: %s\n", f->name, f->desc);
    for (size_t i = 0; i < f->attrs.size; ++i)
        print_attr(&f->attrs.list[i], indent + 1);
}
void print_class(Class_t const* c)
{
    printf("Class %s (Super %s)\n", c->name, c->super);

    if (c->interfaces.size > 0) {
        printf("    Implements interfaces:\n");
        for (size_t i = 0; i < c->interfaces.size; i++)
            printf("        %s\n", c->interfaces.list[i]);
    }

    printf("    Fields: %lu\n", c->fields.size);
    for (size_t i = 0; i < c->fields.size; i++)
        print_field(&c->fields.list[i], 2);

    printf("    Methods: %lu\n", c->methods.size);
    for (size_t i = 0; i < c->methods.size; i++)
        print_field(&c->methods.list[i], 2);

    printf("    Attributes:\n");
    for (size_t i = 0; i < c->attrs.size; ++i)
        print_attr(&c->attrs.list[i], 2);
}
