#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>

sqlite3* db_connect();

void db_insert(sqlite3 *db,
               char *timestamp,
               double ecg,
               double ppg);

void db_export_session();

#endif