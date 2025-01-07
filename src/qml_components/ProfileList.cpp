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
    for (const auto* profile : activeProfiles) {
        const auto relativePath = relative(profile->getPath(), profilesFolder);
        activeProfilesString += support::pathToUtfString(relativePath);
        activeProfilesString += ':';
    }
    activeProfilesString.pop_back();
    statement.bind(":value", activeProfilesString);
    statement.execute();
}

qml_components::ProfileList::ProfileList(
  db::SqliteCppDb* songDb,
  const QMap<QString, ThemeFamily>& themeFamilies,
  std::filesystem::path profilesFolder,
  QObject* parent)
  : QAbstractListModel(parent)
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
        .value_or("");
    auto dbActiveProfilesVec = std::vector<std::filesystem::path>{};
    for (auto it = begin(dbActiveProfiles); it != end(dbActiveProfiles);) {
        auto end = std::find(it, cend(dbActiveProfiles), ':');
        for (auto* profile : profiles) {
            if (profile->getPath() ==
                this->profilesFolder /
                  support::utfStringToPath(std::string(it, end))) {
                previousSessionProfiles.push_back(profile);
                break;
            }
        }
        it = end;
    }

    const auto dbCurrentProfile =
      songDb
        ->createStatement(
          "SELECT value FROM properties WHERE key = 'main_profile'")
        .executeAndGet<std::string>();
    if (!dbCurrentProfile.has_value()) {
        setMainProfile({ profiles[0] });
        return;
    }
    const auto profilePath = support::utfStringToPath(dbCurrentProfile.value());
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
    saveMainProfile();
    saveActiveProfiles();
}
auto
qml_components::ProfileList::rowCount(const QModelIndex& parent) const -> int
{
    return profiles.size();
}
auto
qml_components::ProfileList::data(const QModelIndex& index, int role) const
  -> QVariant
{
    if (role == Qt::DisplayRole) {
        return QVariant::fromValue(profiles.at(index.row()));
    }
    return {};
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
        emit dataChanged(index(profiles.size() - 1),
                         index(profiles.size() - 1));
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
    if (activeProfiles.removeAll(profile) > 0) {
        saveActiveProfiles();
        emit activeProfilesChanged();
    }
    if (previousSessionProfiles.removeAll(profile) > 0) {
        emit previousSessionProfilesChanged();
    }
    const auto index = profiles.indexOf(profile);
    beginRemoveRows(QModelIndex(), index, index);
    profiles.remove(index);
    endRemoveRows();
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
    if (!activeProfiles.contains(profile)) {
        activeProfiles.push_front(profile);
        saveActiveProfiles();
        emit activeProfilesChanged();
    }
}
auto
qml_components::ProfileList::getMainProfile() const
  -> resource_managers::Profile*
{
    return mainProfile;
}

auto
qml_components::ProfileList::getProfiles()
  -> const QList<resource_managers::Profile*>&
{
    return profiles;
}
auto
qml_components::ProfileList::getActiveProfiles()
  -> QList<resource_managers::Profile*>
{
    return activeProfiles;
}
auto
qml_components::ProfileList::getPreviousSessionProfiles()
  -> QList<resource_managers::Profile*>
{
    return previousSessionProfiles;
}
void
qml_components::ProfileList::setActiveProfiles(
  QList<resource_managers::Profile*> newActiveProfiles)
{
    // contains duplicates or nulls
    if (std::ranges::unique(newActiveProfiles).size() !=
        newActiveProfiles.size()) {
        spdlog::warn("ProfileList::setActiveProfiles: Contains duplicates");
        return;
    }
    if (newActiveProfiles.contains(nullptr)) {
        spdlog::warn("ProfileList::setActiveProfiles: Contains nulls");
        return;
    }
    if (newActiveProfiles.empty()) {
        spdlog::warn("Can't deactivate all profiles");
        return;
    }
    if (newActiveProfiles == activeProfiles) {
        return;
    }
    activeProfiles = std::move(newActiveProfiles);
    if (!activeProfiles.contains(mainProfile)) {
        setMainProfile(activeProfiles[0]);
    }
    saveActiveProfiles();
    emit activeProfilesChanged();
}

auto
qml_components::ProfileList::at(int index) const -> resource_managers::Profile*
{
    return profiles.at(index);
}
