#ifndef CLASS_H
#define CLASS_H

#include <stddef.h>
#include <stdint.h>

typedef struct Value {
    enum ValueType {
        I,
        F,
        A,
        L,
        D,
        ARR,
    } type;
    union {
        int32_t i;
        float f;
        void* a;

        int64_t l;
        double d;
        struct {
            uint32_t len, cap;
            struct Value* buf;
        } arr;
    };
} Value_t;

typedef struct {
    enum ConstType {
        CONST_UTF8 = 0x01,
        CONST_INT = 0x03,
        CONST_FLOAT = 0x04,
        CONST_LONG = 0x05,
        CONST_DOUBLE = 0x06,
        CONST_CLASS = 0x07,
        CONST_STRING = 0x08,
        CONST_FIELD = 0x09,
        CONST_METHOD = 0x0a,
        CONST_NAME_AND_TYPE = 0x0c,
    } tag;
    union {
        char* utf8;
        int32_t i;
        int64_t l;
        float f;
        double d;
        struct {
            uint16_t name_index;
            uint16_t desc_index;
        };
        struct {
            uint16_t class_index;
            uint16_t name_and_type_index;
        };
        uint16_t string_index;
    };
} Const_t;

typedef struct _Class Class_t;

enum Flags {
    ACC_STATIC = 0x0008,
    ACC_NATIVE = 0x0100,
};
typedef struct {
    uint16_t flags;
    char const* name;
    char const* desc;

    union {
        size_t offset;
        Value_t static_val;
    };

    char const* source_file;
} Field_t;

typedef struct {
    uint16_t flags;
    char const* name;
    char const* desc;

    struct {
        size_t max_stack, max_locals;
        size_t code_length;
        uint8_t* code;
    };

    Class_t* c;
    size_t vtable_offset;

    char const* source_file;
} Method_t;

struct _Class {
    struct {
        size_t size;
        Const_t* list;
    } constant_pool;
    char const* name;
    Class_t* super;

    uint16_t flags;
    size_t size;

    struct {
        size_t size;
        char const** list;
    } interfaces;
    struct {
        size_t size;
        Field_t* list;
    } fields;
    struct {
        size_t size;
        Method_t* list;
    } methods;

    Method_t** vtable; // NULL-terminated

    char const* source_file;
};

char const* resolve_utf8(Const_t* constant_pool_list, size_t i);
char const* resolve_class(Const_t* constant_pool_list, size_t i);
Field_t* resolve_fieldref(Const_t* constant_pool_list, size_t i);
Method_t* resolve_methodref(Const_t* constant_pool_list, size_t i);

Method_t* get_method(Class_t* c, char const* methodname, char const* desc);

#endif // CLASS_H
