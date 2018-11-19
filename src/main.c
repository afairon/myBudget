#include <stdio.h>
#include <stdlib.h>
#include "log.h"
#include "db.h"
#include "sqlite3.h"
#include "shell.h"

int main(int argc, const char *argv[])
{
    FILE *outFile;

    outFile = fopen("budget.log", "a");
    if (outFile == NULL) {
        log_fatal("Couldn't initialize log file");
    } else {
        log_set_quiet(1);
        log_set_fp(outFile);
    }

    DB_Handler *handler = connect(NULL);
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

    sh_spawn(handler->db);

    sqlite3_close(handler->db);

    free(handler);

    if (outFile != NULL) {
        fclose(outFile);
    }

    return 0;
}
