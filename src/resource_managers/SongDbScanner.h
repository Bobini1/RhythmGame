//
// Created by bobini on 14.09.23.
//

#ifndef RHYTHMGAME_SONGDBSCANNER_H
#define RHYTHMGAME_SONGDBSCANNER_H

#include "db/SqliteCppDb.h"
#include "ChartDataFactory.h"
namespace resource_managers {

class SongAssetStore;

class SongDbScanner
{
    db::SqliteCppDb* db;
    SongAssetStore* assetStore;

  public:
    SongDbScanner(db::SqliteCppDb* db, SongAssetStore* assetStore);
    void scanDirectory(
      const std::filesystem::path& directory,
      const std::function<void(QString)>& updateCurrentScannedFolder,
      std::atomic_bool* stop) const;
};

} // namespace resource_managers

#endif // RHYTHMGAME_SONGDBSCANNER_H
