//
// Created by bobini on 20.09.23.
//

#ifndef RHYTHMGAME_SONGFOLDERFACTORY_H
#define RHYTHMGAME_SONGFOLDERFACTORY_H
\
#include <QIfPendingReply>
#include "db/SqliteCppDb.h"

namespace qml_components {

class SongFolderFactory : public QObject
{
    Q_OBJECT

    db::SqliteCppDb* db;
    db::SqliteCppDb::Statement getCharts = db->createStatement(
      "SELECT id, title, artist, subtitle, subartist, "
      "genre, stage_file, banner, back_bmp, rank, total, "
      "play_level, difficulty, is_random, random_sequence, normal_note_count, "
      "ln_count, mine_count, length, initial_bpm, max_bpm, min_bpm, "
      "main_bpm, avg_bpm, path, directory, sha256, md5, keymode "
      "FROM charts WHERE directory IS (SELECT id FROM parent_dir WHERE dir IS "
      "?) ORDER BY title, subtitle ASC");
    db::SqliteCppDb::Statement getChartsRecursive =
      db->createStatement(
        "SELECT id, title, artist, subtitle, subartist, "
        "genre, stage_file, banner, back_bmp, rank, total, "
        "play_level, difficulty, is_random, random_sequence, normal_note_count, "
        "ln_count, mine_count, length, initial_bpm, max_bpm, min_bpm, "
        "main_bpm, avg_bpm, path, directory, sha256, md5, keymode FROM charts "
        "WHERE directory IS (SELECT id FROM parent_dir WHERE dir LIKE ? || '%') "
        "ORDER BY title ASC");
    db::SqliteCppDb::Statement getFolders =
      db->createStatement("SELECT dir FROM parent_dir "
                          "WHERE parent_dir IS ? ORDER BY dir ASC");
    db::SqliteCppDb::Statement getFoldersRecursive =
      db->createStatement("SELECT dir FROM parent_dir "
                          "WHERE parent_dir LIKE ? || '%' ORDER BY dir ASC");
    db::SqliteCppDb::Statement getSize = db->createStatement(
      "SELECT COUNT(*) FROM parent_dir WHERE parent_dir = "
      "(SELECT id FROM parent_dir WHERE dir IS ?) UNION SELECT COUNT(*) FROM "
      "charts WHERE "
      "directory IS (SELECT id FROM parent_dir WHERE dir IS ?)");
    db::SqliteCppDb::Statement searchCharts = db->createStatement(
      "SELECT charts.id, charts.title, charts.artist, charts.subtitle, "
      "charts.subartist, "
      "charts.genre, charts.stage_file, charts.banner, charts.back_bmp, "
      "charts.rank, charts.total, "
      "charts.play_level, charts.difficulty, charts.is_random, "
      "charts.random_sequence, charts.normal_note_count, "
      "charts.ln_count, charts.mine_count, charts.length, charts.initial_bpm, "
      "charts.max_bpm, charts.min_bpm, charts.main_bpm, charts.avg_bpm, "
      "charts.path, charts.directory, charts.sha256, "
      "charts.md5, charts.keymode FROM charts_fts(?) "
      "JOIN charts ON charts_fts.rowid = charts.id ORDER BY charts.title, "
      "charts.subtitle ASC");
    db::SqliteCppDb::Statement getParentFolder =
      db->createStatement("SELECT parent_dir FROM parent_dir WHERE dir IS ?");

  public:
    explicit SongFolderFactory(db::SqliteCppDb* db, QObject* parent = nullptr);
    Q_INVOKABLE QVariantList open(const QString& path);
    Q_INVOKABLE QVariantList openRecursive(const QString& path);
    Q_INVOKABLE int folderSize(const QString& path);
    Q_INVOKABLE QString parentFolder(const QString& path);
    Q_INVOKABLE QVariantList search(const QString& query);
};

} // namespace qml_components

#endif // RHYTHMGAME_SONGFOLDERFACTORY_H
