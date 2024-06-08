#ifndef CLASS_H
#define CLASS_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    enum {
        CONST_UTF8 = 0x01,
        CONST_CLASS = 0x07,
        CONST_STRING = 0x08,
        CONST_FIELD = 0x09,
        CONST_METHOD = 0x0a,
        CONST_NAME_AND_TYPE = 0x0c,
    } tag;
    char* string;
    uint16_t name_index;
    uint16_t class_index;
    uint16_t name_and_type_index;
    uint16_t string_index;
    uint16_t desc_index;
} Const_t;

enum AttrType {
    ATTR_CODE,
    ATTR_SOURCE_FILE,
};
typedef struct {
    size_t max_stack, max_locals;
    size_t code_length;
    uint8_t* code;
} AttrCode_t;
typedef struct {
    char const* source_file;
} AttrSourceFile_t;
typedef struct {
    enum AttrType type;
    union {
        AttrCode_t attr_code;
        AttrSourceFile_t attr_source_file;
    };
} Attr_t;

typedef struct _Class Class_t;

typedef struct {
    uint16_t flags;
    char const* name;
    char const* desc;
    struct {
        size_t size;
        Attr_t* list;
    } attrs;
} Field_t;

typedef Field_t Method_t;

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
        size_t* offset;
        size_t* cp_entry;
    } fields;
    struct {
        size_t size;
        Method_t* list;
        size_t* cp_entry;
    } methods;
    struct {
        size_t size;
        Attr_t* list;
    } attrs;
};

char const* resolve_constant(Const_t* constant_pool_list, size_t i);
typedef struct {
    Class_t* c;
    Method_t* m;
} methodref_t;
methodref_t resolve_methodref(Const_t* constant_pool_list, size_t i);

Method_t* get_method(Class_t* c, char const* methodname);
Attr_t get_attr(Method_t* m, enum AttrType t);

#endif // CLASS_H
