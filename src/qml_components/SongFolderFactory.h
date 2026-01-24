//
// Created by bobini on 20.09.23.
//

#ifndef RHYTHMGAME_SONGFOLDERFACTORY_H
#define RHYTHMGAME_SONGFOLDERFACTORY_H

#include "db/SqliteCppDb.h"

#include <QObject>

namespace qml_components {

class SongFolderFactory : public QObject
{
    Q_OBJECT

    db::SqliteCppDb* db;
    db::SqliteCppDb::Statement getCharts = db->createStatement(
      "SELECT c.id, c.title, c.artist, c.subtitle, c.subartist, "
      "c.genre, c.stage_file, c.banner, c.back_bmp, c.rank, c.total, "
      "c.play_level, c.difficulty, c.is_random, c.random_sequence, "
      "c.normal_note_count, "
      "c.ln_count, c.bss_count, c.mine_count, c.length, c.initial_bpm, "
      "c.max_bpm, c.min_bpm, "
      "c.main_bpm, c.avg_bpm, c.path, c.directory, c.sha256, c.md5, c.keymode, "
      "h.bpms, h.histogram_data "
      "FROM charts c "
      "LEFT JOIN histogram_data h ON h.chart_id = c.id "
      "WHERE c.directory IS (SELECT id FROM parent_dir WHERE dir IS ?) "
      "ORDER BY c.title, c.subtitle ASC");
    db::SqliteCppDb::Statement getChartsRecursive = db->createStatement(
      "SELECT c.id, c.title, c.artist, c.subtitle, c.subartist, "
      "c.genre, c.stage_file, c.banner, c.back_bmp, c.rank, c.total, "
      "c.play_level, c.difficulty, c.is_random, c.random_sequence, "
      "c.normal_note_count, "
      "c.ln_count, c.bss_count, c.mine_count, c.length, c.initial_bpm, "
      "c.max_bpm, c.min_bpm, "
      "c.main_bpm, c.avg_bpm, c.path, c.directory, c.sha256, c.md5, c.keymode, "
      "h.bpms, h.histogram_data "
      "FROM charts c "
      "LEFT JOIN histogram_data h ON h.chart_id = c.id "
      "WHERE c.directory IS (SELECT id FROM parent_dir WHERE dir LIKE ? || "
      "'%') "
      "ORDER BY c.title ASC");
    db::SqliteCppDb::Statement getMd5 =
      db->createStatement("SELECT md5 FROM charts WHERE directory IS "
                          "(SELECT id FROM parent_dir WHERE dir IS ?)");
    db::SqliteCppDb::Statement getMd5Recursive = db->createStatement(
      "SELECT md5 FROM charts WHERE chart_directory LIKE ? || '%'");
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
      "SELECT c.id, c.title, c.artist, c.subtitle, c.subartist, "
      "c.genre, c.stage_file, c.banner, c.back_bmp, c.rank, c.total, "
      "c.play_level, c.difficulty, c.is_random, c.random_sequence, "
      "c.normal_note_count, "
      "c.ln_count, c.bss_count, c.mine_count, c.length, c.initial_bpm, "
      "c.max_bpm, c.min_bpm, "
      "c.main_bpm, c.avg_bpm, c.path, c.directory, c.sha256, c.md5, c.keymode, "
      "h.bpms, h.histogram_data "
      "FROM charts_fts(?) "
      "JOIN charts c ON charts_fts.rowid = c.id "
      "LEFT JOIN histogram_data h ON h.chart_id = c.id "
      "ORDER BY c.title, c.subtitle ASC");
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
