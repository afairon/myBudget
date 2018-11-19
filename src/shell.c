#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "log.h"
#include "db.h"
#include "sqlite3.h"
#include "shell.h"

static char *sh_read_line(void) {
    int bufsize = SH_BUFFER_SIZE;
    int position = 0;
    char *buffer = malloc(sizeof(char) * bufsize);
    int c;
    
    if (!buffer) {
        log_fatal("Memory allocation error");
        exit(1);
    }
    
    while (1) {
        c = getchar();
        
        if (c == EOF || c == '\n') {
            buffer[position] = '\0';
            return buffer;
        } else {
            buffer[position] = c;
        }
        position++;
        
        if (position >= bufsize) {
            bufsize += SH_BUFFER_SIZE;
            buffer = realloc(buffer, bufsize);
            if (!buffer) {
                log_fatal("Memory allocation error");
                exit(1);
            }
        }
    }
}

static char **sh_read_args(char *line, int *argc) {
    char **args;
    char *token;
    int position = 0;

    args = (char **) malloc(sizeof(char) * SH_ARGV_SIZE);

    if (!args) {
        log_fatal("Memory allocation error");
        exit(1);
    }

    token = strtok(line, " \t");
    while (token != NULL) {

        *(args+position) = (char *) malloc(sizeof(char) * SH_BUFFER_SIZE);
        strcpy(*(args+position), token);

        token = strtok(NULL, " \t");
        position++;
    }

    *(args+position) = NULL;

    *argc = position;

    return args;
}

static int create_wallet(sqlite3 *db, int argc, char **args) {
    int i;
    int status;

    for (i = 0; i < argc; i++) {
        printf("Wallet Option: %s\n", *(args+i));
    }

    Wallet *myWallet = (Wallet *) malloc(sizeof(Wallet));
    strcpy(myWallet->name, args[2]);

    status = add_wallet(db, myWallet);

    free(myWallet);

    return status;
}

static int create_category(sqlite3 *db, int argc, char **args) {
    int i;
    int status;

    for (i = 0; i < argc; i++) {
        printf("Category Option: %s\n", *(args+i));
    }

    Category *myCategory = (Category *) malloc(sizeof(Category));
    strcpy(myCategory->name, args[2]);

    status = add_category(db, myCategory);

    free(myCategory);

    return status;
}

static int create_transaction(sqlite3 *db, int argc, char **args) {
    int i;
    int status;

    for (i = 0; i < argc; i++) {
        printf("Transaction Option: %s\n", *(args+i));
    }

    Transaction *myTransaction = (Transaction *) malloc(sizeof(myTransaction));
    strcpy(myTransaction->title, args[2]);
    myTransaction->amount = atof(args[3]);
    myTransaction->wallet_id = atoi(args[4]);
    myTransaction->category_id = 0;

    status = add_transaction(db, myTransaction);

    free(myTransaction);

    return status;
}

void sh_spawn(sqlite3 *db) {
    char *line;
    char **args;
    int argc;
    int quit = 0;

    printf("Welcome to myBudget!\n");

    do {
        printf("myBudget:~$ ");
        line = sh_read_line();
        printf("CMD: %s\n", line);
        args = sh_read_args(line, &argc);

        if (strcmp(args[0], "exit") == 0) {
            printf("You need to exit!\n");
            break;
        }

        if (strcmp(args[0], "wallet") == 0) {
            create_wallet(db, argc, args);
        } else if (strcmp(args[0], "category") == 0) {
            create_category(db, argc, args);
        } else if (strcmp(args[0], "transaction") == 0) {
            create_transaction(db, argc, args);
        }

        //free(line);
    } while (1);

    /*free(line);
    free(args);*/
}