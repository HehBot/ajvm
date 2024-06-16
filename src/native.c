#include "loader.h"
#include "native.h"

#include <stdint.h>
#include <stdio.h>

struct java_io_PrintStream_object {
    Method_t** vtable;
    FILE* f;
};
void native_println_I(void* a, int32_t i)
{
    FILE* f = ((struct java_io_PrintStream_object*)a)->f;
    fprintf(f, "%d\n", i);
}
void native_println_D(void* a, double d)
{
    FILE* f = ((struct java_io_PrintStream_object*)a)->f;
    fprintf(f, "%lf\n", d);
}

void init_java_lang_Object(Class_t* c)
{
    static Method_t init = {
        .flags = ACC_NATIVE,
        .name = "<init>",
        .desc = "()V",
    };
    init.c = c;

    static Method_t* vtable[] = { NULL };

    Class_t java_lang_Object = {
        .constant_pool = { 0, NULL },
        .name = "java/lang/Object",
        .super = NULL,
        .flags = 0,
        .size = sizeof(&vtable[0]),
        .interfaces = { 0, NULL },
        .fields = {
            0,
            NULL,
        },
        .methods = { 1, &init },

        .vtable = &vtable[0],

        .source_file = NULL,
    };
    *c = java_lang_Object;
}
void init_java_io_PrintStream(Class_t* c)
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
            .vtable_offset = 0,
        },
        {
            .flags = ACC_NATIVE,
            .name = "println",
            .desc = "(D)V",
            .vtable_offset = 1,
        }
    };
    methods[0].c = c;
    methods[1].c = c;
    methods[2].c = c;

    static Method_t* vtable[] = { &methods[1], &methods[2], NULL };

    Class_t java_io_PrintStream = {
        .constant_pool = { 0, NULL },
        .name = "java/io/PrintStream",
        .super = NULL,
        .flags = 0,
        .size = sizeof(&vtable[0]),
        .interfaces = { 0, NULL },
        .fields = { 0, NULL },
        .methods = { sizeof(methods) / sizeof(methods[0]), methods },

        .vtable = &vtable[0],

        .source_file = NULL,
    };
    *c = java_io_PrintStream;
}
void init_java_lang_System(Class_t* c)
{
    Class_t* java_io_PrintStream = load_class("java/io/PrintStream");
    static struct java_io_PrintStream_object out = { 0 }, err = { 0 };
    out.vtable = java_io_PrintStream->vtable;
    out.f = stdout;
    err.vtable = java_io_PrintStream->vtable;
    err.f = stderr;

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
    streams[0].static_val = (Value_t) { .type = A, .a = &out };
    streams[1].static_val = (Value_t) { .type = A, .a = &err };

    Method_t* vtable[] = { NULL };

    Class_t java_lang_System = {
        .constant_pool = { 0, NULL },
        .name = "java/lang/System",
        .super = NULL,
        .flags = 0,
        .size = sizeof(&vtable[0]),
        .interfaces = { 0, NULL },
        .fields = {
            sizeof(streams) / sizeof(streams[0]),
            streams,
        },
        .methods = { 0, NULL },

        .vtable = &vtable[0],

        .source_file = NULL,
    };
    *c = java_lang_System;
}
