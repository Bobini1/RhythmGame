//
// Created by bobini on 20.09.23.
//

#ifndef RHYTHMGAME_SONGFOLDERFACTORY_H
#define RHYTHMGAME_SONGFOLDERFACTORY_H

#include <QObject>
#include "db/SqliteCppDb.h"

namespace qml_components {

class SongFolderFactory : public QObject
{
    Q_OBJECT

    db::SqliteCppDb* db;
    db::SqliteCppDb::Statement getCharts =
      db->createStatement("SELECT id, title, artist, subtitle, subartist, "
                          "genre, stage_file, banner, back_bmp, rank, total, "
                          "play_level, difficulty, is_random, normal_note_count, "
                          "ln_count, mine_count, length, initial_bpm, max_bpm, "
                          "min_bpm, path, directory, sha256, keymode "
                          "FROM charts WHERE directory IS (SELECT id FROM parent_dir WHERE dir IS ?) "
                          "ORDER BY title, subtitle ASC");
    db::SqliteCppDb::Statement getFolders =
      db->createStatement("SELECT dir FROM parent_dir "
                          "WHERE parent_dir IS ? ORDER BY dir ASC");
    // query to get count of charts and subfolders
    db::SqliteCppDb::Statement getSize =
      db->createStatement("SELECT COUNT(*) FROM parent_dir WHERE parent_dir = "
                          "(SELECT id FROM parent_dir WHERE dir IS ?) UNION SELECT COUNT(*) FROM charts WHERE "
                          "directory IS (SELECT id FROM parent_dir WHERE dir IS ?)");
    db::SqliteCppDb::Statement searchCharts = db->createStatement(
      "SELECT charts.id, charts.title, charts.artist, charts.subtitle, charts.subartist, "
      "charts.genre, charts.stage_file, charts.banner, charts.back_bmp, charts.rank, charts.total, "
      "charts.play_level, charts.difficulty, charts.is_random, charts.normal_note_count, "
      "charts.ln_count, charts.mine_count, charts.length, charts.initial_bpm, charts.max_bpm, "
      "charts.min_bpm, charts.path, charts.directory, charts.sha256, charts.keymode FROM charts_fts(?) "
      "JOIN charts ON charts_fts.rowid = charts.id ORDER BY charts.title, charts.subtitle ASC");
    db::SqliteCppDb::Statement getParentFolder =
      db->createStatement("SELECT parent_dir FROM parent_dir WHERE dir IS ?");

  public:
    explicit SongFolderFactory(db::SqliteCppDb* db, QObject* parent = nullptr);
    Q_INVOKABLE QVariantList open(QString path);
    Q_INVOKABLE int folderSize(QString path);
    Q_INVOKABLE QString parentFolder(QString path);
    Q_INVOKABLE QVariantList search(QString query);
};

} // namespace qml_components

#endif // RHYTHMGAME_SONGFOLDERFACTORY_H
