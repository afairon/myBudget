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

int init_db();

int add_wallet(Wallet *);
int add_category(Category *);
int add_transaction(Transaction *);

Queue *get_wallets(Wallet *);
Queue *get_categories(Category *);
Queue *get_transactions(Transaction *);

Queue *get_categories_overview(Category *);

int remove_wallet(Wallet *);
int remove_category(Category *);
int remove_transaction(Transaction *);

unsigned int count_records(RECORD_TYPES);

void clear_queue(Queue *);

#endif