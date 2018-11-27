#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "db.h"
#include "rxi/log.h"
#include "sqlite3/sqlite3.h"

// SQLite connetion
sqlite3 *db;

// connect establishes a connection to SQLite database.
DB_Handler *connect(const char *name) {
    int rc;

    if (name == NULL || name[0] == '\0') {
        name = DB_NAME;
    }

    DB_Handler *handler = (DB_Handler *)malloc(sizeof(DB_Handler));
    strcpy(handler->db_name, name);

    rc = sqlite3_open(name, &db);

    if (rc != SQLITE_OK) {
        handler->db = NULL;
        sqlite3_close(db);
        return handler;
    }

    handler->db = db;

    return handler;
}

// init_db creates three tables : wallets, categories and transactions.
// This also creates indexes.
int init_db() {
    char *sql;
    char *zErrMsg = 0;
    int rc;

    sql = "BEGIN;" \

        "CREATE TABLE IF NOT EXISTS wallets(" \
        "id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL," \
        "name VARCHAR(32) UNIQUE NOT NULL" \
        ");" \

        "CREATE TABLE IF NOT EXISTS categories(" \
        "id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL," \
        "name VARCHAR(32) UNIQUE NOT NULL" \
        ");" \

        "CREATE TABLE IF NOT EXISTS transactions(" \
        "id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL," \
        "name VARCHAR(64) NOT NULL," \
        "description TEXT," \
        "amount REAL NOT NULL," \
        "wallet_id INTEGER NOT NULL," \
        "category_id INTEGER," \
        "FOREIGN KEY(wallet_id) REFERENCES wallets(id)," \
        "FOREIGN KEY(category_id) REFERENCES categories(id)" \
        ");" \

        "CREATE INDEX IF NOT EXISTS idx_wallet ON wallets(" \
        "id," \
        "name" \
        ");" \

        "CREATE INDEX IF NOT EXISTS idx_category ON categories(" \
        "id," \
        "name" \
        ");" \

        "CREATE INDEX IF NOT EXISTS idx_transaction ON transactions(" \
        "name," \
        "amount,"\
        "wallet_id," \
        "category_id" \
        ");" \

        "COMMIT;";
    
    rc = sqlite3_exec(db, sql, NULL, 0, &zErrMsg);

    if (rc != SQLITE_OK) {
        log_fatal("%s", zErrMsg);
    }

    sqlite3_free(zErrMsg);

    return rc;
}

// add_wallet inserts a new wallet into the database.
int add_wallet(Wallet *wallet) {
    char *sql;
    char *zErrMsg = 0;
    int rc;

    sql = sqlite3_mprintf("INSERT INTO wallets(name)" \
        "VALUES('%q');",
        wallet->name
    );
    
    rc = sqlite3_exec(db, sql, NULL, 0, &zErrMsg);

    sqlite3_free(sql);

    if (rc != SQLITE_OK) {
        log_warn("%s", zErrMsg);
    }

    sqlite3_free(zErrMsg);

    return rc;
}

// add_category inserts a new category into the database.
int add_category(Category *category) {
    char *sql;
    char *zErrMsg = 0;
    int rc;

    sql = sqlite3_mprintf("INSERT INTO categories(name)" \
        "VALUES('%q');",
        category->name
    );
    
    rc = sqlite3_exec(db, sql, NULL, 0, &zErrMsg);

    sqlite3_free(sql);

    if (rc != SQLITE_OK) {
        log_warn("%s", zErrMsg);
    }

    sqlite3_free(zErrMsg);

    return rc;
}

// add_transaction inserts a new transaction into the database.
int add_transaction(Transaction *transaction) {
    char *sql;
    char *zErrMsg = 0;
    int rc;

    sql = sqlite3_mprintf("INSERT INTO transactions(" \
        "name," \
        "description," \
        "amount," \
        "wallet_id," \
        "category_id)" \
        "VALUES(" \
        "'%q'," \
        "'%q'," \
        "%lf," \
        "%d," \
        "%d" \
        ");",
        transaction->name,
        transaction->description,
        transaction->amount,
        transaction->wallet.id,
        transaction->category.id
    );
    
    rc = sqlite3_exec(db, sql, NULL, 0, &zErrMsg);

    sqlite3_free(sql);

    if (rc != SQLITE_OK) {
        log_warn("%s", zErrMsg);
    }

    sqlite3_free(zErrMsg);

    return rc;
}

// get_wallets retrieves wallets and put them into
// a linked list.
Queue *get_wallets(Wallet *wallet) {
    char *sql;
    int rc;
    Queue *origin, *last;
    sqlite3_stmt *stmt;

    origin = NULL;

    sql = "SELECT wallets.id," \
        "wallets.name," \
        "SUM(transactions.amount) AS balance " \
        "FROM wallets " \
        "LEFT JOIN transactions ON wallets.id = transactions.wallet_id " \
        "GROUP BY wallets.name " \
        "ORDER BY wallets.id ASC;";

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

    if (rc != SQLITE_OK) {
        return origin;
    }

    while (sqlite3_step(stmt) != SQLITE_DONE) {
        unsigned int id = sqlite3_column_int(stmt, 0);
        const char *name = (const char *) sqlite3_column_text(stmt, 1);
        double balance = sqlite3_column_double(stmt, 2);

        // Filter
        if (wallet != NULL && wallet->name[0] != '\0') {
            if (strcmp(name, wallet->name) != 0 && id != wallet->id) {
                continue;
            }
        }

        Queue *record = (Queue *) malloc(sizeof(Queue));

        if (!record) {
            log_fatal("Memory allocation error");
            exit(1);
        }

        record->record.wallet.id = id;
        if (name != NULL) {
            strcpy(record->record.wallet.name, name);
        } else {
            record->record.wallet.name[0] = '\0';
        }
        record->record.wallet.balance = balance;
        record->next = NULL;

        if (origin != NULL) {
            last->next = record;
            last = record;
        } else {
            origin = record;
            last = record;
        }
    }

    sqlite3_finalize(stmt);

    return origin;
}

// get_categories retrieves categories and put them into
// a linked list.
Queue *get_categories(Category *category) {
    char *sql;
    int rc;
    Queue *origin, *last;
    sqlite3_stmt *stmt;

    origin = NULL;

    sql = "SELECT categories.id," \
        "categories.name " \
        "FROM categories;";

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

    if (rc != SQLITE_OK) {
        return origin;
    }

    while (sqlite3_step(stmt) != SQLITE_DONE) {
        unsigned int id = sqlite3_column_int(stmt, 0);
        const char *name = (const char *) sqlite3_column_text(stmt, 1);

        // Filter
        if (category != NULL && category->name[0] != '\0') {
            if (strcmp(name, category->name) != 0 && id != category->id) {
                continue;
            }
        }

        Queue *record = (Queue *) malloc(sizeof(Queue));

        if (!record) {
            log_fatal("Memory allocation error");
            exit(1);
        }

        record->record.category.id = id;
        if (name != NULL) {
            strcpy(record->record.category.name, name);
        } else {
            record->record.category.name[0] = '\0';
        }
        record->next = NULL;

        if (origin != NULL) {
            last->next = record;
            last = record;
        } else {
            origin = record;
            last = record;
        }
    }

    sqlite3_finalize(stmt);

    return origin;
}

// get_transactions retrieves transactions and put them into
// a linked list.
Queue *get_transactions(Transaction *transaction) {
    char *sql;
    int rc;
    Queue *origin, *last;
    sqlite3_stmt *stmt;

    origin = NULL;

    sql = "SELECT transactions.id," \
        "transactions.name," \
        "transactions.description," \
        "transactions.amount," \
        "transactions.wallet_id," \
        "wallets.name AS wallet," \
        "transactions.category_id," \
        "categories.name AS category " \
        "FROM transactions " \
        "LEFT JOIN wallets ON transactions.wallet_id = wallets.id " \
        "LEFT JOIN categories ON transactions.category_id = categories.id " \
        "GROUP BY transactions.id;";

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

    if (rc != SQLITE_OK) {
        return origin;
    }

    while (sqlite3_step(stmt) != SQLITE_DONE) {
        unsigned int id = sqlite3_column_int(stmt, 0);
        const char *name = (const char *) sqlite3_column_text(stmt, 1);
        const char *description = (const char *) sqlite3_column_text(stmt, 2);
        double amount = sqlite3_column_double(stmt, 3);
        unsigned int wallet_id = sqlite3_column_int(stmt, 4);
        const char *wallet_name = (const char *) sqlite3_column_text(stmt, 5);
        unsigned int category_id = sqlite3_column_int(stmt, 6);
        const char *category_name = (const char *) sqlite3_column_text(stmt, 7);

        // Filter
        if (transaction != NULL && transaction->name[0] != '\0') {
            if (strcmp(name, transaction->name) != 0) {
                continue;
            }
        }

        Queue *record = (Queue *) malloc(sizeof(Queue));

        if (!record) {
            log_fatal("Memory allocation error");
            exit(1);
        }
        
        record->record.transaction.id = id;
        if (name != NULL) {
            strcpy(record->record.transaction.name, name);
        } else {
            record->record.transaction.name[0] = '\0';
        }
        if (description != NULL) {
            strcpy(record->record.transaction.description, description);
        } else {
            record->record.transaction.description[0] = '\0';
        }
        record->record.transaction.amount = amount;
        record->record.transaction.wallet.id = id;
        if (wallet_name != NULL) {
            strcpy(record->record.transaction.wallet.name, wallet_name);
        } else {
            record->record.transaction.wallet.name[0] = '\0';
        }
        record->record.transaction.category.id = category_id;
        if (category_name != NULL) {
            strcpy(record->record.transaction.category.name, category_name);
        } else {
            record->record.transaction.category.name[0] = '\0';
        }
        record->next = NULL;

        if (origin != NULL) {
            last->next = record;
            last = record;
        } else {
            origin = record;
            last = record;
        }
    }

    sqlite3_finalize(stmt);

    return origin;
}

// get_categories_overview retrieves categories, spent amounts and put them into
// a linked list.
Queue *get_categories_overview(Category *category) {
    char *sql;
    int rc;
    Queue *origin, *last;
    sqlite3_stmt *stmt;

    origin = NULL;

    sql = "SELECT categories.id," \
        "categories.name," \
        "SUM(transactions.amount) AS amount " \
        "FROM categories " \
        "LEFT JOIN transactions ON categories.id = transactions.category_id " \
        "GROUP BY categories.name " \
        "ORDER BY amount ASC;";

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

    if (rc != SQLITE_OK) {
        return origin;
    }

    while (sqlite3_step(stmt) != SQLITE_DONE) {
        Queue *record = (Queue *) malloc(sizeof(Queue));

        if (!record) {
            log_fatal("Memory allocation error");
            exit(1);
        }

        unsigned int id = sqlite3_column_int(stmt, 0);
        const char *name = (const char *) sqlite3_column_text(stmt, 1);
        double amount = sqlite3_column_double(stmt, 2);

        record->record.category.id = id;
        if (name != NULL) {
            strcpy(record->record.category.name, name);
        } else {
            record->record.category.name[0] = '\0';
        }
        record->record.category.amount = amount;
        record->next = NULL;

        if (origin != NULL) {
            last->next = record;
            last = record;
        } else {
            origin = record;
            last = record;
        }
    }

    sqlite3_finalize(stmt);

    return origin;
}

// remove_wallet deletes wallet from database.
int remove_wallet(Wallet *wallet) {
    char *sql;
    char *zErrMsg = 0;
    int rc;

    sql = sqlite3_mprintf("BEGIN;" \
        "DELETE FROM transactions WHERE " \
        "transactions.wallet_id = %d;" \
        "DELETE FROM wallets WHERE " \
        "wallets.id = %d;" \
        "COMMIT;",
        wallet->id,
        wallet->id
    );
    
    rc = sqlite3_exec(db, sql, NULL, 0, &zErrMsg);

    sqlite3_free(sql);

    if (rc != SQLITE_OK) {
        log_warn("%s", zErrMsg);
    }

    sqlite3_free(zErrMsg);

    return rc;
}

// remove_category deletes category from database.
int remove_category(Category *category) {
    char *sql;
    char *zErrMsg = 0;
    int rc;

    sql = sqlite3_mprintf("BEGIN;" \
        "UPDATE transactions SET category_id = 0 WHERE " \
        "category_id = %d;" \
        "DELETE FROM categories WHERE " \
        "id = %d;" \
        "COMMIT;",
        category->id,
        category->id
    );
    
    rc = sqlite3_exec(db, sql, NULL, 0, &zErrMsg);

    sqlite3_free(sql);

    if (rc != SQLITE_OK) {
        log_warn("%s", zErrMsg);
    }

    sqlite3_free(zErrMsg);

    return rc;
}

// remove_transaction removes transaction from database.
int remove_transaction(Transaction *transaction) {
    char *sql;
    char *zErrMsg = 0;
    int rc;

    sql = sqlite3_mprintf("BEGIN;" \
        "DELETE FROM transactions WHERE " \
        "transactions.id = %d;" \
        "COMMIT;",
        transaction->id,
        transaction->name
    );
    
    rc = sqlite3_exec(db, sql, NULL, 0, &zErrMsg);

    sqlite3_free(sql);

    if (rc != SQLITE_OK) {
        log_warn("%s", zErrMsg);
    }

    sqlite3_free(zErrMsg);

    return rc;
}

// count_records returns number of records in the table.
unsigned int count_records(RECORD_TYPES type) {
    char *sql;
    int rc;
    unsigned int count = 0;
    sqlite3_stmt *stmt;

    switch (type) {
        case WALLET_TYPE:
            sql = "SELECT COUNT(*) from wallets;";
            break;
        case CATEGORY_TYPE:
            sql = "SELECT COUNT(*) from categories;";
            break;
        case TRANSACTION_TYPE:
            sql = "SELECT COUNT(*) from transactions;";
            break;
        default:
            return 0;
    }

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

    while (sqlite3_step(stmt) != SQLITE_DONE) {
        count = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);

    return count;
}

// clear_queue frees up the memory.
void clear_queue(Queue *origin) {
    Queue *temp;
    while (origin != NULL) {
        temp = origin;
        origin = origin->next;
        free(temp);
    }
}