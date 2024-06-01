#ifndef UTIL_H
#define UTIL_H

#include <stdarg.h>

void _Noreturn __attribute__((format(printf, 1, 2))) errorf(char const* fmt, ...);
void vdebugf(char const* fmt, va_list ap);
void __attribute__((format(printf, 1, 2))) debugf(char const* fmt, ...);

#define panic() errorf("INTERNAL %s:%d %s\n", __FILE__, __LINE__, __func__)

#endif // UTIL_H
