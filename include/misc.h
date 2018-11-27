#ifndef MISC_H
#define MISC_H

#include <stdio.h>
#include <stdarg.h>

enum {
    PRINT_INFO,
    PRINT_SUCCESS,
    PRINT_FAIL,
    PRINT_WARNING
};

#define pretty_info(...) pretty_printf(stdout, PRINT_INFO, __VA_ARGS__)
#define pretty_success(...) pretty_printf(stdout, PRINT_SUCCESS, __VA_ARGS__)
#define pretty_fail(...) pretty_printf(stderr, PRINT_FAIL, __VA_ARGS__)
#define pretty_warning(...) pretty_printf(stderr, PRINT_WARNING, __VA_ARGS__)

void pretty_printf(FILE *, int, const char *, ...);

#endif