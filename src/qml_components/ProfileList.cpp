//
// Created by bobini on 30.09.23.
//

#include "ProfileList.h"

#include "support/QStringToPath.h"
#include "support/PathToUtfString.h"
#include "support/UtfStringToPath.h"

#include <QQmlEngine>
#include <utility>
#include <spdlog/spdlog.h>
#include <QUuid>

void
qml_components::ProfileList::saveMainProfile()
{
    auto statement =
      songDb->createStatement("INSERT OR REPLACE INTO properties(key, value) "
                              "VALUES('main_profile', :value)");
    const auto relativePath = support::pathToUtfString(
      relative(mainProfile->getPath(), profilesFolder));
    statement.bind(":value", relativePath);
    statement.execute();
}
void
qml_components::ProfileList::saveActiveProfiles()
{
    if (!battleProfiles.player1Profile || !battleProfiles.player2Profile) {
        setBattleActive(/*active=*/false);
    }
    auto statement =
      songDb->createStatement("INSERT OR REPLACE INTO properties(key, value) "
                              "VALUES('active_profiles', :value)");
    auto activeProfilesString = std::string{};
    if (battleProfiles.player1Profile != nullptr) {
        activeProfilesString += support::pathToUtfString(
          relative(battleProfiles.player1Profile->getPath(), profilesFolder));
    }
    activeProfilesString += ":";
    if (battleProfiles.player2Profile != nullptr) {
        activeProfilesString += support::pathToUtfString(
          relative(battleProfiles.player2Profile->getPath(), profilesFolder));
    }
    statement.bind(":value", activeProfilesString);
    statement.execute();
}

qml_components::ProfileList::ProfileList(
  std::filesystem::path mainDbPath,
  db::SqliteCppDb* songDb,
  const QMap<QString, ThemeFamily>& themeFamilies,
  std::filesystem::path profilesFolder,
  QString avatarPath,
  QObject* parent)
  : QObject(parent)
  , profilesFolder(std::move(profilesFolder))
  , mainDbPath(std::move(mainDbPath))
  , songDb(songDb)
  , themeFamilies(themeFamilies)
  , avatarPath(std::move(avatarPath))
{
    if (!exists(this->profilesFolder)) {
        create_directory(this->profilesFolder);
    }
    if (!is_directory(this->profilesFolder)) {
        throw std::runtime_error("profiles folder is not a directory");
    }
    for (const auto& entry :
         std::filesystem::directory_iterator(this->profilesFolder)) {
        if (entry.is_directory()) {
            try {
                auto* profile = new resource_managers::Profile(
                  this->mainDbPath,
                  entry.path() / "profile.sqlite",
                  themeFamilies,
                  this->avatarPath,
                  this);
                QQmlEngine::setObjectOwnership(profile,
                                               QQmlEngine::CppOwnership);
                profiles.append(profile);
            } catch (const std::exception& e) {
                spdlog::error("Failed to load profile {}: {}",
                              entry.path().string(),
                              e.what());
            }
        }
    }
    if (profiles.empty()) {
        spdlog::info("No profiles found, creating default profile");
        if (auto* profile = createProfile(); profile == nullptr) {
            throw std::runtime_error("Failed to create default profile");
        }
    }

    const auto dbActiveProfiles =
      songDb
        ->createStatement(
          "SELECT value FROM properties WHERE key = 'active_profiles'")
        .executeAndGet<std::string>()
        .value_or(":");
    if (std::ranges::find(dbActiveProfiles, ':') != dbActiveProfiles.end()) {
        if (const auto firstProfile =
              dbActiveProfiles.substr(0, dbActiveProfiles.find(':'));
            !firstProfile.empty()) {
            for (auto* profile : profiles) {
                if (profile->getPath() == this->profilesFolder / firstProfile) {
                    battleProfiles.setPlayer1Profile(profile);
                    break;
                }
            }
        }
        if (const auto secondProfile =
              dbActiveProfiles.substr(dbActiveProfiles.find(':') + 1);
            !secondProfile.empty()) {
            for (auto* profile : profiles) {
                if (profile->getPath() ==
                    this->profilesFolder / secondProfile) {
                    battleProfiles.setPlayer2Profile(profile);
                    break;
                }
            }
        }
    }

    const auto dbCurrentProfile =
      songDb
        ->createStatement(
          "SELECT value FROM properties WHERE key = 'main_profile'")
        .executeAndGet<std::string>();
    if (!dbCurrentProfile.has_value()) {
        setMainProfile(profiles[0]);
    } else {
        const auto profilePath =
          support::utfStringToPath(dbCurrentProfile.value());
        const auto absoluteProfilePath = this->profilesFolder / profilePath;
        for (auto* profile : profiles) {
            if (profile->getPath() == absoluteProfilePath) {
                setMainProfile(profile);
                break;
            }
        }
        if (mainProfile == nullptr) {
            setMainProfile(profiles[0]);
        }
    }

    connect(&battleProfiles,
            &BattleProfiles::player1ProfileChanged,
            this,
            &ProfileList::saveActiveProfiles);
    connect(&battleProfiles,
            &BattleProfiles::player2ProfileChanged,
            this,
            &ProfileList::saveActiveProfiles);
}
auto
qml_components::ProfileList::createProfile() -> resource_managers::Profile*
{
    try {
        auto* profile = new resource_managers::Profile(
          this->mainDbPath,
          profilesFolder / QUuid::createUuid().toString().toStdString() /
            "profile.sqlite",
          themeFamilies,
          avatarPath,
          this);
        QQmlEngine::setObjectOwnership(profile, QQmlEngine::CppOwnership);
        profiles.append(profile);
        emit profilesChanged();
        return profile;
    } catch (const std::exception& e) {
        spdlog::error("Failed to create profile: {}", e.what());
        return nullptr;
    }
}

void
qml_components::ProfileList::removeProfile(resource_managers::Profile* profile)
{
    if (profile == nullptr) {
        return;
    }
    if (profiles.size() == 1) {
        auto* profile = createProfile();
        if (profile == nullptr) {
            return;
        }
    }
    const auto index = profiles.indexOf(profile);
    profiles.remove(index);
    if (profile == mainProfile) {
        setMainProfile(profiles[0]);
    }
    if (battleProfiles.player1Profile == profile) {
        battleProfiles.setPlayer1Profile(nullptr);
    }
    if (battleProfiles.player2Profile == profile) {
        battleProfiles.setPlayer2Profile(nullptr);
    }
    profile->deleteLater();
    // We should delete the profile after the profile object (with a db
    // connection) is destroyed.
    connect(profile,
            &resource_managers::Profile::destroyed,
            this,
            [path = profile->getPath().parent_path(), this] {
                auto ec = std::error_code{};
                remove_all(path, ec);
                if (ec) {
                    spdlog::error("Failed to remove profile: {}", ec.message());
                }
            });
    emit profilesChanged();
}
void
qml_components::ProfileList::setMainProfile(resource_managers::Profile* profile)
{
    if (profile == nullptr || profile == mainProfile) {
        return;
    }
    if (!profiles.contains(profile)) {
        spdlog::warn("ProfileList::setMainProfile: Profile is not in the list "
                     "of profiles. Probably being deleted.");
        return;
    }
    mainProfile = profile;
    saveMainProfile();
    emit mainProfileChanged();
}
auto
qml_components::ProfileList::getMainProfile() const
  -> resource_managers::Profile*
{
    return mainProfile;
}
auto
qml_components::ProfileList::getBattleProfiles() -> BattleProfiles*
{
    return &battleProfiles;
}
auto
qml_components::ProfileList::getBattleActive() const -> bool
{
    return battleActive;
}
void
qml_components::ProfileList::setBattleActive(bool active)
{
    if (active == battleActive) {
        return;
    }
    if (active &&
        !(battleProfiles.player1Profile && battleProfiles.player2Profile)) {
        spdlog::warn("Can't start battle mode before setting battle profiles");
        return;
    }
    battleActive = active;
    emit battleActiveChanged();
}

void
qml_components::BattleProfiles::setPlayer1Profile(
  resource_managers::Profile* profile)
{
    if (profile == player1Profile || profile == player2Profile && profile) {
        return;
    }
    player1Profile = profile;
    emit player1ProfileChanged();
}
auto
qml_components::BattleProfiles::getPlayer1Profile() const
  -> resource_managers::Profile*
{
    return player1Profile;
}
void
qml_components::BattleProfiles::setPlayer2Profile(
  resource_managers::Profile* profile)
{
    if (profile == player2Profile || profile == player1Profile && profile) {
        return;
    }
    player2Profile = profile;
    emit player2ProfileChanged();
}

auto
qml_components::BattleProfiles::getPlayer2Profile() const
  -> resource_managers::Profile*
{
    return player2Profile;
}

auto
qml_components::ProfileList::getProfiles()
  -> const QList<resource_managers::Profile*>&
{
    return profiles;
}

auto
qml_components::ProfileList::at(const int index) const
  -> resource_managers::Profile*
{
    return profiles.at(index);
}
