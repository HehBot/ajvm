#include "util.h"

#include <argp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

static int debug = 0;

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

void vdebugfc(char const* c, char const* restrict fmt, va_list ap)
{
    if (debug) {
        printf("%s", c);
        vprintf(fmt, ap);
        printf(RESET);
    }
}
void vdebugf(char const* restrict fmt, va_list ap)
{
    if (debug) {
        vdebugfc(GREEN, fmt, ap);
    }
}
void debugfc(char const* c, char const* restrict fmt, ...)
{
    if (debug) {
        va_list ap;
        va_start(ap, fmt);
        vdebugfc(c, fmt, ap);
        va_end(ap);
    }
}
void debugf(char const* restrict fmt, ...)
{
    if (debug) {
        va_list ap;
        va_start(ap, fmt);
        vdebugf(fmt, ap);
        va_end(ap);
    }
}

static char args_doc[] = "MAIN_CLASS";
static char doc[] = "ajvm -- an implementation of a JVM";
static struct argp_option options[] = {
    { "debug", 'd', 0, 0, "Produce debugging output" },
    { 0 },
};
static error_t parse_opt(int key, char* arg, struct argp_state* state)
{
    struct cmd_args* cmd_args = state->input;

    switch (key) {
    case 'd':
        debug = 1;
        break;
    case ARGP_KEY_ARG:
        if (state->arg_num >= 2)
            argp_usage(state);
        cmd_args->main_class = arg;
        break;
    case ARGP_KEY_END:
        if (state->arg_num < 1)
            argp_usage(state);
        break;
    default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}
struct cmd_args parse_cmd_args(int argc, char** argv)
{
    struct cmd_args cmd_args = { NULL };
    static struct argp argp = { options, parse_opt, args_doc, doc };
    argp_parse(&argp, argc, argv, 0, 0, &cmd_args);
    return cmd_args;
}
