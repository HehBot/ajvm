#ifndef LOADER_H
#define LOADER_H

#include "class.h"

Class_t load_class(char const* classfile);
void print_class(Class_t const* c);

#endif // LOADER_H
