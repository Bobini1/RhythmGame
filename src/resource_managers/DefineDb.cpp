//
// Created by bobini on 18.09.23.
//

#include "DefineDb.h"

namespace {
auto
tableMissingForeignKey(db::SqliteCppDb& db,
                       const std::string& tableName,
                       std::string_view foreignKeyNeedle) -> bool
{
    const auto query =
      "SELECT sql FROM sqlite_master WHERE type = 'table' AND name = '" +
      tableName + "';";
    auto stmt = db.createStatement(query);
    auto sql = stmt.executeAndGet<std::string>();
    if (!sql) {
        return false;
    }
    return !sql->contains(foreignKeyNeedle);
}

void
migrateChartsDirectoryForeignKey(db::SqliteCppDb& db)
{
    if (!tableMissingForeignKey(db, "charts", "FOREIGN KEY (directory)")) {
        return;
    }

    db.execute("BEGIN IMMEDIATE;");
    try {
        db.execute("ALTER TABLE charts RENAME TO charts_old;");
        db.execute(
          "CREATE TABLE charts ("
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
          "random_sequence STRING NOT NULL,"
          "normal_note_count INTEGER NOT NULL,"
          "ln_count INTEGER NOT NULL,"
          "bss_count INTEGER NOT NULL,"
          "mine_count INTEGER NOT NULL,"
          "length INTEGER NOT NULL,"
          "initial_bpm REAL NOT NULL,"
          "max_bpm REAL NOT NULL,"
          "min_bpm REAL NOT NULL,"
          "main_bpm REAL NOT NULL,"
          "avg_bpm REAL NOT NULL,"
          "path TEXT NOT NULL UNIQUE,"
          "chart_directory TEXT,"
          "directory INTEGER,"
          "sha256 TEXT NOT NULL,"
          "md5 TEXT NOT NULL,"
          "keymode INTEGER NOT NULL,"
          "FOREIGN KEY (directory) REFERENCES parent_dir(id) ON DELETE CASCADE"
          ");");
        db.execute(
          "INSERT INTO charts ("
          "id, title, artist, subtitle, subartist, genre, stage_file, "
          "banner, back_bmp, rank, total, play_level, difficulty, is_random, "
          "random_sequence, normal_note_count, ln_count, bss_count, "
          "mine_count, "
          "length, initial_bpm, max_bpm, min_bpm, main_bpm, avg_bpm, path, "
          "chart_directory, directory, sha256, md5, keymode) "
          "SELECT "
          "id, title, artist, subtitle, subartist, genre, stage_file, "
          "banner, back_bmp, rank, total, play_level, difficulty, is_random, "
          "random_sequence, normal_note_count, ln_count, 0, "
          "mine_count, "
          "length, initial_bpm, max_bpm, min_bpm, main_bpm, avg_bpm, path, "
          "chart_directory, directory, sha256, md5, keymode "
          "FROM charts_old;");
        db.execute("DROP TABLE charts_old;");
        db.execute("COMMIT;");
    } catch (...) {
        db.execute("ROLLBACK;");
        throw;
    }
}
} // namespace

namespace resource_managers {
void
defineDb(db::SqliteCppDb& db)
{
    db.execute(
      "CREATE TABLE IF NOT EXISTS charts ("
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
      "random_sequence STRING NOT NULL,"
      "normal_note_count INTEGER NOT NULL,"
      "ln_count INTEGER NOT NULL,"
      "bss_count INTEGER NOT NULL,"
      "mine_count INTEGER NOT NULL,"
      "length INTEGER NOT NULL,"
      "initial_bpm REAL NOT NULL,"
      "max_bpm REAL NOT NULL,"
      "min_bpm REAL NOT NULL,"
      "main_bpm REAL NOT NULL,"
      "avg_bpm REAL NOT NULL,"
      "path TEXT NOT NULL UNIQUE,"
      "chart_directory TEXT,"
      "directory INTEGER,"
      "sha256 TEXT NOT NULL,"
      "md5 TEXT NOT NULL,"
      "keymode INTEGER NOT NULL,"
      "FOREIGN KEY (directory) REFERENCES parent_dir(id) ON DELETE CASCADE"
      ");");
    migrateChartsDirectoryForeignKey(db);
    db.execute(
      "CREATE INDEX IF NOT EXISTS directory_index ON charts(directory)");
    db.execute("CREATE INDEX IF NOT EXISTS chart_directory_index ON "
               "charts(chart_directory)");
    db.execute("CREATE INDEX IF NOT EXISTS sha256_index ON charts(sha256)");
    db.execute("CREATE INDEX IF NOT EXISTS md5_index ON charts(md5)");

    db.execute("CREATE VIRTUAL TABLE IF NOT EXISTS charts_fts USING "
               "fts5(title, artist, subtitle, subartist, genre, path, "
               "content='charts', content_rowid='id')");
    db.execute(
      "CREATE TRIGGER IF NOT EXISTS tbl_ai AFTER INSERT ON charts BEGIN "
      " INSERT INTO charts_fts(rowid, title, artist, subtitle, subartist, "
      "genre, path) VALUES (new.id, new.title, new.artist, new.subtitle, "
      "new.subartist, new.genre, new.path); "
      "END;");
    db.execute(
      "CREATE TRIGGER IF NOT EXISTS tbl_ad AFTER DELETE ON charts BEGIN "
      " INSERT INTO charts_fts(charts_fts, rowid, title, artist, subtitle, "
      "subartist, genre, path) VALUES('delete', old.id, old.title, old.artist, "
      "old.subtitle, old.subartist, old.genre, old.path); "
      "END;");
    db.execute(
      "CREATE TRIGGER IF NOT EXISTS tbl_au AFTER UPDATE ON charts BEGIN "
      " INSERT INTO charts_fts(charts_fts, rowid, title, artist, subtitle, "
      "subartist, genre, path) VALUES('delete', old.id, old.title, old.artist, "
      "old.subtitle, old.subartist, old.genre, old.path); "
      " INSERT INTO charts_fts(rowid, title, artist, subtitle, subartist, "
      "genre, path) VALUES (new.id, new.title, new.artist, new.subtitle, "
      "new.subartist, new.genre, new.path); "
      "END;");

    // unused atm (too big)
    db.execute("CREATE TABLE IF NOT EXISTS note_data ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "sha256 TEXT NOT NULL UNIQUE,"
               "note_data BLOB NOT NULL"
               ");");

    db.execute("DELETE FROM note_data;");

    db.execute("CREATE TABLE IF NOT EXISTS histogram_data ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "chart_id INTEGER NOT NULL UNIQUE,"
               "bpms BLOB NOT NULL,"
               "histogram_data BLOB NOT NULL,"
               "FOREIGN KEY (chart_id) REFERENCES charts(id) ON DELETE CASCADE"
               ");");

    db.execute("CREATE TABLE IF NOT EXISTS parent_dir ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "parent_dir INTEGER,"
               "dir TEXT NOT NULL UNIQUE"
               ");");

    db.execute(
      "CREATE TABLE IF NOT EXISTS root_dir ("
      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
      "path TEXT NOT NULL UNIQUE,"
      "status INTEGER DEFAULT 0 NOT NULL" // 0 = not scanned, 1 = scanned
      ");");

    db.execute("CREATE TABLE IF NOT EXISTS properties ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "key TEXT NOT NULL UNIQUE,"
               "value"
               ");");

    db.execute("CREATE TABLE IF NOT EXISTS preview_files ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "path TEXT NOT NULL,"
               "directory TEXT NOT NULL UNIQUE"
               ");");

    db.execute("PRAGMA optimize;");
    db.execute("VACUUM");
}
} // namespace resource_managers
