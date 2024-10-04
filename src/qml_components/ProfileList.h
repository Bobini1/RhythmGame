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
    db::SqliteCppDb* songDb;
    input::GamepadManager* gamepadManager;
    QMap<QString, ThemeFamily> themeFamilies;
    QList<resource_managers::Profile*> activeProfiles;

    Q_PROPERTY(
      QList<resource_managers::Profile*> activeProfiles READ getActiveProfiles
        WRITE setActiveProfiles NOTIFY activeProfilesChanged)

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
    auto getProfiles() -> const QList<resource_managers::Profile*>&;

    /**
     * @brief The list of active profiles.
     * The first profile is the primary profile, used to select the theme and
     * theme settings. If other profiles are active, battle mode is enabled.
     * There is always at least one profile in the list.
     * @return The list of profiles that are currently active.
     */
    auto getActiveProfiles() -> QList<resource_managers::Profile*>;
    /**
     * @brief Set the list of active profiles.
     * The first profile is the primary profile, used to select the theme and
     * theme settings. Provide more than one profile to enable battle mode.
     * @param profiles The list of profiles that are to be set as active.
     */
    void setActiveProfiles(QList<resource_managers::Profile*> profiles);

    Q_INVOKABLE resource_managers::Profile* at(int index) const;
    Q_INVOKABLE resource_managers::Profile* createProfile();
    Q_INVOKABLE void removeProfile(resource_managers::Profile* profile);

  signals:
    void activeProfilesChanged();
};
} // namespace qml_components

#endif // RHYTHMGAME_PROFILELIST_H
