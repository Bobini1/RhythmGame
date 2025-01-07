//
// Created by bobini on 30.09.23.
//

#ifndef RHYTHMGAME_PROFILELIST_H
#define RHYTHMGAME_PROFILELIST_H

#include <QAbstractListModel>
#include "resource_managers/Profile.h"

#include <qproperty.h>
#include <qqmllist.h>
namespace resource_managers {
class InputTranslators;
}
namespace qml_components {
class ThemeFamily;

/**
 * @brief The list of local profiles.
 * Besides listing all local profiles, this class also contains the list of
 * active profiles (for battle mode) and the main profile, used to select themes
 * and theme settings.
 */
class ProfileList final : public QAbstractListModel
{
    Q_OBJECT

    std::filesystem::path profilesFolder;
    QList<resource_managers::Profile*> profiles;
    db::SqliteCppDb* songDb;
    resource_managers::InputTranslators* keyConfigs;
    QMap<QString, ThemeFamily> themeFamilies;
    QList<resource_managers::Profile*> activeProfiles;
    resource_managers::Profile* mainProfile{};
    QList<resource_managers::Profile*> previousSessionProfiles;

    Q_PROPERTY(
      QList<resource_managers::Profile*> activeProfiles READ getActiveProfiles
        WRITE setActiveProfiles NOTIFY activeProfilesChanged)
    Q_PROPERTY(resource_managers::Profile* mainProfile READ getMainProfile WRITE
                 setMainProfile NOTIFY mainProfileChanged)
    Q_PROPERTY(
      QList<resource_managers::Profile*> previousSessionProfiles READ
        getPreviousSessionProfiles NOTIFY previousSessionProfilesChanged)

    auto createDefaultProfile() -> resource_managers::Profile*;

    void saveMainProfile();
    void saveActiveProfiles();

  public:
    explicit ProfileList(db::SqliteCppDb* songDb,
                         const QMap<QString, ThemeFamily>& themeFamilies,
                         std::filesystem::path profilesFolder,
                         QObject* parent = nullptr);

    auto rowCount(const QModelIndex& parent = QModelIndex()) const
      -> int override;
    auto data(const QModelIndex& index, int role = Qt::DisplayRole) const
      -> QVariant override;
    auto getProfiles() -> const QList<resource_managers::Profile*>&;

    /**
     * @brief The list of active profiles.
     * If more than one profile is active, battle mode is enabled.
     * There is always at least one profile in the list.
     * @return The list of profiles that are currently active.
     */
    auto getActiveProfiles() -> QList<resource_managers::Profile*>;

    /**
     * @brief Get the profiles that were activated while closing the previous
    session of the game.
    */
    auto getPreviousSessionProfiles() -> QList<resource_managers::Profile*>;

    /**
     * @brief Sets the active profiles.
     * @param activeProfiles The new active profiles.
     */
    void setActiveProfiles(QList<resource_managers::Profile*> activeProfiles);

    Q_INVOKABLE resource_managers::Profile* at(int index) const;
    Q_INVOKABLE resource_managers::Profile* createProfile();
    /**
     * @brief Remove the selected profile, completely deleting it.
     * @warning Be careful.
     */
    Q_INVOKABLE void removeProfile(resource_managers::Profile* profile);
    /**
     * @brief Set the profile that is used to select themes and theme settings.
     * If the profile is not active, sets it to active.
     * @param profile the profile to set as main.
     */
    void setMainProfile(resource_managers::Profile* profile);
    /**
     * @brief The profile used to select themes and theme settings.
     * The main profile is guaranteed to be an active profile.
     */
    auto getMainProfile() const -> resource_managers::Profile*;

  signals:
    void activeProfilesChanged();
    void mainProfileChanged();
    void previousSessionProfilesChanged();
};
} // namespace qml_components

#endif // RHYTHMGAME_PROFILELIST_H
