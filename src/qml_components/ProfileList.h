//
// Created by bobini on 30.09.23.
//

#ifndef RHYTHMGAME_PROFILELIST_H
#define RHYTHMGAME_PROFILELIST_H

#include <QAbstractListModel>
#include "resource_managers/Profile.h"

#include <qproperty.h>
namespace qml_components {
class ThemeFamily;
class ProfileList final : public QAbstractListModel
{
    Q_OBJECT

    std::filesystem::path profilesFolder;
    QList<resource_managers::Profile*> profiles;
    resource_managers::Profile* currentProfile;
    db::SqliteCppDb* songDb;
    input::GamepadManager* gamepadManager;
    QMap<QString, ThemeFamily> themeFamilies;

    Q_PROPERTY(resource_managers::Profile* currentProfile READ getCurrentProfile
                 WRITE setCurrentProfile NOTIFY currentProfileChanged)

    auto createInputTranslator() -> input::InputTranslator*;

  public:
    enum ProfileRoles
    {
        ProfileRole = Qt::UserRole + 1,
        ActiveRole,
    };

    explicit ProfileList(db::SqliteCppDb* songDb,
                         const QMap<QString, ThemeFamily>& themeFamilies,
                         std::filesystem::path profilesFolder,
                         input::GamepadManager* gamepadManager,
                         QObject* parent = nullptr);

    auto rowCount(const QModelIndex& parent = QModelIndex()) const
      -> int override;
    auto data(const QModelIndex& index, int role = Qt::DisplayRole) const
      -> QVariant override;
    auto roleNames() const -> QHash<int, QByteArray> override;
    void setCurrentProfile(resource_managers::Profile* profile);
    auto getCurrentProfile() const -> resource_managers::Profile*;
    auto getProfiles() -> const QList<resource_managers::Profile*>&;

    Q_INVOKABLE resource_managers::Profile* at(int index) const;
    Q_INVOKABLE resource_managers::Profile* createProfile();
    Q_INVOKABLE void removeProfile(resource_managers::Profile* profile);

  signals:
    void currentProfileChanged();
};
} // namespace qml_components

#endif // RHYTHMGAME_PROFILELIST_H
