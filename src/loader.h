#ifndef LOADER_H
#define LOADER_H

#include "class.h"

static uint16_t u2_from_big_endian(uint16_t u)
{
    return ((u & 0xff) << 8 | (u & 0xff00) >> 8);
}

Class_t* load_class(char const* classfile);

#endif // LOADER_H
