#ifndef UTIL_H
#define UTIL_H

#include <stdarg.h>

void _Noreturn __attribute__((format(printf, 1, 2))) errorf(char const* fmt, ...);
void vdebugf(char const* fmt, va_list ap);
void __attribute__((format(printf, 1, 2))) debugf(char const* fmt, ...);

#define panicf(fmt, ...) errorf("[INTERNAL %s:%d %s] " fmt, __FILE__, __LINE__, __func__, __VA_ARGS__)

#endif // UTIL_H
