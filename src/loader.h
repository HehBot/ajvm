#ifndef LOADER_H
#define LOADER_H

#include "class.h"
#include "util.h"

static inline uint16_t u2_from_big_endian(uint16_t u)
{
    return ((u & 0xff) << 8 | (u & 0xff00) >> 8);
}

Class_t* load_class(char const* classfile);
void load_init(void);
void load_end(void);

static inline size_t get_size_from_desc(char desc)
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

#endif // LOADER_H
