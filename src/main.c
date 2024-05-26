#include "class.h"
#include "loader.h"
#include "util.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void _Noreturn error(char const* restrict msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    exit(1);
}

int main(int argc, char** argv)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <Java classfile>\n", argv[0]);
        return 1;
    }

    Class_t c = load_class(argv[1]);
    print_class(&c);

    return 0;
}
