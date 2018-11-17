#ifndef DB_H
#define DB_H

#include <time.h>
#include "sqlite3.h"

#define DB_NAME "myBudget.db"

struct DB_Handler {
    sqlite3 *db;
    char *db_name;
    char *zErrMsg;
};

struct Wallet {
    int id;
    char name[32];
};

struct Category {
    int id;
    char name[32];
};

struct Transaction {
    int id;
    char title[64];
    char description[1024];
    double amount;
    int category_id;
    int wallet_id;
    time_t created;
};

typedef struct DB_Handler DB_Handler;
typedef struct Wallet Wallet;
typedef struct Category Category;
typedef struct Transaction Transaction;

struct DB_Handler *connect(char *);

int init_db(DB_Handler *);
int add_wallet(DB_Handler *, Wallet *);
int add_category(DB_Handler *, Category *);
int add_transaction(DB_Handler *, Transaction *);

#endif