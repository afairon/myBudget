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

int init_db(DB_Handler *handler) {
    char *sql;
    char *zErrMsg = 0;
    int rc;

    sql = "BEGIN;" \

        "CREATE TABLE IF NOT EXISTS wallets(" \
        "id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL," \
        "name VARCHAR(32) UNIQUE NOT NULL," \
        "created DEFAULT CURRENT_TIMESTAMP NOT NULL" \
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
        "category_id INTEGER NOT NULL," \
        "created DEFAULT CURRENT_TIMESTAMP NOT NULL" \
        ");" \

        "CREATE UNIQUE INDEX IF NOT EXISTS idx_wallet ON wallets(" \
        "id," \
        "name" \
        ");" \

        "CREATE UNIQUE INDEX IF NOT EXISTS idx_category ON categories(" \
        "id," \
        "name" \
        ");" \

        "COMMIT;";
    
    rc = sqlite3_exec(handler->db, sql, NULL, 0, &zErrMsg);

    if (rc != SQLITE_OK) {
        handler->zErrMsg = zErrMsg;
        return 1;
    }

    return 0;
}

int add_wallet(DB_Handler *handler, Wallet *wallet) {
    char *sql, *sql_ft;
    char *zErrMsg = 0;
    int rc;

    sql_ft = (char *) malloc(85);

    sql = "BEGIN;" \

        "INSERT INTO wallets(name)" \
        "VALUES(" \
        "'%s'" \
        ");" \

        "COMMIT;";
    
    sprintf(sql_ft, sql, wallet->name);
    
    rc = sqlite3_exec(handler->db, sql_ft, NULL, 0, &zErrMsg);

    free(sql_ft);

    if (rc != SQLITE_OK) {
        handler->zErrMsg = zErrMsg;
        return 1;
    }

    return 0;
}

int add_category(DB_Handler *handler, Category *category) {
    char *sql, *sql_ft;
    char *zErrMsg = 0;
    int rc;

    sql_ft = (char *) malloc(85);

    sql = "BEGIN;" \

        "INSERT INTO categories(name)" \
        "VALUES(" \
        "'%s'" \
        ");" \

        "COMMIT;";
    
    sprintf(sql_ft, sql, category->name);
    
    rc = sqlite3_exec(handler->db, sql_ft, NULL, 0, &zErrMsg);

    free(sql_ft);

    if (rc != SQLITE_OK) {
        handler->zErrMsg = zErrMsg;
        return 1;
    }

    return 0;
}

int add_transaction(DB_Handler *handler, Transaction *transaction) {
    char *sql, *sql_ft;
    char *zErrMsg = 0;
    int rc;

    sql_ft = (char *) malloc(256);

    sql = "BEGIN;" \

        "INSERT INTO transactions(" \
        "title," \
        "description," \
        "amount," \
        "wallet_id," \
        "category_id)" \
        "VALUES(" \
        "'%s'," \
        "'%s'," \
        "%lf," \
        "%d," \
        "%d" \
        ");" \

        "COMMIT;";
    
    sprintf(
        sql_ft,
        sql,
        transaction->title,
        transaction->description,
        transaction->amount,
        transaction->wallet_id,
        transaction->category_id
    );
    
    rc = sqlite3_exec(handler->db, sql_ft, NULL, 0, &zErrMsg);

    free(sql_ft);

    if (rc != SQLITE_OK) {
        handler->zErrMsg = zErrMsg;
        return 1;
    }

    return 0;
}