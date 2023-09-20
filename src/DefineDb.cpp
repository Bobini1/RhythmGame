//
// Created by bobini on 18.09.23.
//

#include "DefineDb.h"
void
defineDb(db::SqliteCppDb& db)
{
    // create charts table if it doesn't exist
    db.execute("CREATE TABLE IF NOT EXISTS charts ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "title TEXT NOT NULL,"
               "artist TEXT NOT NULL,"
               "subtitle TEXT NOT NULL,"
               "subartist TEXT NOT NULL,"
               "genre TEXT NOT NULL,"
               "rank INTEGER NOT NULL,"
               "total REAL NOT NULL,"
               "play_level INTEGER NOT NULL,"
               "difficulty INTEGER NOT NULL,"
               "is_random INTEGER NOT NULL,"
               "note_count INTEGER NOT NULL,"
               "length INTEGER NOT NULL,"
               "path TEXT NOT NULL UNIQUE,"
               "directory_in_db TEXT NOT NULL,"
               "sha256 TEXT NOT NULL,"
               "note_data BLOB NOT NULL"
               ");");

    // create a parent_dir table if it doesn't exist
    db.execute("CREATE TABLE IF NOT EXISTS parent_dir ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "parent_dir TEXT NOT NULL,"
               "path TEXT NOT NULL UNIQUE"
               ");");

    // create a root_dir table if it doesn't exist
    db.execute("CREATE TABLE IF NOT EXISTS root_dir ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "path TEXT NOT NULL UNIQUE"
               ");");

    // pending root dirs
    db.execute("CREATE TABLE IF NOT EXISTS pending_root_dir ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "path TEXT NOT NULL UNIQUE"
               ");");
}
