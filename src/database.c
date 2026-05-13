#include <stdio.h>
#include <sqlite3.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>

#include "database.h"

sqlite3* db_connect()
{
    sqlite3 *db;

    int rc =
        sqlite3_open("current_session.db", &db);

    if(rc)
    {
        printf("Cannot open database\n");

        return NULL;
    }

    char *err_msg = NULL;

    const char *sql =
        "CREATE TABLE IF NOT EXISTS sensor_data("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "timestamp TEXT NOT NULL,"
        "ecg REAL NOT NULL,"
        "ppg REAL NOT NULL"
        ");";

    sqlite3_exec(db, sql, 0, 0, &err_msg);

    return db;
}

void db_insert(sqlite3 *db,
               char *timestamp,
               double ecg,
               double ppg)
{
    sqlite3_stmt *stmt;

    const char *sql =
        "INSERT INTO sensor_data "
        "(timestamp, ecg, ppg) "
        "VALUES (?, ?, ?)";

    if(sqlite3_prepare_v2(db,
                          sql,
                          -1,
                          &stmt,
                          NULL) != SQLITE_OK)
    {
        printf("Prepare failed: %s\n",
               sqlite3_errmsg(db));

        return;
    }

    sqlite3_bind_text(stmt,
                      1,
                      timestamp,
                      -1,
                      SQLITE_STATIC);

    sqlite3_bind_double(stmt,
                        2,
                        ecg);

    sqlite3_bind_double(stmt,
                        3,
                        ppg);

    if(sqlite3_step(stmt) != SQLITE_DONE)
    {
        printf("Insert failed: %s\n",
               sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);
}

void db_export_session()
{
    mkdir("sessions", 0777);

    time_t now = time(NULL);

    struct tm *t = localtime(&now);

    char filename[256];

    strftime(filename,
             sizeof(filename),
             "sessions/session_%Y%m%d_%H%M%S.db",
             t);

    FILE *src = fopen("current_session.db", "rb");

    if(src == NULL)
    {
        perror("open source db");
        return;
    }

    FILE *dst = fopen(filename, "wb");

    if(dst == NULL)
    {
        perror("open export db");
        fclose(src);
        return;
    }

    char buffer[4096];

    size_t n;

    while((n = fread(buffer,
                      1,
                      sizeof(buffer),
                      src)) > 0)
    {
        fwrite(buffer,
               1,
               n,
               dst);
    }

    fclose(src);

    fclose(dst);

    printf("Database exported: %s\n",
           filename);
}