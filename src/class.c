#include "class.h"
#include "loader.h"
#include "util.h"

#include <string.h>

typedef struct {
    char const* name;
    char const* type;
} resolved_t;

char const* resolve_utf8(Const_t* constant_pool_list, size_t i)
{
    Const_t* c = &constant_pool_list[i - 1];
    if (c->tag != CONST_UTF8)
        errorf("unable to resolve utf8 constant %lu (expected tag 0x%x, got 0x%x)\n", i, CONST_UTF8, c->tag);
    return c->utf8;
}

static resolved_t resolve_constant(Const_t* constant_pool_list, size_t i, enum ConstType expected_tag)
{
    Const_t* c = &constant_pool_list[i - 1];
    if (c->tag != expected_tag)
        errorf("unable to resolve constant %lu as tag", i);
    resolved_t ret = { NULL, NULL };
    switch (c->tag) {
    case CONST_UTF8:
        ret.name = c->utf8;
        break;
    case CONST_STRING:
        ret.name = resolve_utf8(constant_pool_list, c->string_index);
        break;
    case CONST_CLASS:
        ret.name = resolve_utf8(constant_pool_list, c->name_index);
        break;
    case CONST_NAME_AND_TYPE:
        ret.name = resolve_utf8(constant_pool_list, c->name_index);
        ret.type = resolve_utf8(constant_pool_list, c->desc_index);
        break;
    default:
        errorf("unable to resolve constant %lu, tag 0x%x", i, c->tag);
    }
    return ret;
}

static Field_t* get_field(Class_t* c, char const* fieldname)
{
    for (size_t i = 0; i < c->fields.size; ++i)
        if (strcmp(c->fields.list[i].name, fieldname) == 0)
            return &c->fields.list[i];
    errorf("unable to find field %s in class %s", fieldname, c->name);
}
Field_t* resolve_fieldref(Const_t* constant_pool_list, size_t i)
{
    Const_t* con = &constant_pool_list[i - 1];
    if (con->tag != CONST_FIELD)
        errorf("unable to resolve fieldref %lu", i);
    char const* classname = resolve_constant(constant_pool_list, con->class_index, CONST_CLASS).name;
    Class_t* c = load_class(classname);
    char const* fieldname = resolve_constant(constant_pool_list, con->name_and_type_index, CONST_NAME_AND_TYPE).name;
    return get_field(c, fieldname);
}

Method_t* get_method(Class_t* c, char const* methodname, char const* desc)
{
    for (size_t i = 0; i < c->methods.size; ++i)
        if (strcmp(c->methods.list[i].name, methodname) == 0
            && strcmp(c->methods.list[i].desc, desc) == 0)
            return &c->methods.list[i];
    errorf("unable to find method %s in class %s", methodname, c->name);
}
Method_t* resolve_methodref(Const_t* constant_pool_list, size_t i)
{
    Const_t* con = &constant_pool_list[i - 1];
    if (con->tag != CONST_METHOD)
        errorf("unable to resolve methodref %lu", i);
    char const* classname = resolve_constant(constant_pool_list, con->class_index, CONST_CLASS).name;
    Class_t* c = load_class(classname);
    resolved_t name_and_type = resolve_constant(constant_pool_list, con->name_and_type_index, CONST_NAME_AND_TYPE);
    return get_method(c, name_and_type.name, name_and_type.type);
}

char const* resolve_class(Const_t* constant_pool_list, size_t i)
{
    return resolve_constant(constant_pool_list, i, CONST_CLASS).name;
}
