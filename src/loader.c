#include "class.h"
#include "loader.h"
#include "util.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t u4_from_big_endian(uint32_t u)
{
    return ((u & 0xff) << 24 | (u & 0xff00) << 8 | (u & 0xff0000) >> 8 | (u & 0xff000000) >> 24);
}

static void* bytes(FILE* f, size_t sz, int is_str)
{
    void* buf = malloc(sz + (is_str ? 1 : 0));
    if (fread(buf, 1, sz, f) != sz)
        errorf("unexpected eof");
    if (is_str)
        ((uint8_t*)buf)[sz] = '\0';
    return buf;
}
static uint32_t read_big_endian_u4(FILE* f)
{
    uint32_t a;
    if (fread(&a, sizeof(a), 1, f) != 1)
        errorf("unexpected eof");
    return u4_from_big_endian(a);
}
static uint16_t read_big_endian_u2(FILE* f)
{
    uint16_t a;
    if (fread(&a, sizeof(a), 1, f) != 1)
        errorf("unexpected eof");
    return u2_from_big_endian(a);
}
static uint8_t read_big_endian_u1(FILE* f)
{
    uint8_t a;
    if (fread(&a, sizeof(a), 1, f) != 1)
        errorf("unexpected eof");
    return a;
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
            // indentdebugf("%lu \"%s\"\n", i, c.string);
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
            errorf("Unsupported constant pool tag: %d", c.tag);
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
    errorf("Unknown attribute type: %s\n", t);
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
        panicf("unknown attr type 0x%x", attr->type);
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

static Field_t* load_fields(FILE* cf, size_t nr, Const_t* constant_pool_list, size_t* cp_entry)
{
    if (nr == 0)
        return NULL;
    Field_t* fields = malloc(sizeof(fields[0]) * nr);
    for (size_t i = 0; i < nr; ++i) {
        Field_t f;
        f.flags = read_big_endian_u2(cf);
        cp_entry[i] = read_big_endian_u2(cf);
        f.name = resolve_constant(constant_pool_list, cp_entry[i]);
        f.desc = resolve_constant(constant_pool_list, read_big_endian_u2(cf));
        f.attrs.size = read_big_endian_u2(cf);
        f.attrs.list = load_attrs(cf, f.attrs.size, constant_pool_list);
        fields[i] = f;
    }
    return fields;
}

static void __attribute__((format(printf, 2, 3))) indentdebugf(int indent, char const* restrict fmt, ...)
{
    for (int i = 0; i < indent; ++i)
        debugf("    ");
    va_list ap;
    va_start(ap, fmt);
    vdebugf(fmt, ap);
    va_end(ap);
}
static void print_attr(Attr_t const* a, size_t indent)
{
    switch (a->type) {
    case ATTR_CODE:
        indentdebugf(indent, "Code: Max stack=%lu, Max locals=%lu,\n", a->attr_code.max_stack, a->attr_code.max_locals);
        indentdebugf(indent, "      Bytecode=\n");
        {
#define CODEWIDTH 10
            size_t i;
            for (i = 0; i < CODEWIDTH * (a->attr_code.code_length / CODEWIDTH); i += CODEWIDTH) {
                indentdebugf(indent + 2, "  ");
                for (size_t j = 0; j < CODEWIDTH; ++j)
                    debugf("0x%02x ", a->attr_code.code[i + j]);
                debugf("\n");
            }
            indentdebugf(indent + 2, "  ");
            for (size_t k = i; k < a->attr_code.code_length; ++k)
                debugf("0x%02x ", a->attr_code.code[k]);
        }
        break;
    case ATTR_SOURCE_FILE:
        indentdebugf(indent, "Source file: %s", a->attr_source_file.source_file);
        break;
    default:
        panicf("unknown attr type 0x%x", a->type);
    }
    debugf("\n");
}
static void print_field(Field_t* f, size_t indent)
{
    indentdebugf(indent, "%s:%s %s\n", f->name, (f->flags & 0x0008 ? " [static]" : ""), f->desc);
    for (size_t i = 0; i < f->attrs.size; ++i)
        print_attr(&f->attrs.list[i], indent + 1);
}
static void print_class(Class_t const* c, size_t indent)
{
    indentdebugf(indent, "Super: %s\n", c->super->name);

    if (c->interfaces.size > 0) {
        indentdebugf(indent, "Implements interfaces:\n");
        for (size_t i = 0; i < c->interfaces.size; i++)
            indentdebugf(indent + 1, "%s\n", c->interfaces.list[i]);
    }

    indentdebugf(indent, "Fields: %lu\n", c->fields.size);
    for (size_t i = 0; i < c->fields.size; i++)
        print_field(&c->fields.list[i], indent + 1);

    indentdebugf(indent, "Methods: %lu\n", c->methods.size);
    for (size_t i = 0; i < c->methods.size; i++)
        print_field(&c->methods.list[i], indent + 1);

    indentdebugf(indent, "Attributes:\n");
    for (size_t i = 0; i < c->attrs.size; ++i)
        print_attr(&c->attrs.list[i], indent + 1);
}

static void init_java_lang_Object(Class_t* c)
{
    static Method_t java_lang_Object_init = {
        .flags = 0,
        .name = "<init>",
        .desc = "()V",
        .attrs = { 0, NULL },
    };
    Class_t java_lang_Object = {
        .constant_pool = { 0, NULL },
        .name = "java/lang/Object",
        .super = NULL,
        .flags = 0,
        .interfaces = { 0, NULL },
        .fields = { 0, NULL, NULL, NULL },
        .methods = { 1, &java_lang_Object_init },
        .attrs = { 0, NULL },
    };
    *c = java_lang_Object;
}

static size_t get_size_from_desc(char desc)
{
    switch (desc) {
    case 'I':
    case 'F':
        return 4;
    case 'J':
    case 'D':
    case 'L':
        return 8;
    case '[':
        return 16;
    default:
        panicf("unknown type desc %c", desc);
    }
}

static struct {
    size_t nr, cap;
    Class_t* list;
} loaded_classes = { 0, 0, NULL };

Class_t* load_class(char const* classname)
{
    if (loaded_classes.cap == 0) {
        loaded_classes.cap = 2;
        loaded_classes.list = malloc(sizeof(loaded_classes.list[0]) * loaded_classes.cap);
        init_java_lang_Object(&loaded_classes.list[0]);
        loaded_classes.nr = 1;
    }

    for (size_t i = 0; i < loaded_classes.nr; ++i)
        if (strcmp(loaded_classes.list[i].name, classname) == 0)
            return &loaded_classes.list[i];

    char* filename = malloc(strlen(classname) + strlen(".class") + 1);
    sprintf(filename, "%s.class", classname);
    FILE* cf = fopen(filename, "r");
    if (cf == NULL)
        errorf("Unable to open file %s for reading", filename);
    uint32_t magic = read_big_endian_u4(cf);
    if (magic != 0xcafebabe)
        errorf("bad magic number 0x%x for class file %s\n", magic, filename);
    free(filename);

    uint16_t minor_version = read_big_endian_u2(cf), major_version = read_big_endian_u2(cf);

    if (loaded_classes.nr == loaded_classes.cap) {
        loaded_classes.cap = 2 * loaded_classes.cap;
        loaded_classes.list = realloc(loaded_classes.list, sizeof(loaded_classes.list[0]) * loaded_classes.cap);
    }
    Class_t* c = &loaded_classes.list[loaded_classes.nr++];

    c->constant_pool.size = read_big_endian_u2(cf) - 1;
    c->constant_pool.list = load_constant_pool(cf, c->constant_pool.size);

    c->flags = read_big_endian_u2(cf);
    c->name = resolve_constant(c->constant_pool.list, read_big_endian_u2(cf));
    c->super = load_class(resolve_constant(c->constant_pool.list, read_big_endian_u2(cf)));

    c->interfaces.size = read_big_endian_u2(cf);
    c->interfaces.list = load_interfaces(cf, c->interfaces.size, c->constant_pool.list);

    c->fields.size = read_big_endian_u2(cf);
    c->fields.cp_entry = malloc(sizeof(c->fields.cp_entry[0]) * c->fields.size);
    c->fields.list = load_fields(cf, c->fields.size, c->constant_pool.list, c->fields.cp_entry);
    c->fields.offset = malloc(sizeof(c->fields.offset[0]) * c->fields.size);
    size_t off = 0;
    for (size_t i = 0; i < c->fields.size; ++i) {
        c->fields.offset[i] = off;
        c->fields.cp_entry[i] = 0;
        off += get_size_from_desc(c->fields.list[i].desc[0]);
    }
    c->size = off;

    c->methods.size = read_big_endian_u2(cf);
    c->methods.cp_entry = malloc(sizeof(c->methods.cp_entry[0]) * c->methods.size);
    c->methods.list = (Method_t*)load_fields(cf, c->methods.size, c->constant_pool.list, c->methods.cp_entry);

    c->attrs.size = read_big_endian_u2(cf);
    c->attrs.list = load_attrs(cf, c->attrs.size, c->constant_pool.list);

    indentdebugf(1, "====== Loaded class '%s' (classfile ver. %d.%d) =====\n", c->name, major_version, minor_version);
    print_class(c, 2);
    indentdebugf(1, "=====================================================\n");
    fclose(cf);
    return c;
}

static void free_attr(Attr_t const* a)
{
    if (a->type == ATTR_CODE)
        free(a->attr_code.code);
}

static void free_field(Field_t const* f)
{
    for (size_t i = 0; i < f->attrs.size; ++i)
        free_attr(&f->attrs.list[i]);
    free(f->attrs.list);
}

static void free_class(Class_t const* c)
{
    free(c->interfaces.list);

    for (size_t i = 0; i < c->fields.size; ++i)
        free_field(&c->fields.list[i]);
    free(c->fields.list);
    free(c->fields.offset);
    free(c->fields.cp_entry);

    for (size_t i = 0; i < c->methods.size; ++i)
        free_field((Method_t*)&c->methods.list[i]);
    free(c->methods.list);
    free(c->methods.cp_entry);

    for (size_t i = 0; i < c->constant_pool.size; ++i)
        if (c->constant_pool.list[i].tag == CONST_UTF8)
            free(c->constant_pool.list[i].string);
    free(c->constant_pool.list);

    for (size_t i = 0; i < c->attrs.size; ++i)
        free_attr(&c->attrs.list[i]);
    free(c->attrs.list);
}

void load_end()
{
    for (size_t i = 1; i < loaded_classes.nr; ++i)
        free_class(&loaded_classes.list[i]);
    free(loaded_classes.list);
}
