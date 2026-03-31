//
// Created by Codex on 30.03.2026.
//

#ifndef RHYTHMGAME_BEATORAJAREPLAYIMPORTER_H
#define RHYTHMGAME_BEATORAJAREPLAYIMPORTER_H

#include <QObject>

namespace resource_managers {
class Profile;
}

namespace qml_components {

class BeatorajaReplayImporter : public QObject
{
    Q_OBJECT

  public:
    explicit BeatorajaReplayImporter(QObject* parent = nullptr);

    Q_INVOKABLE QVariantMap importFolder(resource_managers::Profile* profile,
                                         const QString& folderPath) const;
};

} // namespace qml_components

#endif // RHYTHMGAME_BEATORAJAREPLAYIMPORTER_H
