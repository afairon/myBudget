#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "db.h"
#include "shell.h"
#include "rxi/log.h"
#include "misc.h"
#include "sqlite3/sqlite3.h"

sqlite3 *db;

static char *lst_cmd[] = {
    "wallet",
    "category",
    "transaction",
    "export",
    "report",
    "help",
    "exit"
};

static char *lst_sub_cmd[] = {
    "add",
    "create",
    "display",
    "print",
    "show",
    "delete",
    "remove"
};

static int (*cmd_func[]) (int, char **) = {
    &wallet_cmd,
    &category_cmd,
    &transaction_cmd,
    &export_transactions,
    &categories_report,
    &sh_help,
    &sh_exit
};

static int (*sub_cmd_func[]) (RECORD_TYPES, Record *) = {
    &create_record,
    &create_record,
    &show_record,
    &show_record,
    &show_record,
    &delete_record,
    &delete_record
};

static char *sh_read_line(void) {
    int bufsize = SH_BUFFER_SIZE;
    int position = 0;
    char *buffer = (char *) malloc(sizeof(char) * bufsize);
    int c;
    
    if (!buffer) {
        log_fatal("Memory allocation error");
        exit(1);
    }
    
    for (;;) {
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
            buffer = (char *) realloc(buffer, bufsize);
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

static void clear_args(int argc, char **args) {
    int i;

    for (i = 0; i < argc; i++) {
        free(args[i]);
    }
    free(args);
}

static int create_wallet(Wallet *wallet) {
    int status;

    status = add_wallet(db, wallet);

    if (status == SQLITE_OK) {
        pretty_success("Create wallet \"%s\" successfully", wallet->name);
    } else {
        pretty_fail("Failed to create a wallet");
    }

    return 1;
}

static int create_category(Category *category) {
    int status;

    status = add_category(db, category);

    if (status == SQLITE_OK) {
        pretty_success("Create category \"%s\" successfully", category->name);
    } else {
        pretty_fail("Failed to create a category");
    }

    return 1;
}

static int create_transaction(Transaction *transaction) {
    int status;

    status = add_transaction(db, transaction);

    if (status == SQLITE_OK) {
        pretty_success("Create transaction \"%s\" successfully", transaction->name);
    } else {
        pretty_fail("Failed to create a transaction");
    }

    return 1;
}

static int show_wallets(Wallet *wallet) {
    Queue *records, *tmprecord;

    records = get_wallets(db, wallet);
    tmprecord = records;
    printf("\n+--id--|--------------name--------------|-----balance----+\n");
    while (tmprecord != NULL) {
        printf("|%-6u|%-32.32s|%16.2lf|\n",
            tmprecord->record.wallet.id,
            tmprecord->record.wallet.name,
            tmprecord->record.wallet.balance
        );
        tmprecord = tmprecord->next;
    }
    
    printf("+--------------------------------------------------------+\n");

    clear_queue(records);

    return 1;
}

static int show_categories(Category *category) {
    Queue *records, *tmprecord;
    
    records = get_categories(db, category);
    tmprecord = records;
    printf("\n+--id--|--------------name--------------+\n");
    while (tmprecord != NULL) {
        printf("|%-6u|%-32.32s|\n",
            tmprecord->record.category.id,
            tmprecord->record.category.name
        );
        tmprecord = tmprecord->next;
    }

    printf("+---------------------------------------+\n");

    clear_queue(records);

    return 1;
}

static int show_transactions(Transaction *transaction) {
    Queue *records, *tmprecord;
    
    records = get_transactions(db, transaction);
    tmprecord = records;
    printf("\n+--id--|------name------|----------description----------|----amount----|-----wallet----|----category----+\n");
    while (tmprecord != NULL) {
        printf("|%-6u|%-16.16s|%-31.31s|%14.2lf|%-15.5s|%-16.16s|\n",
            tmprecord->record.transaction.id,
            tmprecord->record.transaction.name,
            tmprecord->record.transaction.description,
            tmprecord->record.transaction.amount,
            tmprecord->record.transaction.wallet.name,
            tmprecord->record.transaction.category.name
        );
        tmprecord = tmprecord->next;
    }

    printf("+-------------------------------------------------------------------------------------------------------+\n");

    clear_queue(records);

    return 1;
}

static int delete_wallet(Wallet *wallet) {
    int status;

    status = remove_wallet(db, wallet);

    if (status == SQLITE_OK) {
        pretty_success("Delete wallet \"%s\" successfully", wallet->name);
    } else {
        pretty_fail("Failed to delete a wallet");
    }

    return 1;
}

static int delete_category(Category *category) {
    int status;

    status = remove_category(db, category);

    if (status == SQLITE_OK) {
        pretty_success("Delete category \"%s\" successfully", category->name);
    } else {
        pretty_fail("Failed to delete a category");
    }

    return 1;
}

static int delete_transaction(Transaction *transaction) {
    int status;

    status = remove_transaction(db, transaction);

    if (status == SQLITE_OK) {
        pretty_success("Delete transaction \"%s\" successfully", transaction->name);
    } else {
        pretty_fail("Failed to delete a transaction");
    }

    return 1;
}

static int export_transactions(int argc, char **args) {
    int i;
    char *fileName;
    FILE *outFile;
    Queue *records, *tmprecords;

    if (argc < 1) {
        printf("File name: ");
        fileName = sh_read_line();
    } else {
        fileName = args[0];
    }

    outFile = fopen(fileName, "w");

    if (outFile == NULL) {
        log_fatal("Couldn't open file \"%s\"", fileName);
        return 1;
    }

    records = get_transactions(db, NULL);
    tmprecords = records;

    fprintf(outFile, "id,title,description,amount,wallet,category\n");

    while (tmprecords != NULL) {
        fprintf(outFile, "%u,%s,%s,%.2lf,%s,%s\n",
            tmprecords->record.transaction.id,
            tmprecords->record.transaction.name,
            tmprecords->record.transaction.description,
            tmprecords->record.transaction.amount,
            tmprecords->record.transaction.wallet.name,
            tmprecords->record.transaction.category.name
        );
        tmprecords = tmprecords->next;
    }

    clear_queue(records);

    fclose(outFile);

    pretty_success("Data exported to \"%s\"", fileName);

    free(fileName);

    return 1;
}

static int categories_report(int argc, char **args) {
    double amount = 0.0L;
    Queue *records, *tmprecord;
    
    records = get_categories_report(db, NULL);
    tmprecord = records;
    printf("\n+--id--|--------------name--------------|-----amount----+\n");
    while (tmprecord != NULL) {
        printf("|%-6u|%-32.32s|%15.2lf|\n",
            tmprecord->record.category.id,
            tmprecord->record.category.name,
            tmprecord->record.category.amount
        );
        amount += tmprecord->record.category.amount;
        tmprecord = tmprecord->next;
    }

    printf("+------|--------------------------------|-----amount----+\n");

    printf("|%-6u|%-32.32s|%15.2lf|\n", 0, "Total", amount);

    printf("+-------------------------------------------------------+\n");

    clear_queue(records);

    return 1;
}

static int create_record(RECORD_TYPES type, Record *record) {
    Queue *wallets;
    Queue *categories;

    switch (type) {
        case WALLET_TYPE:
            if (record->wallet.name[0] == '\0') {
                printf("Name: ");
                gets(record->wallet.name);
            }
            create_wallet(&record->wallet);
            break;
        case CATEGORY_TYPE:
            if (record->category.name[0] == '\0') {
                printf("Name: ");
                gets(record->category.name);   
            }
            create_category(&record->category);
            break;
        case TRANSACTION_TYPE:
            if (record->transaction.name[0] == '\0') {
                printf("Name: ");
                gets(record->transaction.name);
            }
            if (record->transaction.description[0] == '\0') {
                printf("Description: ");
                gets(record->transaction.description);
            }
            if (record->transaction.amount == 0.0) {
                printf("Amount: ");
                scanf("%lf", &record->transaction.amount);
                while (getchar() != '\n');
            }

            if (record->transaction.wallet.name[0] == '\0') {
                for (;;) {
                    printf("Wallet Name: ");
                    gets(record->transaction.wallet.name);
                    wallets = get_wallets(db, &record->transaction.wallet);
                    if (wallets != NULL) {
                        if (!(record->transaction.wallet.name[0] == '\0')) {
                            record->transaction.wallet = wallets->record.wallet;
                            clear_queue(wallets);
                            break;
                        }
                        clear_queue(wallets);
                    }
                    show_wallets(NULL);
                }
            }

            if (record->transaction.category.name[0] == '\0') {
                for (;;) {
                    printf("Category Name: ");
                    gets(record->transaction.category.name);
                    categories = get_categories(db, &record->transaction.category);
                    if (categories != NULL) {
                        if (!(record->transaction.category.name[0] == '\0')) {
                            record->transaction.category = categories->record.category;
                        }
                        clear_queue(categories);
                        break;
                    }
                    show_categories(NULL);
                }
            }
            create_transaction(&record->transaction);
            break;
        default:
            break;
    }

    return 1;
}

static int show_record(RECORD_TYPES type, Record *record) {
    switch (type) {
        case WALLET_TYPE:
            show_wallets(&record->wallet);
            break;
        case CATEGORY_TYPE:
            show_categories(&record->category);
            break;
        case TRANSACTION_TYPE:
            show_transactions(&record->transaction);
            break;
        default:
            break;
    }

    return 1;
}

static int delete_record(RECORD_TYPES type, Record *record) {
    switch (type) {
        case WALLET_TYPE:
            printf("ID: ");
            scanf("%d", &record->wallet.id);
            while (getchar() != '\n');
            printf("Name: ");
            gets(record->wallet.name);
            pretty_warning("Deleting a wallet will remove all transactions linked to this wallet.");
            printf("Would you like to continue (y/n)? ");
            if (getchar() != 'y' && getchar() != 'Y') {
                break;
            }
            delete_wallet(&record->wallet);
            break;
        case CATEGORY_TYPE:
            printf("ID: ");
            scanf("%d", &record->category.id);
            while (getchar() != '\n');
            printf("Name: ");
            gets(record->category.name);
            delete_category(&record->category);
            break;
        case TRANSACTION_TYPE:
            printf("ID: ");
            scanf("%d", &record->transaction.id);
            delete_transaction(&record->transaction);
            break;
        default:
            break;
    }

    while (getchar() != '\n');

    return 1;
}

static void parse_wallet(int argc, char **args, Wallet *wallet) {
    int i;

    for (i = 0; i < argc; i++) {
        switch (i) {
            case 0:
                strcpy(wallet->name, args[i]);
                break;
            default:
                break;
        }
    }
}

static void parse_category(int argc, char **args, Category *category) {
    int i;

    for (i = 0; i < argc; i++) {
        switch (i) {
            case 0:
                strcpy(category->name, args[i]);
                break;
            default:
                break;
        }
    }
}

static void parse_transaction(int argc, char **args, Transaction *transaction) {
    int i;
    Queue *wallets;
    Queue *categories;
    Wallet wallet;
    Category category;

    for (i = 0; i < argc; i++) {
        switch (i) {
            case 0:
                strcpy(transaction->name, args[i]);
                break;
            case 1:
                strcpy(transaction->description, args[i]);
                break;
            case 2:
                transaction->amount = atoi(args[i]);
                break;
            case 3:
                strcpy(wallet.name, args[i]);
                wallets = get_wallets(db, &wallet);
                if (wallets != NULL) {
                    transaction->wallet = wallets->record.wallet;
                    clear_queue(wallets);
                }
                break;
            case 4:
                strcpy(category.name, args[i]);
                categories = get_categories(db, &category);
                if (categories != NULL) {
                    transaction->category = categories->record.category;
                    clear_queue(categories);
                }
                break;
            default:
                break;
        }
    }
}

static int wallet_cmd(int argc, char **args) {
    int i;
    Record record;

    if (argc < 1 || args[0] == NULL) {
        pretty_fail("Expect argument to \"wallet\"");
        return 1;
    }

    record.wallet.name[0] = '\0';

    parse_wallet(argc-1, args+1, &record.wallet);

    for (i = 0; i < NUM_SH_SUB_CMD; i++) {
        if (strcmp(args[0], lst_sub_cmd[i]) == 0) {
            return (*sub_cmd_func[i])(WALLET_TYPE, &record);
        }
    }

    pretty_fail("Invalid command \"%s\" for wallet", args[0]);

    return 1;
}

static int category_cmd(int argc, char **args) {
    int i;
    Record record;

    if (argc < 1 || args[0] == NULL) {
        pretty_fail("Expect argument to \"category\"");
        return 1;
    }

    record.category.name[0] = '\0';

    parse_category(argc-1, args+1, &record.category);

    for (i = 0; i < NUM_SH_SUB_CMD; i++) {
        if (strcmp(args[0], lst_sub_cmd[i]) == 0) {
            return (*sub_cmd_func[i])(CATEGORY_TYPE, &record);
        }
    }

    pretty_fail("Invalid command \"%s\" for category", args[0]);

    return 1;
}

static int transaction_cmd(int argc, char **args) {
    int i;
    Record record;

    if (argc < 1 || args[0] == NULL) {
        pretty_fail("Expect argument to \"transaction\"");
        return 1;
    }

    record.transaction.name[0] = '\0';
    record.transaction.description[0] = '\0';
    record.transaction.amount = 0.0;
    record.transaction.wallet.name[0] = '\0';
    record.transaction.category.name[0] = '\0';

    parse_transaction(argc-1, args+1, &record.transaction);

    for (i = 0; i < NUM_SH_SUB_CMD; i++) {
        if (strcmp(args[0], lst_sub_cmd[i]) == 0) {
            return (*sub_cmd_func[i])(TRANSACTION_TYPE, &record);
        }
    }

    pretty_fail("Invalid command \"%s\" for transaction", args[0]);

    return 1;
}

static int sh_help(int argc, char **args) {
    printf("\nThe commands are:\n\n");
    printf("\twallet\t\tcommands for wallet\n");
    printf("\tcategory\tcommands for category\n");
    printf("\ttransaction\tcommands for transaction\n");
    printf("\texport\t\tcommands for export\n");
    printf("\treport\t\tcommands for report\n");
    printf("\thelp\t\tdisplay this message\n");
    printf("\texit\t\texit the program\n\n");

    printf("Use help <command> for more information about a command.\n\n");

    return 1;
}

static int sh_exit(int argc, char **args) {
    return 0;
}

static int sh_exec(int argc, char **args) {
    int i;

    if (argc < 1) {
        return 1;
    }

    for (i = 0; i < NUM_SH_CMD; i++) {
        if (strcmp(args[0], lst_cmd[i]) == 0) {
            return (*cmd_func[i])(argc-1, args+1);
        }
    }

    pretty_fail("Invalid command \"%s\"", args[0]);

    return 1;
}

void sh_spawn(DB_Handler *handler) {
    char *line;
    char **args;
    int argc;
    int code;

    db = handler->db;

    printf("Welcome to myBudget!\n");

    do {
        printf("> ");
        line = sh_read_line();
        args = sh_read_args(line, &argc);

        code = sh_exec(argc, args);

        free(line);
        clear_args(argc, args);
    } while (code != 0);
}