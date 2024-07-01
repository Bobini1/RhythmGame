//
// Created by PC on 08/10/2023.
//

#ifndef RHYTHMGAME_FILEQUERY_H
#define RHYTHMGAME_FILEQUERY_H

#include <QUrl>
#include <QFile>
namespace qml_components {
class FileQuery final : public QObject
{
    Q_OBJECT

  public:
    Q_INVOKABLE bool exists(const QString& path);
    /**
     * @brief Get a list of all files in a directory EXCLUDING ini files.
     * @param directory The directory to search.
     * @return A list of all files in the directory EXCLUDING ini files.
     */
    Q_INVOKABLE QList<QString> getSelectableFilesForDirectory(
      const QString& directory) const;
};
} // namespace qml_components

#endif // RHYTHMGAME_FILEQUERY_H
