//
// Created by Codex on 30.03.2026.
//

#ifndef RHYTHMGAME_BEATORAJAREPLAYIMPORTER_H
#define RHYTHMGAME_BEATORAJAREPLAYIMPORTER_H

#include "ReplayImportOperation.h"

class QObject;

namespace resource_managers {
class Profile;
class SongAssetStore;
}

namespace qml_components {

/**
 * @brief Imports beatoraja/LR2 replays on a worker thread.
 * @details Progress is queued to the existing main-thread operation. The caller
 * handles completion and exceptions after this function returns.
 */
void
startBeatorajaReplayImport(resource_managers::Profile* profile,
                           resource_managers::SongAssetStore* songAssetStore,
                           const QString& folderPath,
                           ReplayImportOperation* operation);

} // namespace qml_components

#endif // RHYTHMGAME_BEATORAJAREPLAYIMPORTER_H
