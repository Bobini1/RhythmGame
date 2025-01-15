//
// Created by bobini on 30.09.23.
//

#ifndef RHYTHMGAME_PROFILELIST_H
#define RHYTHMGAME_PROFILELIST_H

#include <QAbstractListModel>
#include "resource_managers/Profile.h"

#include <qpointer.h>
#include <qproperty.h>
#include <qqmllist.h>
namespace resource_managers {
class inputTranslator;
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
    input::InputTranslator* keyConfigs;
    QMap<QString, ThemeFamily> themeFamilies;
    resource_managers::Profile* player1Profile{};
    resource_managers::Profile* player2Profile{};
    resource_managers::Profile* mainProfile{};

    Q_PROPERTY(resource_managers::Profile* player1Profile READ getPlayer1Profile
                 WRITE setPlayer1Profile NOTIFY player1ProfileChanged)
    Q_PROPERTY(resource_managers::Profile* player2Profile READ getPlayer2Profile
                 WRITE setPlayer2Profile NOTIFY player2ProfileChanged)
    Q_PROPERTY(resource_managers::Profile* mainProfile READ getMainProfile WRITE
                 setMainProfile NOTIFY mainProfileChanged)

    auto createDefaultProfile() -> resource_managers::Profile*;

    void saveMainProfile();
    void saveActiveProfiles() const;

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

    Q_INVOKABLE resource_managers::Profile* at(int index) const;
    Q_INVOKABLE resource_managers::Profile* createProfile();
    /**
     * @brief Remove the selected profile, completely deleting it.
     * @warning Be careful.
     */
    Q_INVOKABLE void removeProfile(resource_managers::Profile* profile);
    /**
     * @brief Set the profile that is used to select themes and theme settings.
     * @details It also specifies the layout in battle mode.
     * The main profile does not need to be assigned to P1 or P2.
     * @param profile the profile to set as main.
     */
    void setMainProfile(resource_managers::Profile* profile);
    /**
     * @brief The profile used to select themes and theme settings.
     * @details The main profile does not need to be assigned to P1 or P2.
     */
    auto getMainProfile() const -> resource_managers::Profile*;

    void setPlayer1Profile(resource_managers::Profile* profile);

    auto getPlayer1Profile() const -> resource_managers::Profile*;

    void setPlayer2Profile(resource_managers::Profile* profile);

    auto getPlayer2Profile() const -> resource_managers::Profile*;

  signals:
    void player1ProfileChanged();
    void player2ProfileChanged();
    void mainProfileChanged();
};
} // namespace qml_components

#endif // RHYTHMGAME_PROFILELIST_H
