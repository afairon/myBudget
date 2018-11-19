#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "log.h"
#include "db.h"
#include "sqlite3.h"

DB_Handler *connect(char *name) {
    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;

    if (name == NULL || strcmp("", name) == 0) {
        name = DB_NAME;
    }

    DB_Handler *handler = (DB_Handler *)malloc(sizeof(DB_Handler));
    handler->db_name = name;

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
        "title VARCHAR(64) NOT NULL," \
        "description TEXT," \
        "amount REAL NOT NULL," \
        "wallet_id INTEGER NOT NULL," \
        "category_id INTEGER," \
        "created DEFAULT CURRENT_TIMESTAMP NOT NULL," \
        "FOREIGN KEY(wallet_id) REFERENCES wallets(id)," \
        "FOREIGN KEY(category_id) REFERENCES categories(id)" \
        ");" \

        "CREATE UNIQUE INDEX IF NOT EXISTS idx_wallet ON wallets(" \
        "id," \
        "name" \
        ");" \

        "CREATE UNIQUE INDEX IF NOT EXISTS idx_category ON categories(" \
        "id," \
        "name" \
        ");" \

        "CREATE UNIQUE INDEX IF NOT EXISTS idx_transaction ON transactions(" \
        "title," \
        "amount,"\
        "wallet_id," \
        "category_id" \
        ");" \

        "COMMIT;";
    
    rc = sqlite3_exec(db, sql, NULL, 0, &zErrMsg);

    return rc;
}

int add_wallet(sqlite3 *db, Wallet *wallet) {
    char *sql;
    char *zErrMsg = 0;
    int rc;

    sql = sqlite3_mprintf("INSERT INTO wallets(name)" \
        "VALUES('%q');",
        wallet->name);
    
    rc = sqlite3_exec(db, sql, NULL, 0, &zErrMsg);

    sqlite3_free(zErrMsg);

    return rc;
}

int add_category(sqlite3 *db, Category *category) {
    char *sql;
    char *zErrMsg = 0;
    int rc;

    sql = sqlite3_mprintf("INSERT INTO categories(name)" \
        "VALUES('%q');",
        category->name);
    
    rc = sqlite3_exec(db, sql, NULL, 0, &zErrMsg);

    sqlite3_free(zErrMsg);

    return rc;
}

int add_transaction(sqlite3 *db, Transaction *transaction) {
    char *sql;
    char *zErrMsg = 0;
    int rc;

    sql = sqlite3_mprintf("INSERT INTO transactions(" \
        "title," \
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
        transaction->title,
        transaction->description,
        transaction->amount,
        transaction->wallet_id,
        transaction->category_id
    );
    
    rc = sqlite3_exec(db, sql, NULL, 0, &zErrMsg);

    sqlite3_free(zErrMsg);

    return rc;
}