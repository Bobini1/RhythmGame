//
// Created by bobini on 18.09.23.
//

#include "DefineDb.h"

#include "support/Version.h"
namespace resource_managers {
void
defineDb(db::SqliteCppDb& db)
{
    auto transaction = db.transaction();
    db.execute("CREATE TABLE IF NOT EXISTS properties ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "key TEXT NOT NULL UNIQUE,"
               "value"
               ");");

    auto version = std::optional<std::tuple<int, int, int>>{};
    {
        auto versionStmt = db.createStatement(
          "SELECT value FROM properties WHERE key = 'version';");
        version = versionStmt.executeAndGet<int64_t>().transform(
          [](int64_t v) { return support::unpackVersion(v); });
    }
    if (version && *version < std::tuple{ 1, 3, 0 }) {
        db.execute("DROP TABLE IF EXISTS note_data;");
        db.execute("DROP TABLE IF EXISTS histogram_data;");
        db.execute("DROP TABLE IF EXISTS charts_fts;");
        db.execute("DROP TABLE IF EXISTS charts;");
        db.execute("DROP TABLE IF EXISTS parent_dir;");
        db.execute("DROP TABLE IF EXISTS histogram_data;");
    }
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
               "rank REAL NOT NULL,"
               "total REAL NOT NULL,"
               "play_level INTEGER NOT NULL,"
               "difficulty INTEGER NOT NULL,"
               "is_random INTEGER NOT NULL,"
               "random_sequence STRING NOT NULL,"
               "normal_note_count INTEGER NOT NULL,"
               "scratch_count INTEGER NOT NULL,"
               "ln_count INTEGER NOT NULL,"
               "bss_count INTEGER NOT NULL,"
               "mine_count INTEGER NOT NULL,"
               "length INTEGER NOT NULL,"
               "initial_bpm REAL NOT NULL,"
               "max_bpm REAL NOT NULL,"
               "min_bpm REAL NOT NULL,"
               "main_bpm REAL NOT NULL,"
               "avg_bpm REAL NOT NULL,"
               "peak_density REAL NOT NULL,"
               "avg_density REAL NOT NULL,"
               "end_density REAL NOT NULL,"
               "path TEXT NOT NULL UNIQUE,"
               "chart_directory TEXT,"
               "directory INTEGER,"
               "sha256 TEXT NOT NULL,"
               "md5 TEXT NOT NULL,"
               "keymode INTEGER NOT NULL,"
               "game_version INTEGER NOT NULL"
               ");");

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

    db.execute("CREATE TABLE IF NOT EXISTS note_data ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "sha256 TEXT NOT NULL UNIQUE,"
               "note_data BLOB NOT NULL"
               ");");

    db.execute("DELETE FROM note_data;");

    db.execute("CREATE TABLE IF NOT EXISTS parent_dir ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "parent_dir INTEGER,"
               "dir TEXT NOT NULL UNIQUE"
               ");");

    // Additional folder memberships leave each chart's asset location intact.
    db.execute("CREATE TABLE IF NOT EXISTS folder_charts ("
               "directory INTEGER NOT NULL REFERENCES parent_dir(id) "
               "ON DELETE CASCADE,"
               "chart_id INTEGER NOT NULL REFERENCES charts(id) "
               "ON DELETE CASCADE,"
               "PRIMARY KEY (directory, chart_id))");
    db.execute("CREATE INDEX IF NOT EXISTS folder_charts_chart_index "
               "ON folder_charts(chart_id)");

    db.execute(
      "CREATE TABLE IF NOT EXISTS root_dir ("
      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
      "path TEXT NOT NULL UNIQUE,"
      "status INTEGER DEFAULT 0 NOT NULL" // 0 = not scanned, 1 = scanned
      ");");

    db.execute("CREATE TABLE IF NOT EXISTS preview_files ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "path TEXT NOT NULL,"
               "directory TEXT NOT NULL UNIQUE"
               ");");

    db.execute("CREATE TABLE IF NOT EXISTS readme_files ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "path TEXT NOT NULL,"
               "directory TEXT NOT NULL UNIQUE"
               ");");

    db.execute("CREATE TABLE IF NOT EXISTS histogram_data ("
               "id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "chart_id INTEGER NOT NULL UNIQUE,"
               "bpms BLOB NOT NULL,"
               "histogram_data BLOB NOT NULL"
               ");");

#ifndef RHYTHMGAME_USE_BACKBEAT
    // This database can also be used by a build with Backbeat disabled.
    // Remove only its cached catalog; profile scores live in separate
    // databases.
    db.execute("DELETE FROM charts WHERE path GLOB 'backbeat:/*'");
    db.execute("DELETE FROM parent_dir WHERE dir GLOB 'backbeat:/*'");
    db.execute("DELETE FROM preview_files WHERE path GLOB 'backbeat:/*'");
    db.execute("DELETE FROM readme_files WHERE path GLOB 'backbeat:/*'");
    db.execute("DELETE FROM histogram_data WHERE chart_id NOT IN (SELECT id "
               "FROM charts)");
#endif

    if (!version || *version < std::tuple{ 1, 3, 6 }) {
        db.execute("UPDATE charts SET rank = CASE rank "
                   "WHEN 0 THEN 25 "
                   "WHEN 1 THEN 50 "
                   "WHEN 2 THEN 75 "
                   "WHEN 3 THEN 100 "
                   "ELSE 75 END WHERE path NOT LIKE '%.bmson';");
        // changing the type to REAL
        db.execute("ALTER TABLE charts RENAME COLUMN rank TO rank_old");
        db.execute(
          "ALTER TABLE charts ADD COLUMN rank REAL NOT NULL DEFAULT 75");
        db.execute("UPDATE charts SET rank = rank_old");
        db.execute("ALTER TABLE charts DROP COLUMN rank_old");
    }

    {
        auto stmt = db.createStatement(
          "INSERT OR REPLACE INTO properties (key, value) VALUES "
          "('version', ?);");
        stmt.bind(1, static_cast<int64_t>(support::currentVersion));
        stmt.execute();
    }

    transaction.commit();
    db.execute("PRAGMA optimize;");
}
} // namespace resource_managers
