//
// Created by bobini on 20.09.23.
//

#ifndef RHYTHMGAME_ROOTSONGFOLDER_H
#define RHYTHMGAME_ROOTSONGFOLDER_H

#include <QObject>
#include <QtQmlIntegration>
#include "Folder.h"
namespace qml_components {

class RootSongFolder : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    db::SqliteCppDb* db;
    static inline RootSongFolder* instance = nullptr;

  public:
    explicit RootSongFolder(db::SqliteCppDb* db, QObject* parent = nullptr);
    static auto create(QQmlEngine* engine, QJSEngine* scriptEngine)
      -> RootSongFolder*;
    static void setInstance(RootSongFolder* newInstance);
    Q_INVOKABLE Folder* get();
};

} // namespace qml_components

#endif // RHYTHMGAME_ROOTSONGFOLDER_H
