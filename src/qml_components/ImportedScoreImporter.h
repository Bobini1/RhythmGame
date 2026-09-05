#ifndef RHYTHMGAME_IMPORTEDSCOREIMPORTER_H
#define RHYTHMGAME_IMPORTEDSCOREIMPORTER_H

#include "db/SqliteCppDb.h"

#include <QByteArray>
#include <QString>

#include <functional>

namespace qml_components {

struct ScoreImportCallbacks
{
    std::function<void(int)> started;
    std::function<void()> imported;
    std::function<void()> skipped;
    std::function<void(QString)> failed;
};

void
importLocalScoreDatabase(db::SqliteCppDb& targetDb,
                         const QString& filePath,
                         const ScoreImportCallbacks& callbacks);

[[nodiscard]] auto
importBokutachiPersonalBests(db::SqliteCppDb& targetDb,
                             const QByteArray& response) -> int;

} // namespace qml_components

#endif // RHYTHMGAME_IMPORTEDSCOREIMPORTER_H
