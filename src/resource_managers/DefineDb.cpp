//
// Created by bobini on 18.09.23.
//

#include "DefineDb.h"
namespace resource_managers {
void
defineDb(db::SqliteCppDb& db)
{
    db.execute("CREATE TABLE IF NOT EXISTS charts ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "title TEXT NOT NULL,"
               "artist TEXT NOT NULL,"
               "subtitle TEXT NOT NULL,"
               "subartist TEXT NOT NULL,"
               "genre TEXT NOT NULL,"
               "stage_file TEXT NOT NULL,"
               "banner TEXT NOT NULL,"
               "back_bmp TEXT NOT NULL,"
               "rank INTEGER NOT NULL,"
               "total REAL NOT NULL,"
               "play_level INTEGER NOT NULL,"
               "difficulty INTEGER NOT NULL,"
               "is_random INTEGER NOT NULL,"
               "normal_note_count INTEGER NOT NULL,"
               "ln_count INTEGER NOT NULL,"
               "mine_count INTEGER NOT NULL,"
               "length INTEGER NOT NULL,"
               "initial_bpm REAL NOT NULL,"
               "max_bpm REAL NOT NULL,"
               "min_bpm REAL NOT NULL,"
               "path TEXT NOT NULL UNIQUE,"
               "chart_directory TEXT,"
               "directory INTEGER,"
               "sha256 TEXT NOT NULL,"
               "keymode INTEGER NOT NULL"
               ");");

    db.execute("CREATE INDEX IF NOT EXISTS directory_index ON charts(directory)");
    db.execute("CREATE INDEX IF NOT EXISTS chart_directory_index ON charts(chart_directory)");
    db.execute("CREATE INDEX IF NOT EXISTS sha256_index ON charts(sha256)");

    db.execute("CREATE VIRTUAL TABLE IF NOT EXISTS charts_fts USING fts5(title, artist, subtitle, subartist, genre, path, content='charts', content_rowid='id')");
    db.execute("CREATE TRIGGER IF NOT EXISTS tbl_ai AFTER INSERT ON charts BEGIN "
               " INSERT INTO charts_fts(rowid, title, artist, subtitle, subartist, genre, path) VALUES (new.id, new.title, new.artist, new.subtitle, new.subartist, new.genre, new.path); "
               "END;");
    db.execute("CREATE TRIGGER IF NOT EXISTS tbl_ad AFTER DELETE ON charts BEGIN "
               " INSERT INTO charts_fts(charts_fts, rowid, title, artist, subtitle, subartist, genre, path) VALUES('delete', old.id, old.title, old.artist, old.subtitle, old.subartist, old.genre, old.path); "
               "END;");
    db.execute("CREATE TRIGGER IF NOT EXISTS tbl_au AFTER UPDATE ON charts BEGIN "
               " INSERT INTO charts_fts(charts_fts, rowid, title, artist, subtitle, subartist, genre, path) VALUES('delete', old.id, old.title, old.artist, old.subtitle, old.subartist, old.genre, old.path); "
               " INSERT INTO charts_fts(rowid, title, artist, subtitle, subartist, genre, path) VALUES (new.id, new.title, new.artist, new.subtitle, new.subartist, new.genre, new.path); "
               "END;");

    db.execute("CREATE TABLE IF NOT EXISTS note_data ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "sha256 TEXT NOT NULL UNIQUE,"
               "note_data BLOB NOT NULL"
               ");");

    db.execute("CREATE TABLE IF NOT EXISTS parent_dir ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "parent_dir INTEGER,"
               "dir TEXT NOT NULL UNIQUE"
               ");");

    db.execute("CREATE TABLE IF NOT EXISTS root_dir ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "path TEXT NOT NULL UNIQUE,"
               "status INTEGER DEFAULT 0 NOT NULL" // 0 = not scanned, 1 = scanned
               ");");

    db.execute("CREATE TABLE IF NOT EXISTS current_profile ("
               "id INTEGER PRIMARY KEY CHECK (id = 1),"
               "path TEXT NOT NULL UNIQUE"
               ");");

    db.execute("CREATE TABLE IF NOT EXISTS preview_files ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "path TEXT NOT NULL,"
               "directory INTEGER UNIQUE"
               ");");

    db.execute("PRAGMA optimize;");
}
} // namespace resource_managers