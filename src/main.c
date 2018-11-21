#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <signal.h>

#include "db.h"
#include "shell.h"
#include "rxi/log.h"
#include "sqlite3/sqlite3.h"

DB_Handler *handler;
FILE *outFile = NULL;

void signal_handler(int signum) {
    sqlite3_close(handler->db);
    if (outFile != NULL) {
        fclose(outFile);
    }
    free(handler);
    exit(1);
}

int main(int argc, const char *argv[])
{
    int i;
    int LOG_F = 0;

    for (i = 0; i < argc; i++) {
        if (
            strcmp(argv[i], "-l") == 0 ||
            strcmp(argv[i], "--log") == 0
        ) {
            LOG_F = 1;
        }
        if (
            strcmp(argv[i], "-h") == 0 ||
            strcmp(argv[i], "--help") == 0
        ) {
            printf("Budget Manager\n");
            printf("-l, --log\tEnable logger\n");
            printf("-h, --help\tDisplay this message\n");
            exit(0);
        }
    }

    if (LOG_F) {
        outFile = fopen("budget.log", "a");
        if (outFile == NULL) {
            log_fatal("Couldn't initialize log file");
        } else {
            log_set_fp(outFile);
        }
    }

    handler = connect(NULL);
    if (handler->db == NULL) {
        log_fatal("Error sqlite: Couldn't open database \"%s\"", handler->db_name);
        free(handler);
        exit(1);
    }
    
    if (init_db(handler->db) != SQLITE_OK) {
        sqlite3_close(handler->db);
        free(handler);
        exit(1);
    }

    signal(SIGINT, signal_handler);

    sh_spawn(handler);

    sqlite3_close(handler->db);

    free(handler);

    if (outFile != NULL) {
        fclose(outFile);
    }

    return 0;
}
