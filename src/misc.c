#include <stdio.h>
#include <stdarg.h>

#ifdef PRETTY_PRINT
#if defined(_WIN32) || defined(_WIN64)
#include <Windows.h>
#endif
#endif

#include "misc.h"

// pretty_printf prints message with icon and color.
void pretty_printf(FILE *output, int type, const char *fmt, ...) {
    
    #ifdef PRETTY_PRINT
    #if defined(_WIN32) || defined(_WIN64)
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    WORD saved_attributes;
    
    GetConsoleScreenBufferInfo(hConsole, &consoleInfo);
    saved_attributes = consoleInfo.wAttributes;
    #endif
    #endif

    va_list args;

    va_start(args, fmt);

    #ifdef PRETTY_PRINT
    switch (type) {
        #ifdef __unix__
        case PRINT_INFO:
            fprintf(output, "\x1b[32m\u2139 ");
            break;
        case PRINT_SUCCESS:
            fprintf(output, "\x1b[32m\u2714 ");
            break;
        case PRINT_FAIL:
            fprintf(output, "\x1b[31m\u2717 ");
            break;
        case PRINT_WARNING:
            fprintf(output, "\x1b[31m\u26a0 ");
            break;
        default:
            break;
        #elif defined(_WIN32) || defined(_WIN64)
        case PRINT_INFO:
        case PRINT_SUCCESS:
            SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN);
            break;
        case PRINT_FAIL:
        case PRINT_WARNING:
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
            break;
        default:
            break;
        #else
        default:
            break;
        #endif
    }
    #endif

    vfprintf(output, fmt, args);

    #ifdef PRETTY_PRINT
    #ifdef __unix__
    fprintf(output, "\x1b[0m");
    #elif defined(_WIN32) || defined(_WIN64)
    SetConsoleTextAttribute(hConsole, saved_attributes);
    #endif
    #endif
    
    va_end(args);
    fprintf(output, "\n");
    fflush(output);
}