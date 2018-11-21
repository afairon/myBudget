#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "db.h"
#include "rxi/log.h"
#include "sqlite3/sqlite3.h"

DB_Handler *connect(const char *name) {
    sqlite3 *db;
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

int init_db(sqlite3 *db) {
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

int add_wallet(sqlite3 *db, Wallet *wallet) {
    char *sql;
    char *zErrMsg = 0;
    int rc;

    sql = sqlite3_mprintf("INSERT INTO wallets(name)" \
        "VALUES('%q');",
        wallet->name
    );
    
    rc = sqlite3_exec(db, sql, NULL, 0, &zErrMsg);

    if (rc != SQLITE_OK) {
        log_warn("%s", zErrMsg);
    }

    sqlite3_free(sql);
    sqlite3_free(zErrMsg);

    return rc;
}

int add_category(sqlite3 *db, Category *category) {
    char *sql;
    char *zErrMsg = 0;
    int rc;

    sql = sqlite3_mprintf("INSERT INTO categories(name)" \
        "VALUES('%q');",
        category->name
    );
    
    rc = sqlite3_exec(db, sql, NULL, 0, &zErrMsg);

    if (rc != SQLITE_OK) {
        log_warn("%s", zErrMsg);
    }

    sqlite3_free(sql);
    sqlite3_free(zErrMsg);

    return rc;
}

int add_transaction(sqlite3 *db, Transaction *transaction) {
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

    if (rc != SQLITE_OK) {
        log_warn("%s", zErrMsg);
    }

    sqlite3_free(sql);
    sqlite3_free(zErrMsg);

    return rc;
}

Queue *get_wallets(sqlite3 *db, Wallet *wallet) {
    char *sql;
    int rc;
    Queue *origin, *last;
    sqlite3_stmt *stmt;

    origin = NULL;

    sql = sqlite3_mprintf("SELECT wallets.id," \
        "wallets.name," \
        "SUM(transactions.amount) AS balance " \
        "FROM wallets " \
        "LEFT JOIN transactions ON wallets.id = transactions.wallet_id " \
        "GROUP BY wallets.name " \
        "ORDER BY wallets.id ASC;"
    );

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

    sqlite3_free(sql);

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

Queue *get_categories(sqlite3 *db, Category *category) {
    char *sql;
    int rc;
    Queue *origin, *last;
    sqlite3_stmt *stmt;

    origin = NULL;

    sql = sqlite3_mprintf("SELECT categories.id," \
        "categories.name " \
        "FROM categories;"
    );

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

    sqlite3_free(sql);

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

Queue *get_transactions(sqlite3 *db, Transaction *transaction) {
    char *sql;
    int rc;
    Queue *origin, *last;
    sqlite3_stmt *stmt;

    origin = NULL;

    sql = sqlite3_mprintf("SELECT transactions.id," \
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
        "GROUP BY transactions.id;"
    );

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

    sqlite3_free(sql);

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

Queue *get_categories_report(sqlite3 *db, Category *category) {
    char *sql;
    int rc;
    Queue *origin, *last;
    sqlite3_stmt *stmt;

    origin = NULL;

    sql = sqlite3_mprintf("SELECT categories.id," \
        "categories.name," \
        "SUM(transactions.amount) AS amount " \
        "FROM categories " \
        "LEFT JOIN transactions ON categories.id = transactions.category_id " \
        "GROUP BY categories.name " \
        "ORDER BY amount ASC;"
    );

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

    sqlite3_free(sql);

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

int remove_wallet(sqlite3 *db, Wallet *wallet) {
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

    if (rc != SQLITE_OK) {
        log_warn("%s", zErrMsg);
    }

    sqlite3_free(sql);
    sqlite3_free(zErrMsg);

    return rc;
}

int remove_category(sqlite3 *db, Category *category) {
    char *sql;
    char *zErrMsg = 0;
    int rc;

    sql = sqlite3_mprintf("BEGIN;" \
        "DELETE FROM categories WHERE " \
        "categories.id = %d OR " \
        "categories.name = '%q';" \
        "COMMIT;",
        category->id,
        category->name
    );
    
    rc = sqlite3_exec(db, sql, NULL, 0, &zErrMsg);

    if (rc != SQLITE_OK) {
        log_warn("%s", zErrMsg);
    }

    sqlite3_free(sql);
    sqlite3_free(zErrMsg);

    return rc;
}

int remove_transaction(sqlite3 *db, Transaction *transaction) {
    char *sql;
    char *zErrMsg = 0;
    int rc;

    sql = sqlite3_mprintf("DELETE FROM transactions WHERE " \
        "transactions.id = %d OR " \
        "transactions.name = '%q';",
        transaction->id,
        transaction->name
    );
    
    rc = sqlite3_exec(db, sql, NULL, 0, &zErrMsg);

    if (rc != SQLITE_OK) {
        log_warn("%s", zErrMsg);
    }

    sqlite3_free(sql);
    sqlite3_free(zErrMsg);

    return rc;
}

void clear_queue(Queue *origin) {
    Queue *temp;
    while (origin != NULL) {
        temp = origin;
        origin = origin->next;
        free(temp);
    }
}