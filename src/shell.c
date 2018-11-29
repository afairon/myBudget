#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "db.h"
#include "shell.h"
#include "rxi/log.h"
#include "misc.h"
#include "sqlite3/sqlite3.h"

// List of commands
static char *lst_cmd[] = {
    "wallet",
    "category",
    "transaction",
    "export",
    "overview",
    "help",
    "exit"
};

// List of sub-commands
static char *lst_sub_cmd[] = {
    "add",
    "create",
    "display",
    "print",
    "show",
    "delete",
    "remove"
};

// Array of pointers to command
static int (*cmd_func[]) (int, char **) = {
    &wallet_cmd,
    &category_cmd,
    &transaction_cmd,
    &export_transactions,
    &categories_overview,
    &sh_help,
    &sh_exit
};

// Array of pointers to sub-command
static int (*sub_cmd_func[]) (RECORD_TYPES, Record *) = {
    &create_record,
    &create_record,
    &show_record,
    &show_record,
    &show_record,
    &delete_record,
    &delete_record
};

// Array of pointers to help
static int (*sh_cmd_help[]) (void) = {
    &wallet_help,
    &category_help,
    &transaction_help,
    &export_help,
    &overview_help
};

// sh_read_line allocates a memory space to store a string.
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
        
        if (c == '\n') {
            buffer[position] = '\0';
            return buffer;
        } else if (c == EOF) {
            buffer[position] = EOF;
            return buffer;
        } else {
            buffer[position] = c;
        }
        position++;
        
        // Buffer overflow
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

// sh_read_args splits the string into an array of strings.
static char **sh_read_args(char *line, int *argc) {
    char **args;
    char *token;
    int position = 0;

    // Create array of pointers to string
    args = (char **) malloc(sizeof(char *) * SH_ARGV_SIZE);

    if (!args) {
        log_fatal("Memory allocation error");
        exit(1);
    }

    // Tokenize line
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

// clear_args frees memory space occupied by the
// shell arguments.
static void clear_args(int argc, char **args) {
    int i;

    for (i = 0; i < argc; i++) {
        free(args[i]);
    }
    free(args);
}

// sh_is_int checks if the string is an integer.
static int sh_is_int(char *line) {
    int i;

    for (i = 0; line[i] != '\0'; i++) {
        switch (line[i]) {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            case '-':
                break;
            default:
                return 0;
        }
    }

    return 1;
}

// sh_is_float checks if the string is a float.
static int sh_is_float(char *line) {
    int i;

    for (i = 0; line[i] != '\0'; i++) {
        switch (line[i]) {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            case '.':
            case '-':
                break;
            default:
                return 0;
        }
    }

    return 1;
}

// create_wallet creates a wallet.
static int create_wallet(Wallet *wallet) {
    int status;

    status = add_wallet(wallet);

    if (status == SQLITE_OK) {
        pretty_success("Create wallet \"%s\" successfully", wallet->name);
    } else {
        pretty_fail("Failed to create a wallet");
    }

    return 1;
}

// create_category creates a category.
static int create_category(Category *category) {
    int status;

    status = add_category(category);

    if (status == SQLITE_OK) {
        pretty_success("Create category \"%s\" successfully", category->name);
    } else {
        pretty_fail("Failed to create a category");
    }

    return 1;
}

// create_transaction creates a transaction.
static int create_transaction(Transaction *transaction) {
    int status;

    status = add_transaction(transaction);

    if (status == SQLITE_OK) {
        pretty_success("Create transaction \"%s\" successfully", transaction->name);
    } else {
        pretty_fail("Failed to create a transaction");
    }

    return 1;
}

// show_wallets displays and formats wallets.
static int show_wallets(Wallet *wallet) {
    Queue *records, *tmprecord;

    records = get_wallets(wallet);
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

// show_categories displays and formats categories.
static int show_categories(Category *category) {
    Queue *records, *tmprecord;
    
    records = get_categories(category);
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

// show_transactions displays and formats transactions.
static int show_transactions(Transaction *transaction) {
    Queue *records, *tmprecord;
    
    records = get_transactions(transaction);
    tmprecord = records;
    printf("\n+--id--|------name------|----------description----------|----amount----|-----wallet----|----category----+\n");
    while (tmprecord != NULL) {
        printf("|%-6u|%-16.16s|%-31.31s|%14.2lf|%-15.15s|%-16.16s|\n",
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

// delete_wallet deletes the wallet.
static int delete_wallet(Wallet *wallet) {
    int status;

    status = remove_wallet(wallet);

    if (status == SQLITE_OK) {
        pretty_success("Delete wallet \"%s\" successfully", wallet->name);
    } else {
        pretty_fail("Failed to delete a wallet");
    }

    return 1;
}

// delete_category deletes the category.
static int delete_category(Category *category) {
    int status;

    status = remove_category(category);

    if (status == SQLITE_OK) {
        pretty_success("Delete category \"%s\" successfully", category->name);
    } else {
        pretty_fail("Failed to delete a category");
    }

    return 1;
}

// delete_transaction deletes the transaction.
static int delete_transaction(Transaction *transaction) {
    int status;

    status = remove_transaction(transaction);

    if (status == SQLITE_OK) {
        pretty_success("Delete transaction \"%s\" successfully", transaction->name);
    } else {
        pretty_fail("Failed to delete a transaction");
    }

    return 1;
}

// export_transactions exports transactions into a CSV file.
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

    records = get_transactions(NULL);
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

// categories_overview displays and format information about categories.
static int categories_overview(int argc, char **args) {
    double amount = 0.0L;
    Queue *records, *tmprecord;
    
    records = get_categories_overview(NULL);
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

// create_record prepares record to be inserted.
static int create_record(RECORD_TYPES type, Record *record) {
    Queue *wallets;
    Queue *categories;
    char *line;

    switch (type) {
        case WALLET_TYPE:
            // Check if wallet name is not empty
            if (record->wallet.name[0] == '\0') {
                for (;;) {
                    printf("Name: ");
                    line = sh_read_line();
                    if (line[0] == EOF) {
                        free(line);
                        return 0;
                    }
                    if (line[0] != '\0' && line[0] != 32) {
                        strcpy(record->wallet.name, line);
                        free(line);
                        break;
                    }
                    free(line);
                }
            }
            create_wallet(&record->wallet);
            break;
        case CATEGORY_TYPE:
            // Check if category name is not empty
            if (record->category.name[0] == '\0') {
                for (;;) {
                    printf("Name: ");
                    line = sh_read_line();
                    if (line[0] == EOF) {
                        free(line);
                        return 0;
                    }
                    if (line[0] != '\0' && line[0] != 32) {
                        strcpy(record->category.name, line);
                        free(line);
                        break;
                    }
                    free(line);
                }
            }
            create_category(&record->category);
            break;
        case TRANSACTION_TYPE:
            if (count_records(WALLET_TYPE) < 1) {
                pretty_fail("You cannot create a transaction without any wallet.");
                break;
            }
            // Check if transaction's name is not empty
            if (record->transaction.name[0] == '\0') {
                for (;;) {
                    printf("Name: ");
                    line = sh_read_line();
                    if (line[0] == EOF) {
                        free(line);
                        return 0;
                    }
                    if (line[0] != '\0' && line[0] != 32) {
                        strcpy(record->transaction.name, line);
                        free(line);
                        break;
                    }
                    free(line);
                }
            }
            // Check if transaction's description is not empty
            if (record->transaction.description[0] == '\0') {
                for (;;) {
                    printf("Description: ");
                    line = sh_read_line();
                    if (line[0] == EOF) {
                        free(line);
                        return 0;
                    }
                    if (line[0] != '\0' && line[0] != 32) {
                        strcpy(record->transaction.description, line);
                        free(line);
                        break;
                    }
                    free(line);
                }
            }
            if (record->transaction.amount == 0.0) {
                for (;;) {
                    printf("Amount: ");
                    line = sh_read_line();
                    if (line[0] == EOF) {
                        free(line);
                        return 0;
                    }
                    if (line[0] != '\0' && line[0] != 32 && sh_is_float(line)) {
                        record->transaction.amount = atof(line);
                        free(line);
                        break;
                    }
                    free(line);
                }
            }
            // Check if transaction linked to the wallet is not empty
            if (record->transaction.wallet.name[0] == '\0') {
                for (;;) {
                    printf("Wallet Name: ");
                    line = sh_read_line();
                    if (line[0] == EOF) {
                        free(line);
                        return 0;
                    }
                    strcpy(record->transaction.wallet.name, line);
                    wallets = get_wallets(&record->transaction.wallet);
                    if (wallets != NULL) {
                        if (!(record->transaction.wallet.name[0] == '\0')) {
                            record->transaction.wallet = wallets->record.wallet;
                            clear_queue(wallets);
                            free(line);
                            break;
                        }
                        clear_queue(wallets);
                    }
                    show_wallets(NULL);
                    free(line);
                }
            }
            if (record->transaction.category.name[0] == '\0' && count_records(CATEGORY_TYPE) > 0) {
                for (;;) {
                    printf("Category Name: ");
                    line = sh_read_line();
                    if (line[0] == EOF) {
                        free(line);
                        return 0;
                    }
                    strcpy(record->transaction.category.name, line);
                    categories = get_categories(&record->transaction.category);
                    if (categories != NULL) {
                        if (!(record->transaction.category.name[0] == '\0')) {
                            record->transaction.category = categories->record.category;
                        }
                        clear_queue(categories);
                        free(line);
                        break;
                    }
                    show_categories(NULL);
                    free(line);
                }
            }
            create_transaction(&record->transaction);
            break;
        default:
            break;
    }

    return 1;
}

// show_record displays formated records.
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

// delete_record creates a record filter to delete a record.
static int delete_record(RECORD_TYPES type, Record *record) {
    Queue *wallets;
    Queue *categories;
    Queue *transactions;
    char *line;

    switch (type) {
        case WALLET_TYPE:
            // List of wallets is empty
            if (count_records(WALLET_TYPE) < 1) {
                pretty_fail("No wallet is available.");
                break;
            }
            if (record->wallet.name[0] == '\0') {
                for (;;) {
                    printf("Name: ");
                    line = sh_read_line();
                    if (line[0] == EOF) {
                        free(line);
                        return 0;
                    }
                    strcpy(record->wallet.name, line);
                    wallets = get_wallets(&record->wallet);
                    if (wallets != NULL) {
                        if (!(record->wallet.name[0] == '\0')) {
                            record->wallet = wallets->record.wallet;
                        }
                        clear_queue(wallets);
                        free(line);
                        break;
                    }
                    show_wallets(NULL);
                    free(line);
                }
            }
            // Check if record exists
            if (record->wallet.id == 0) {
                break;
            }
            pretty_warning("Deleting a wallet will remove all transactions linked to this wallet.");
            printf("Would you like to continue (y/n)? ");
            if (getchar() != 'y' && getchar() != 'Y') {
                break;
            }
            delete_wallet(&record->wallet);
            break;
        case CATEGORY_TYPE:
            // Lists of categories is empty
            if (count_records(CATEGORY_TYPE) < 1) {
                pretty_fail("No category is available.");
                break;
            }
            if (record->category.name[0] == '\0') {
                for (;;) {
                    printf("Category Name: ");
                    line = sh_read_line();
                    if (line[0] == EOF) {
                        free(line);
                        return 0;
                    }
                    strcpy(record->category.name, line);
                    categories = get_categories(&record->category);
                    if (categories != NULL) {
                        if (!(record->category.name[0] == '\0')) {
                            record->category = categories->record.category;
                        }
                        clear_queue(categories);
                        free(line);
                        break;
                    }
                    show_categories(NULL);
                    free(line);
                }
            }
            // Check if record exists
            if (record->category.id == 0) {
                break;
            }
            delete_category(&record->category);
            break;
        case TRANSACTION_TYPE:
            // List of transactions is empty
            if (count_records(TRANSACTION_TYPE) < 1) {
                pretty_fail("No transaction is available.");
                break;
            }
            if (record->transaction.name[0] == '\0') {
                for (;;) {
                    printf("ID: ");
                    line = sh_read_line();
                    if (line[0] == EOF) {
                        free(line);
                        return 0;
                    }
                    if (line[0] != '\0' && line[0] != 32 && sh_is_int(line)) {
                        record->transaction.id = atoi(line);
                        transactions = get_transactions(&record->transaction);
                        if (transactions != NULL) {
                            record->transaction = transactions->record.transaction;
                            clear_queue(transactions);
                            free(line);
                            break;
                        }
                    }
                    free(line);
                    show_transactions(NULL);
                }   
            }
            // Check if records exists
            if (record->transaction.id == 0) {
                break;
            }
            delete_transaction(&record->transaction);
            break;
        default:
            break;
    }

    return 1;
}

// parse_wallet create a wallet structure from
// shell arguments.
static void parse_wallet(int argc, char **args, Wallet *wallet) {
    int i;
    Queue *wallets;

    for (i = 0; i < argc; i++) {
        switch (i) {
            // Name
            case 0:
                strcpy(wallet->name, args[i]);
                wallets = get_wallets(wallet);
                if (wallets != NULL) {
                    *wallet = wallets->record.wallet;
                    clear_queue(wallets);
                }
                break;
            default:
                break;
        }
    }
}

// parse_category create a category structure from
// shell arguments.
static void parse_category(int argc, char **args, Category *category) {
    int i;
    Queue *categories;

    for (i = 0; i < argc; i++) {
        switch (i) {
            // Name
            case 0:
                strcpy(category->name, args[i]);
                categories = get_categories(category);
                if (categories != NULL) {
                    *category = categories->record.category;
                    clear_queue(categories);
                }
                break;
            default:
                break;
        }
    }
}

// parse_transaction create a transaction structure from
// shell arguments.
static void parse_transaction(int argc, char **args, Transaction *transaction) {
    int i;
    Queue *wallets;
    Queue *categories;
    Queue *transactions;
    Wallet wallet;
    Category category;

    for (i = 0; i < argc; i++) {
        switch (i) {
            // Name
            case 0:
                strcpy(transaction->name, args[i]);
                transactions = get_transactions(transaction);
                if (transactions != NULL) {
                    *transaction = transactions->record.transaction;
                    clear_queue(transactions);
                }
                break;
            // Description
            case 1:
                strcpy(transaction->description, args[i]);
                break;
            // Amount
            case 2:
                if (sh_is_float(args[i])) {
                    transaction->amount = atoi(args[i]);
                }
                break;
            // Wallet
            case 3:
                strcpy(wallet.name, args[i]);
                wallets = get_wallets(&wallet);
                if (wallets != NULL) {
                    transaction->wallet = wallets->record.wallet;
                    clear_queue(wallets);
                }
                break;
            // Category
            case 4:
                strcpy(category.name, args[i]);
                categories = get_categories(&category);
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

// wallet_cmd handles interaction with wallet.
// It's responsible for creating wallet,
// displaying wallet and deleting wallet.
static int wallet_cmd(int argc, char **args) {
    int i;
    Record record;

    if (argc < 1 || args[0] == NULL) {
        pretty_fail("Expect argument to \"wallet\"");
        return 1;
    }

    record.wallet.id = 0;
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

// category_cmd handles interaction with category.
// It's responsible for creating category,
// displaying category and deleting category.
static int category_cmd(int argc, char **args) {
    int i;
    Record record;

    if (argc < 1 || args[0] == NULL) {
        pretty_fail("Expect argument to \"category\"");
        return 1;
    }

    record.category.id = 0;
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

// transaction_cmd handles interaction with transaction.
// It's responsible for creating transaction,
// displaying transaction and deleting transaction.
static int transaction_cmd(int argc, char **args) {
    int i;
    Record record;

    if (argc < 1 || args[0] == NULL) {
        pretty_fail("Expect argument to \"transaction\"");
        return 1;
    }

    record.transaction.name[0] = '\0';
    record.transaction.description[0] = '\0';
    record.transaction.amount = 0.0L;
    record.transaction.wallet.id = 0;
    record.transaction.wallet.name[0] = '\0';
    record.transaction.category.id = 0;
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

// wallet_help displays help for wallet.
static int wallet_help() {
    printf("\nwallet <cmd> [name]\n\n");
    printf("The commands are:\n\n");
    printf("\tadd\t\tadd a wallet\n");
    printf("\tremove\t\tremove a wallet\n");
    printf("\tshow\t\tshow a wallet\n");
    return 1;
}

// category_help displays help for category.
static int category_help() {
    printf("\ncategory <cmd> [name]\n\n");
    printf("The commands are:\n\n");
    printf("\tadd\t\tadd a category\n");
    printf("\tremove\t\tremove a category\n");
    printf("\tshow\t\tshow a category\n");
    return 1;
}

// transaction_help displays help for transaction.
static int transaction_help() {
    printf("\ntransaction <cmd> [name] [description] [amount] [wallet] [category]\n\n");
    printf("The commands are:\n\n");
    printf("\tadd\t\tadd a transaction\n");
    printf("\tremove\t\tremove a transaction\n");
    printf("\tshow\t\tshow a transaction\n");
    return 1;
}

// export_help displays help for export.
static int export_help() {
    printf("\nusage: export [filename]\n\n");
    return 1;
}

// overview_help displays help for overview command.
static int overview_help() {
    printf("\nusage: overview\n\n");
    return 1;
}

// sh_help displays the use manual for the application.
static int sh_help(int argc, char **args) {
    int i;

    // Display help for commands
    if (argc > 0) {
        for (i = 0; i < NUM_SH_CMD-2; i++) {
            if (strcmp(args[0], lst_cmd[i]) == 0) {
                return (*sh_cmd_help[i])();
            }
        }
    }

    printf("\nThe commands are:\n\n");
    printf("\twallet\t\tcommands for wallet\n");
    printf("\tcategory\tcommands for category\n");
    printf("\ttransaction\tcommands for transaction\n");
    printf("\texport\t\tcommands for export\n");
    printf("\toverview\tcommands for overview\n");
    printf("\thelp\t\tdisplay this message\n");
    printf("\texit\t\texit the program\n\n");

    printf("Use help <command> for more information about a command.\n\n");

    return 1;
}

// sh_exit terminates the shell.
static int sh_exit(int argc, char **args) {
    return 0;
}

// sh_exec executes shell command.
static int sh_exec(int argc, char **args) {
    int i;

    if (argc < 1) {
        return 1;
    }

    // Exit shell on EOF
    if (args[0][0] == EOF) {
        return 0;
    }

    for (i = 0; i < NUM_SH_CMD; i++) {
        if (strcmp(args[0], lst_cmd[i]) == 0) {

            // Execute command
            return (*cmd_func[i])(argc-1, args+1);
        }
    }

    pretty_fail("Invalid command \"%s\"", args[0]);

    return 1;
}

// sh_spawn initialize shell loop.
void sh_spawn() {
    char *line;
    char **args;
    char *motd;
    int argc;
    int code;

    motd = "" \
" _    _      _                            _                           ______           _            _   _ \n" \
"| |  | |    | |                          | |                          | ___ \\         | |          | | | |\n" \
"| |  | | ___| | ___ ___  _ __ ___   ___  | |_ ___    _ __ ___  _   _  | |_/ /_   _  __| | __ _  ___| |_| |\n" \
"| |/\\| |/ _ \\ |/ __/ _ \\| '_ ` _ \\ / _ \\ | __/ _ \\  | '_ ` _ \\| | | | | ___ \\ | | |/ _` |/ _` |/ _ \\ __| |\n" \
"\\  /\\  /  __/ | (_| (_) | | | | | |  __/ | || (_) | | | | | | | |_| | | |_/ / |_| | (_| | (_| |  __/ |_|_|\n" \
" \\/  \\/ \\___|_|\\___\\___/|_| |_| |_|\\___|  \\__\\___/  |_| |_| |_|\\__, | \\____/ \\__,_|\\__,_|\\__, |\\___|\\__(_)\n" \
"                                                                __/ |                     __/ |           \n" \
"                                                               |___/                     |___/            \n";

    pretty_info("Shell initialized.\nUse help for more information.");
    printf("%s\n", motd);

    do {
        printf("> ");
        // Read line
        line = sh_read_line();
        // Split line into arguments
        args = sh_read_args(line, &argc);

        // CMD execution
        code = sh_exec(argc, args);

        free(line);
        clear_args(argc, args);
    } while (code != 0);
}