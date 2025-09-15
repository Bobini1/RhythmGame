//
// Created by bobini on 30.09.23.
//

#ifndef RHYTHMGAME_PROFILELIST_H
#define RHYTHMGAME_PROFILELIST_H

#include "resource_managers/Profile.h"

#include <qproperty.h>
namespace resource_managers {
class inputTranslator;
} // namespace resource_managers
namespace qml_components {
class ThemeFamily;

/**
 * @brief The profiles used in battle mode.
 * @details This class contains the two profiles used in battle mode.
 * You need to set both profiles before setting ProfileList::battleActive.
 */
class BattleProfiles final : public QObject
{
    Q_OBJECT

    Q_PROPERTY(resource_managers::Profile* player1Profile READ getPlayer1Profile
                 WRITE setPlayer1Profile NOTIFY player1ProfileChanged)
    Q_PROPERTY(resource_managers::Profile* player2Profile READ getPlayer2Profile
                 WRITE setPlayer2Profile NOTIFY player2ProfileChanged)

    resource_managers::Profile* player1Profile{};
    resource_managers::Profile* player2Profile{};

    friend class ProfileList;

  public:
    void setPlayer1Profile(resource_managers::Profile* profile);
    auto getPlayer1Profile() const -> resource_managers::Profile*;

    void setPlayer2Profile(resource_managers::Profile* profile);
    auto getPlayer2Profile() const -> resource_managers::Profile*;

  signals:
    void player1ProfileChanged();
    void player2ProfileChanged();
};

/**
 * @brief The list of local profiles.
 * @details Besides listing all local profiles, this class also contains the
 * list of active profiles (for battle mode) and the main profile, used to
 * select themes and theme settings.
 */
class ProfileList final : public QObject
{
    Q_OBJECT
    /**
     * @brief The profile that is used to select themes and theme settings.
     * Never null.
     * @details It also specifies the shared settings in battle mode.
     * The main profile does not need to be assigned to P1 or P2.
     */
    Q_PROPERTY(resource_managers::Profile* mainProfile READ getMainProfile WRITE
                 setMainProfile NOTIFY mainProfileChanged)
    /**
     * @brief The profiles used in battle mode.
     * @see battleActive
     */
    Q_PROPERTY(BattleProfiles* battleProfiles READ getBattleProfiles CONSTANT)
    /**
     * @brief The list of all available profiles.
     */
    Q_PROPERTY(QList<resource_managers::Profile*> profiles READ getProfiles
                 NOTIFY profilesChanged)
    /**
     * @brief Launch SP charts in battle mode with two profiles.
     * @details Both battleProfiles.player1Profile and
     * battleProfiles.player2Profile must be set before enabling battle.
     */
    Q_PROPERTY(bool battleActive READ getBattleActive WRITE setBattleActive
                 NOTIFY battleActiveChanged)

    std::filesystem::path profilesFolder;
    std::filesystem::path mainDbPath;
    QList<resource_managers::Profile*> profiles;
    db::SqliteCppDb* songDb;
    QMap<QString, ThemeFamily> themeFamilies;
    resource_managers::Profile* mainProfile{};
    BattleProfiles battleProfiles;
    bool battleActive{};
    QList<QString> avatarPaths;

    void saveMainProfile();
    void saveActiveProfiles();

  public:
    explicit ProfileList(std::filesystem::path mainDbPath,
                         db::SqliteCppDb* songDb,
                         const QMap<QString, ThemeFamily>& themeFamilies,
                         std::filesystem::path profilesFolder,
                         QList<QString> avatarPaths,
                         QObject* parent = nullptr);
    auto getProfiles() -> const QList<resource_managers::Profile*>&;

    Q_INVOKABLE resource_managers::Profile* at(int index) const;
    Q_INVOKABLE resource_managers::Profile* createProfile();
    /**
     * @brief Remove the selected profile, completely deleting it.
     * @warning Be careful.
     */
    Q_INVOKABLE void removeProfile(resource_managers::Profile* profile);
    void setMainProfile(resource_managers::Profile* profile);
    auto getMainProfile() const -> resource_managers::Profile*;

    auto getBattleProfiles() -> BattleProfiles*;

    auto getBattleActive() const -> bool;
    void setBattleActive(bool active);

  signals:
    void mainProfileChanged();
    void profilesChanged();
    void battleActiveChanged();
};
} // namespace qml_components

#endif // RHYTHMGAME_PROFILELIST_H
