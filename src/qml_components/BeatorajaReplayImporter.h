//
// Created by Codex on 30.03.2026.
//

#ifndef RHYTHMGAME_BEATORAJAREPLAYIMPORTER_H
#define RHYTHMGAME_BEATORAJAREPLAYIMPORTER_H

#include "ReplayImportOperation.h"

class QObject;

namespace resource_managers {
class Profile;
}

namespace qml_components {

/**
 * @brief Start an asynchronous beatoraja replay import.
 * @details The returned operation is parented to @p parent and tracks
 * progress. The caller is responsible for exposing it to QML.
 */
ReplayImportOperation* startBeatorajaReplayImport(
  resource_managers::Profile* profile,
  const QString& folderPath);

} // namespace qml_components

#endif // RHYTHMGAME_BEATORAJAREPLAYIMPORTER_H
