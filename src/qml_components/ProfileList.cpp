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

auto
qml_components::ProfileList::createDefaultProfile()
  -> resource_managers::Profile*
{
    auto* profile = new resource_managers::Profile(
      this->profilesFolder / "default" / "profile.sqlite", themeFamilies, this);
    QQmlEngine::setObjectOwnership(profile, QQmlEngine::CppOwnership);
    profiles.append(profile);
    return profile;
}

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
  db::SqliteCppDb* songDb,
  const QMap<QString, ThemeFamily>& themeFamilies,
  std::filesystem::path profilesFolder,
  QObject* parent)
  : QObject(parent)
  , profilesFolder(std::move(profilesFolder))
  , songDb(songDb)
  , themeFamilies(themeFamilies)
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
                  entry.path() / "profile.sqlite", themeFamilies, this);
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
        createDefaultProfile();
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
        setMainProfile({ profiles[0] });
    } else {
        const auto profilePath =
          support::utfStringToPath(dbCurrentProfile.value());
        const auto absoluteProfilePath = this->profilesFolder / profilePath;
        for (auto* profile : profiles) {
            if (profile->getPath() == absoluteProfilePath) {
                setMainProfile({ profile });
                break;
            }
        }
        if (mainProfile == nullptr) {
            setMainProfile({ profiles[0] });
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
          profilesFolder / QUuid::createUuid().toString().toStdString() /
            "profile.sqlite",
          themeFamilies,
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
    profile->deleteLater();
    // We should delete the profile after the profile object (with a db
    // connection) is destroyed
    connect(profile,
            &resource_managers::Profile::destroyed,
            nullptr,
            [path = profile->getPath().parent_path()]() { remove_all(path); });
    const auto index = profiles.indexOf(profile);
    profiles.remove(index);
    if (profiles.empty()) {
        createDefaultProfile();
    }
    emit profilesChanged();
    if (mainProfile == profile) {
        setMainProfile(profiles[0]);
    }
    if (battleProfiles.player1Profile == profile) {
        battleProfiles.setPlayer1Profile(nullptr);
    }
    if (battleProfiles.player2Profile == profile) {
        battleProfiles.setPlayer2Profile(nullptr);
    }
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
void
qml_components::BattleProfiles::setPlayer1Profile(
  resource_managers::Profile* profile)
{
    if (profile == player1Profile) {
        return;
    }
    player1Profile = profile;
    if (profile == player2Profile) {
        setPlayer2Profile(nullptr);
    }
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
    if (profile == player2Profile) {
        return;
    }
    player2Profile = profile;
    if (profile == player1Profile) {
        setPlayer1Profile(nullptr);
    }
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
