#ifndef SHELL_H
#define SHELL_H

#include "sqlite3/sqlite3.h"
#include "db.h"

#define SH_BUFFER_SIZE  512
#define SH_ARGV_SIZE    16

#define NUM_SH_CMD      7
#define NUM_SH_SUB_CMD  7

static char     *sh_read_line(void);
static char     **sh_read_args(char *, int *);
static void     clear_args(int, char **);

static int      create_wallet(Wallet *);
static int      create_category(Category *);
static int      create_transaction(Transaction *);

static int      show_wallets(Wallet *);
static int      show_categories(Category *);
static int      show_transactions(Transaction *);

static int      delete_wallet(Wallet *);
static int      delete_category(Category *);
static int      delete_transaction(Transaction *);

static int      export_transactions(int, char **);

static int      create_record(RECORD_TYPES, Record *);
static int      show_record(RECORD_TYPES, Record *);
static int      delete_record(RECORD_TYPES, Record *);

static void     parse_wallet(int, char **, Wallet *);
static void     parse_category(int, char **, Category *);
static void     parse_transaction(int, char **, Transaction *);

static int      wallet_cmd(int, char **);
static int      category_cmd(int, char **);
static int      transaction_cmd(int, char **);

static int      categories_report(int, char **);

static int      sh_help(int, char **);
static int      sh_exit(int, char **);
static int      sh_exec(int, char **);
static int      sh_sub_exec(int, char **);

void    sh_spawn(DB_Handler *);

#endif