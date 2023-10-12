//
// Created by bobini on 30.09.23.
//

#ifndef RHYTHMGAME_PROFILELIST_H
#define RHYTHMGAME_PROFILELIST_H

#include <QAbstractListModel>
#include "resource_managers/Profile.h"
namespace qml_components {
class ProfileList : public QAbstractListModel
{
    Q_OBJECT

    std::filesystem::path profilesFolder;
    QList<resource_managers::Profile*> profiles;
    resource_managers::Profile* currentProfile = nullptr;
    db::SqliteCppDb* songDb;

    Q_PROPERTY(resource_managers::Profile* currentProfile READ getCurrentProfile
                 NOTIFY currentProfileChanged)

  public:
    enum ProfileRoles
    {
        ProfileRole = Qt::UserRole + 1,
        ActiveRole,
    };

    explicit ProfileList(db::SqliteCppDb* songDb,
                         std::filesystem::path profilesFolder,
                         QObject* parent = nullptr);

    auto rowCount(const QModelIndex& parent = QModelIndex()) const
      -> int override;
    auto data(const QModelIndex& index, int role = Qt::DisplayRole) const
      -> QVariant override;
    auto roleNames() const -> QHash<int, QByteArray> override;
    resource_managers::Profile* getCurrentProfile() const;
    void setCurrentProfile(resource_managers::Profile* profile);

    Q_INVOKABLE resource_managers::Profile* at(int index) const;
    Q_INVOKABLE resource_managers::Profile* createProfile();
    Q_INVOKABLE void removeProfile(resource_managers::Profile* profile);

  signals:
    void currentProfileChanged(resource_managers::Profile* profile);
};
} // namespace qml_components

#endif // RHYTHMGAME_PROFILELIST_H
