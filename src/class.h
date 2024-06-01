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

typedef struct {
    size_t max_stack, max_locals;
    size_t code_length;
    uint8_t* code;
} AttrCode_t;
typedef struct {
    char const* source_file;
} AttrSourceFile_t;

typedef struct {
    enum {
        ATTR_CODE,
        ATTR_SOURCE_FILE,
    } type;
    union {
        AttrCode_t attr_code;
        AttrSourceFile_t attr_source_file;
    };
} Attr_t;

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

typedef struct {
    struct {
        size_t size;
        Const_t* list;
    } constant_pool;
    char const* name;
    char const* super;

    uint16_t flags;

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
    struct {
        size_t size;
        Attr_t* list;
    } attrs;
} Class_t;

#endif // CLASS_H
