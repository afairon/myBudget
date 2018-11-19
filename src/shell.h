#ifndef SHELL_H
#define SHELL_H

#include "sqlite3.h"
#include "db.h"

#define SH_BUFFER_SIZE  512
#define SH_ARGV_SIZE    16

#define NUM_SH_CMD      7

static char     *sh_read_line(void);
static char     **sh_read_args(char *, int *);
static int      clear_args(int, char **);

static int      create_wallet(sqlite3 *, int, char **);
static int      create_category(sqlite3 *, int, char **);
static int      create_transaction(sqlite3 *, int, char **);

static int      delete_wallet(sqlite3 *, int, char **);
static int      delete_category(sqlite3 *, int, char **);
static int      delete_transaction(sqlite3 *, int, char **);

static int      sh_exit(sqlite3 *, int, char **);
int     sh_exec(sqlite3 *, int, char **);
void    sh_spawn(DB_Handler *);

#endif