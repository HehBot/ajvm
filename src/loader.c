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
            errorf("unsupported constant pool tag: %d", c.tag);
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

enum AttrType {
    ATTR_CODE,
    ATTR_SOURCE_FILE,
};
static enum AttrType get_attr_type(char const* t)
{
    if (!strcmp(t, "Code"))
        return ATTR_CODE;
    if (!strcmp(t, "SourceFile"))
        return ATTR_SOURCE_FILE;
    errorf("unknown attribute type: %s\n", t);
}

static void load_field_attrs(FILE* cf, Field_t* f, Const_t* constant_pool_list)
{
    size_t nr = read_big_endian_u2(cf);
    for (size_t i = 0; i < nr; ++i) {
        enum AttrType attr_type = get_attr_type(resolve_constant(constant_pool_list, read_big_endian_u2(cf)));

        size_t size = read_big_endian_u4(cf);
        void* attr_buf = bytes(cf, size, 0);

        switch (attr_type) {
        case ATTR_SOURCE_FILE: {
            struct ClassFileAttrSourceFile {
                uint16_t index;
            };
            struct ClassFileAttrSourceFile* p = attr_buf;

            f->source_file = resolve_constant(constant_pool_list, u2_from_big_endian(p->index));
        } break;
        default:
            panicf("unknown attr for field 0x%x", attr_type);
        }

        free(attr_buf);
    }
}
static void load_fields(FILE* cf, Class_t* c)
{
    size_t nr = read_big_endian_u2(cf);
    size_t off = c->super->size;
    Field_t* fields = malloc(sizeof(fields[0]) * nr);
    for (size_t i = 0; i < nr; ++i) {
        Field_t f;
        f.flags = read_big_endian_u2(cf);
        size_t cp_entry = read_big_endian_u2(cf);
        f.name = resolve_constant(c->constant_pool.list, cp_entry);
        f.desc = resolve_constant(c->constant_pool.list, read_big_endian_u2(cf));
        load_field_attrs(cf, &f, c->constant_pool.list);

        f.offset = off;
        off += get_size_from_desc(f.desc[0]);

        fields[i] = f;
    }
    c->fields.list = fields;
    c->fields.size = nr;
    c->size = off + c->super->size;
}

static void load_method_attrs(FILE* cf, Method_t* m, Const_t* constant_pool_list)
{
    size_t nr = read_big_endian_u2(cf);
    for (size_t i = 0; i < nr; ++i) {
        enum AttrType attr_type = get_attr_type(resolve_constant(constant_pool_list, read_big_endian_u2(cf)));

        size_t size = read_big_endian_u4(cf);
        void* attr_buf = bytes(cf, size, 0);

        switch (attr_type) {
        case ATTR_CODE: {
            struct ClassFileAttrCode_Piece_1 {
                uint16_t max_stack;
                uint16_t max_locals;
                uint32_t code_length;
                uint8_t code[];
            };
            struct ClassFileAttrCode_Piece_1* p1 = attr_buf;

            m->max_stack = u2_from_big_endian(p1->max_stack);

            m->max_locals = u2_from_big_endian(p1->max_locals);

            size_t code_length = u4_from_big_endian(p1->code_length);
            m->code_length = code_length;

            void* code = malloc(code_length);
            memmove(code, &p1->code, code_length);
            m->code = code;
        } break;
        case ATTR_SOURCE_FILE: {
            struct ClassFileAttrSourceFile {
                uint16_t index;
            };
            struct ClassFileAttrSourceFile* p = attr_buf;

            m->source_file = resolve_constant(constant_pool_list, u2_from_big_endian(p->index));
        } break;
        default:
            panicf("unknown attr for method 0x%x", attr_type);
        }

        free(attr_buf);
    }
}
static void load_methods(FILE* cf, Class_t* c)
{
    size_t nr = read_big_endian_u2(cf);
    Method_t* methods = malloc(sizeof(methods[0]) * nr);
    for (size_t i = 0; i < nr; ++i) {
        Method_t m;
        m.flags = read_big_endian_u2(cf);
        size_t cp_entry = read_big_endian_u2(cf);
        m.name = resolve_constant(c->constant_pool.list, cp_entry);
        m.desc = resolve_constant(c->constant_pool.list, read_big_endian_u2(cf));
        load_method_attrs(cf, &m, c->constant_pool.list);
        methods[i] = m;
    }
    c->methods.size = nr;
    c->methods.list = methods;
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
static void print_field(Field_t const* f, size_t indent)
{
    char b[40];
    sprintf(b, " <+%lu>", f->offset);
    indentdebugf(indent, "%s:%s %s\n", f->name, (f->flags & ACC_STATIC ? " [static]" : b), f->desc);
}
static void print_method(Method_t const* m, size_t indent)
{
    indentdebugf(indent, "%s:%s\n", m->name, m->desc);
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
        print_method(&c->methods.list[i], indent + 1);
}

void load_class_attrs(FILE* cf, Class_t* c)
{
    size_t nr = read_big_endian_u2(cf);
    for (size_t i = 0; i < nr; ++i) {
        enum AttrType attr_type = get_attr_type(resolve_constant(c->constant_pool.list, read_big_endian_u2(cf)));

        size_t size = read_big_endian_u4(cf);
        void* attr_buf = bytes(cf, size, 0);

        switch (attr_type) {
        case ATTR_SOURCE_FILE: {
            struct ClassFileAttrSourceFile {
                uint16_t index;
            };
            struct ClassFileAttrSourceFile* p = attr_buf;

            c->source_file = resolve_constant(c->constant_pool.list, u2_from_big_endian(p->index));
        } break;
        default:
            panicf("unknown attr for class 0x%x", attr_type);
        }

        free(attr_buf);
    }
}

static struct {
    size_t nr, cap;
    Class_t* list;
} loaded_classes = { 0, 0, NULL };

Class_t* load_class(char const* classname)
{
    for (size_t i = 0; i < loaded_classes.nr; ++i)
        if (strcmp(loaded_classes.list[i].name, classname) == 0)
            return &loaded_classes.list[i];

    char* filename = malloc(strlen(classname) + strlen(".class") + 1);
    sprintf(filename, "%s.class", classname);
    FILE* cf = fopen(filename, "r");
    if (cf == NULL)
        errorf("unable to open file %s for reading", filename);
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

    load_fields(cf, c);
    load_methods(cf, c);
    load_class_attrs(cf, c);

    indentdebugf(1, "======= Loaded %s (classfile '%s' ver. %d.%d) =======\n", c->name, c->source_file, major_version, minor_version);
    print_class(c, 2);
    indentdebugf(1, "=====================================================\n");

    fclose(cf);
    return c;
}

static void free_field(Field_t const* f)
{
}
static void free_method(Method_t const* m)
{
    free(m->code);
}
static void free_class(Class_t const* c)
{
    free(c->interfaces.list);

    for (size_t i = 0; i < c->fields.size; ++i)
        free_field(&c->fields.list[i]);
    free(c->fields.list);

    for (size_t i = 0; i < c->methods.size; ++i)
        free_method(&c->methods.list[i]);
    free(c->methods.list);

    for (size_t i = 0; i < c->constant_pool.size; ++i)
        if (c->constant_pool.list[i].tag == CONST_UTF8)
            free(c->constant_pool.list[i].string);
    free(c->constant_pool.list);
}

static void init_java_lang_Object(Class_t* c)
{
    static Method_t init = {
        .flags = ACC_NATIVE,
        .name = "<init>",
        .desc = "()V",
    };
    Class_t java_lang_Object = {
        .constant_pool = { 0, NULL },
        .name = "java/lang/Object",
        .super = NULL,
        .flags = 0,
        .size = 0,
        .interfaces = { 0, NULL },
        .fields = {
            0,
            NULL,
        },
        .methods = { 1, &init },
        .source_file = NULL,
    };
    *c = java_lang_Object;
}
static void init_java_lang_System(Class_t* c)
{
    static Field_t streams[] = {
        {
            .flags = ACC_STATIC,
            .name = "out",
            .desc = "Ljava/io/PrintStream;",
        },
        {
            .flags = ACC_STATIC,
            .name = "err",
            .desc = "Ljava/io/PrintStream;",
        }
    };
    streams[0].static_val = (Value_t) { .type = A, .a = stdout };
    streams[1].static_val = (Value_t) { .type = A, .a = stderr };
    Class_t java_lang_System = {
        .constant_pool = { 0, NULL },
        .name = "java/lang/System",
        .super = NULL,
        .flags = 0,
        .size = 0,
        .interfaces = { 0, NULL },
        .fields = {
            sizeof(streams) / sizeof(streams[0]),
            streams,
        },
        .methods = { 0, NULL },
        .source_file = NULL,
    };
    *c = java_lang_System;
}
static void init_java_io_PrintStream(Class_t* c)
{
    static Method_t methods[] = {
        {
            .flags = 0,
            .name = "<init>",
            .desc = "()V",
        },
        {
            .flags = ACC_NATIVE,
            .name = "println",
            .desc = "(I)V",
        }
    };
    Class_t java_io_PrintStream = {
        .constant_pool = { 0, NULL },
        .name = "java/io/PrintStream",
        .super = NULL,
        .flags = 0,
        .size = 0,
        .interfaces = { 0, NULL },
        .fields = { 0, NULL },
        .methods = { sizeof(methods) / sizeof(methods[0]), methods },
        .source_file = NULL,
    };
    *c = java_io_PrintStream;
}
void load_init()
{
    loaded_classes.cap = 3;
    loaded_classes.list = malloc(sizeof(loaded_classes.list[0]) * loaded_classes.cap);
    loaded_classes.nr = 3;
    init_java_lang_Object(&loaded_classes.list[0]);
    init_java_lang_System(&loaded_classes.list[1]);
    init_java_io_PrintStream(&loaded_classes.list[2]);
}
void load_end()
{
    for (size_t i = 3; i < loaded_classes.nr; ++i)
        free_class(&loaded_classes.list[i]);
    free(loaded_classes.list);
}
