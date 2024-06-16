#include "class.h"
#include "loader.h"
#include "util.h"

#include <string.h>

char const* resolve_constant(Const_t* constant_pool_list, size_t i)
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
        errorf("unable to resolve constant %lu", i);
    }
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
    char const* classname = resolve_constant(constant_pool_list, con->class_index);
    Class_t* c = load_class(classname);
    char const* fieldname = resolve_constant(constant_pool_list, con->name_and_type_index);
    return get_field(c, fieldname);
}

Method_t* get_method(Class_t* c, char const* methodname)
{
    for (size_t i = 0; i < c->methods.size; ++i)
        if (strcmp(c->methods.list[i].name, methodname) == 0)
            return &c->methods.list[i];
    errorf("unable to find method %s in class %s", methodname, c->name);
}
methodref_t resolve_methodref(Const_t* constant_pool_list, size_t i)
{
    Const_t* con = &constant_pool_list[i - 1];
    if (con->tag != CONST_METHOD)
        errorf("unable to resolve methodref %lu", i);
    char const* classname = resolve_constant(constant_pool_list, con->class_index);
    Class_t* c = load_class(classname);
    char const* methodname = resolve_constant(constant_pool_list, con->name_and_type_index);
    Method_t* m = get_method(c, methodname);
    methodref_t mf = { c, m };
    return mf;
}
