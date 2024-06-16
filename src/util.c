#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#define BOLD "\x1b[1m"
#define RED "\x1b[31m"
#define GREEN "\x1b[32m"
#define RESET "\x1b[0m"

int debug;

void errorf(char const* restrict fmt, ...)
{
    fprintf(stderr, BOLD RED "ERROR: ");
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
    printf(GREEN);
    vprintf(fmt, ap);
    printf(RESET);
#endif
}
void debugf(char const* restrict fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vdebugf(fmt, ap);
    va_end(ap);
}
