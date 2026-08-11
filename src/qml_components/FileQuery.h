//
// Created by PC on 08/10/2023.
//

#ifndef RHYTHMGAME_FILEQUERY_H
#define RHYTHMGAME_FILEQUERY_H

#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>
namespace resource_managers {
class SongAssetStore;
}
namespace qml_components {
class FileQuery : public QObject
{
    Q_OBJECT

    resource_managers::SongAssetStore* assetStore;

  public:
    explicit FileQuery(resource_managers::SongAssetStore* assetStore = nullptr,
                       QObject* parent = nullptr);
    Q_INVOKABLE QString localFileUrl(const QString& path) const;
    Q_INVOKABLE QString readTextFile(const QString& path) const;
    /**
     * @brief Get a list of all files in a directory EXCLUDING ini files.
     * @param directory The directory to search.
     * @return A list of all files in the directory EXCLUDING ini files.
     */
    Q_INVOKABLE QList<QString> getSelectableFilesForDirectory(
      const QString& directory) const;
    Q_INVOKABLE QList<QString> getSelectableFilesForDirectory(
      const QString& directory,
      const QStringList& filters) const;
    Q_INVOKABLE QList<QString> getSelectableFontFilesForDirectory(
      const QString& directory,
      bool fixedPitchOnly,
      bool tabularDigitsOnly) const;
    Q_INVOKABLE QList<QString> getSystemFontFamilies(
      bool fixedPitchOnly,
      bool tabularDigitsOnly) const;
    Q_INVOKABLE QString
    getDefaultSystemFontFamily(bool fixedPitchOnly,
                               bool tabularDigitsOnly) const;
};
} // namespace qml_components

#endif // RHYTHMGAME_FILEQUERY_H
