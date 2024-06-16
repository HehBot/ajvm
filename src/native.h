#ifndef NATIVE_H
#define NATIVE_H

#include "class.h"

#include <stdio.h>

void init_java_lang_Object(Class_t* c);
void init_java_io_PrintStream(Class_t* c);
// load java/lang/System AFTER java/io/PrintStream as former depends on latter
void init_java_lang_System(Class_t* c);

void native_println_I(void* a, int32_t i);
void native_println_D(void* a, double d);

#endif // NATIVE_H
