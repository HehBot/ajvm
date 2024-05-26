#ifndef UTIL_H
#define UTIL_H

void _Noreturn error(char const* msg, ...) __attribute__((format(printf, 1, 2)));

#define panic() error("INTERNAL %s:%d %s\n", __FILE__, __LINE__, __func__)

#endif // UTIL_H
