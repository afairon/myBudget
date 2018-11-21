#ifndef DB_H
#define DB_H

#include <time.h>
#include "sqlite3/sqlite3.h"

#define DB_NAME "myBudget.db"

typedef enum RECORD_TYPES {
    WALLET_TYPE,
    CATEGORY_TYPE,
    TRANSACTION_TYPE
} RECORD_TYPES;

typedef struct DB_Handler {
    sqlite3 *db;
    char db_name[32];
} DB_Handler;

typedef struct Wallet {
    unsigned int id;
    char name[32];
    double balance;
} Wallet;

typedef struct Category {
    unsigned int id;
    char name[32];
    double amount;
} Category;

typedef struct Transaction {
    unsigned int id;
    char name[64];
    char description[1024];
    double amount;
    Wallet wallet;
    Category category;
} Transaction;

typedef union Record {
    Wallet wallet;
    Category category;
    Transaction transaction;
} Record;

typedef struct Queue {
    union Record record;
    struct Queue *next;
} Queue;

DB_Handler *connect(const char *);

int init_db(sqlite3 *);

int add_wallet(sqlite3 *, Wallet *);
int add_category(sqlite3 *, Category *);
int add_transaction(sqlite3 *, Transaction *);

Queue *get_wallets(sqlite3 *, Wallet *);
Queue *get_categories(sqlite3 *, Category *);
Queue *get_transactions(sqlite3 *, Transaction *);

Queue *get_categories_report(sqlite3 *, Category *);

int remove_wallet(sqlite3 *, Wallet *);
int remove_category(sqlite3 *, Category *);
int remove_transaction(sqlite3 *, Transaction *);

void clear_queue(Queue *);

#endif