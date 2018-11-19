#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "log.h"
#include "db.h"
#include "sqlite3.h"
#include "shell.h"

static char *lst_cmd[] = {
    "wallet",
    "dwallet",
    "category",
    "dcategory",
    "transaction",
    "dtransaction",
    "exit"
};

static int (*cmd_func[]) (sqlite3 *, int, char **) = {
    &create_wallet,
    &delete_wallet,
    &create_category,
    &delete_category,
    &create_transaction,
    &delete_transaction,
    &sh_exit,
};

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

    args = (char **) malloc(sizeof(char *) * SH_ARGV_SIZE);

    if (!args) {
        log_fatal("Memory allocation error");
        exit(1);
    }

    token = strtok(line, " \t");
    while (token != NULL) {
        args[position] = (char *) malloc(sizeof(char) * SH_BUFFER_SIZE);
        strcpy(args[position], token);

        token = strtok(NULL, " \t");
        position++;
    }

    *argc = position;

    return args;
}

static int clear_args(int argc, char **args) {
    int i;

    for (i = 0; i < argc; i++) {
        free(args[i]);
    }
    return 0;
}

static int create_wallet(sqlite3 *db, int argc, char **args) {
    int i;
    int status;

    if (argc < 3) {
        printf("\x1b[31mInvalid statement: ");
        for (i = 0; i < argc; i++) {
            printf("%s ", *(args+i));
        }
        printf("\n\x1b[0m");
        return 1;
    }

    Wallet *myWallet = (Wallet *) calloc(1, sizeof(Wallet));
    strcpy(myWallet->name, args[2]);

    status = add_wallet(db, myWallet);

    free(myWallet);

    if (status == SQLITE_OK) {
        printf("\x1b[32m\u2714 Create wallet \"%s\" successfully.\n\x1b[0m", args[2]);
    } else {
        printf("\x1b[31m\u2717 Failed to create a wallet.\n\x1b[0m");
    }

    return 1;
}

static int create_category(sqlite3 *db, int argc, char **args) {
    int i;
    int status;

    if (argc < 3) {
        printf("\x1b[31mInvalid statement: ");
        for (i = 0; i < argc; i++) {
            printf("%s ", *(args+i));
        }
        printf("\n\x1b[0m");
        return 1;
    }

    Category *myCategory = (Category *) calloc(1, sizeof(Category));
    strcpy(myCategory->name, args[2]);

    status = add_category(db, myCategory);

    free(myCategory);

    if (status == SQLITE_OK) {
        printf("\x1b[32m\u2714 Create category \"%s\" successfully.\n\x1b[0m", args[2]);
    } else {
        printf("\x1b[31m\u2717 Failed to create a category.\n\x1b[0m");
    }

    return 1;
}

static int create_transaction(sqlite3 *db, int argc, char **args) {
    int i;
    int status;

    if (argc < 5) {
        printf("\x1b[31mInvalid statement: ");
        for (i = 0; i < argc; i++) {
            printf("%s ", *(args+i));
        }
        printf("\n\x1b[0m");
        return 1;
    }

    Transaction *myTransaction = (Transaction *) calloc(1, sizeof(Transaction));
    strcpy(myTransaction->title, args[2]);
    strcpy(myTransaction->description, args[3]);
    myTransaction->amount = atof(args[4]);
    myTransaction->wallet_id = atoi(args[5]);
    myTransaction->category_id = atoi(args[6]);

    status = add_transaction(db, myTransaction);

    free(myTransaction);

    if (status == SQLITE_OK) {
        printf("\x1b[32m\u2714 Create transaction \"%s\" successfully.\n\x1b[0m", args[2]);
    } else {
        printf("\x1b[31m\u2717 Failed to create a transaction.\n\x1b[0m");
    }

    return 1;
}

static int delete_wallet(sqlite3 *db, int argc, char **args) {
    int i;
    int status;

    if (argc < 3) {
        printf("\x1b[31mInvalid statement: ");
        for (i = 0; i < argc; i++) {
            printf("%s ", *(args+i));
        }
        printf("\n\x1b[0m");
        return 1;
    }

    Wallet *myWallet = (Wallet *) calloc(1, sizeof(Wallet));
    myWallet->id = atoi(args[2]);

    status = remove_wallet(db, myWallet);

    free(myWallet);

    if (status == SQLITE_OK) {
        printf("\x1b[32m\u2714 Delete wallet \"%s\" successfully.\n\x1b[0m", args[2]);
    } else {
        printf("\x1b[31m\u2717 Failed to delete a wallet.\n\x1b[0m");
    }

    return 1;
}

static int delete_category(sqlite3 *db, int argc, char **args) {
    int i;
    int status;

    if (argc < 3) {
        printf("\x1b[31mInvalid statement: ");
        for (i = 0; i < argc; i++) {
            printf("%s ", *(args+i));
        }
        printf("\n\x1b[0m");
        return 1;
    }

    Category *myCategory = (Category *) calloc(1, sizeof(Category));
    myCategory->id = atoi(args[2]);

    status = remove_category(db, myCategory);

    free(myCategory);

    if (status == SQLITE_OK) {
        printf("\x1b[32m\u2714 Delete category \"%s\" successfully.\n\x1b[0m", args[2]);
    } else {
        printf("\x1b[31m\u2717 Failed to delete a category.\n\x1b[0m");
    }

    return 1;
}

static int delete_transaction(sqlite3 *db, int argc, char **args) {
    int i;
    int status;

    if (argc < 3) {
        printf("\x1b[31mInvalid statement: ");
        for (i = 0; i < argc; i++) {
            printf("%s ", *(args+i));
        }
        printf("\n\x1b[0m");
        return 1;
    }

    Transaction *myTransaction = (Transaction *) calloc(1, sizeof(Transaction));
    myTransaction->id = atoi(args[2]);

    status = remove_transaction(db, myTransaction);

    free(myTransaction);

    if (status == SQLITE_OK) {
        printf("\x1b[32m\u2714 Delete transaction \"%s\" successfully.\n\x1b[0m", args[2]);
    } else {
        printf("\x1b[31m\u2717 Failed to delete a transaction.\n\x1b[0m");
    }

    return 1;
}

static int sh_exit(sqlite3 *db, int argc, char **args) {
    return 0;
}

int sh_exec(sqlite3 *db, int argc, char **args) {
    int i;

    for (i = 0; i < NUM_SH_CMD; i++) {
        if (strcmp(args[0], lst_cmd[i]) == 0) {
            return (*cmd_func[i])(db, argc, args);
        }
    }

    printf("\x1b[31mInvalid statement: ");
    for (int i = 0; i < argc; i++) {
        printf("%s ", *(args+i));
    }
    printf("\n\x1b[0m");

    return 1;
}

void sh_spawn(DB_Handler *handler) {
    char *line;
    char **args;
    int argc;
    int code;

    log_info("Shell started.");

    printf("Welcome to myBudget!\n");

    do {
        printf("> ");
        line = sh_read_line();
        printf("CMD: %s\n", line);
        args = sh_read_args(line, &argc);

        code = sh_exec(handler->db, argc, args);

        free(line);
        clear_args(argc, args);
        free(args);
    } while (code != 0);
}