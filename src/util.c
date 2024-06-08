#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void errorf(char const* restrict fmt, ...)
{
    fprintf(stderr, "ERROR: ");
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    exit(EXIT_FAILURE);
}

void vdebugf(char const* restrict fmt, va_list ap)
{
#ifndef NDEBUG
    vprintf(fmt, ap);
#endif
}
void debugf(char const* restrict fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vdebugf(fmt, ap);
    va_end(ap);
}
