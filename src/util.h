#ifndef UTIL_H
#define UTIL_H

#include <stdarg.h>

#define BOLD "\x1b[1m"
#define RED "\x1b[31m"
#define GREEN "\x1b[32m"
#define YELLOW "\x1b[33m"
#define BLUE "\x1b[34m"
#define RESET "\x1b[0m"

struct cmd_args {
    char const* main_class;
};
struct cmd_args parse_cmd_args(int argc, char** argv);

void _Noreturn __attribute__((format(printf, 1, 2))) errorf(char const* fmt, ...);
void vdebugf(char const* fmt, va_list ap);
void vdebugfc(char const* c, char const* fmt, va_list ap);
void __attribute__((format(printf, 1, 2))) debugf(char const* fmt, ...);
void __attribute__((format(printf, 2, 3))) debugfc(char const* c, char const* fmt, ...);

#define panicf(fmt, ...) errorf("[INTERNAL %s:%d %s] " fmt, __FILE__, __LINE__, __func__, ##__VA_ARGS__)

#endif // UTIL_H
