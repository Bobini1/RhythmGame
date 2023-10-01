//
// Created by bobini on 30.09.23.
//

#include "ProfileList.h"
#include "support/QStringToPath.h"
#include "support/PathToUtfString.h"

#include <utility>
#include <spdlog/spdlog.h>
#include <QUuid>
qml_components::ProfileList::ProfileList(db::SqliteCppDb* songDb,
                                         std::filesystem::path profilesFolder,
                                         QObject* parent)
  : QAbstractListModel(parent)
  , profilesFolder(std::move(profilesFolder))
  , songDb(songDb)
{
    if (!std::filesystem::exists(this->profilesFolder)) {
        std::filesystem::create_directory(this->profilesFolder);
    }
    if (!std::filesystem::is_directory(this->profilesFolder)) {
        throw std::runtime_error("profiles folder is not a directory");
    }
    for (const auto& entry :
         std::filesystem::directory_iterator(this->profilesFolder)) {
        if (entry.is_directory()) {
            try {
                auto* profile = new resource_managers::Profile(
                  entry.path() / "profile.sqlite");
                profile->setParent(this);
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
        auto* profile = new resource_managers::Profile(
          this->profilesFolder / "default" / "profile.sqlite");
        profile->setParent(this);
        profiles.append(profile);
    }
    auto currentProfile =
      songDb->createStatement("SELECT path FROM current_profile")
        .executeAndGet<std::string>();
    if (!currentProfile.has_value()) {
        setCurrentProfile(profiles[0]);
        return;
    }
    auto profilePath =
      support::qStringToPath(QString::fromStdString(currentProfile.value()));
    auto absoluteProfilePath = this->profilesFolder / profilePath;
    for (auto* profile : profiles) {
        if (profile->getPath() == absoluteProfilePath) {
            setCurrentProfile(profile);
            break;
        }
    }
    if (this->currentProfile == nullptr) {
        setCurrentProfile(profiles[0]);
    }
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
    if (role == ProfileRole) {
        return QVariant::fromValue(profiles.at(index.row()));
    }
    if (role == ActiveRole) {
        return QVariant::fromValue(profiles.at(index.row()) == currentProfile);
    }
    if (role == Qt::DisplayRole) {
        return profiles.at(index.row())->getName();
    }
    return {};
}
auto
qml_components::ProfileList::roleNames() const -> QHash<int, QByteArray>
{
    return { { ProfileRole, "profiles" }, { ActiveRole, "active" } };
}
auto
qml_components::ProfileList::getCurrentProfile() const
  -> resource_managers::Profile*
{
    return currentProfile;
}
auto
qml_components::ProfileList::createProfile() -> resource_managers::Profile*
{
    resource_managers::Profile* profile = nullptr;
    try {
        profile = new resource_managers::Profile(
          profilesFolder / QUuid::createUuid().toString().toStdString() /
          "profile.sqlite");
        profile->setParent(this);
    } catch (const std::exception& e) {
        spdlog::error("Failed to create profile: {}", e.what());
        return nullptr;
    }
    profiles.append(profile);
    emit dataChanged(index(profiles.size() - 1), index(profiles.size() - 1));
    return profile;
}
void
qml_components::ProfileList::removeProfile(resource_managers::Profile* profile)
{
    if (profile == currentProfile) {
        spdlog::warn("Can't remove current profile");
        return;
    }
    auto index = profiles.indexOf(profile);
    if (index == -1) {
        spdlog::warn("Can't remove profile - not found");
        return;
    }
    beginRemoveRows(QModelIndex(), index, index);
    profiles.removeAt(index);
    endRemoveRows();
    delete profile;
    std::filesystem::remove_all(profile->getPath().parent_path());
}
void
qml_components::ProfileList::setCurrentProfile(
  resource_managers::Profile* profile)
{
    if (profile == currentProfile) {
        return;
    }
    auto index = profiles.indexOf(profile);
    if (index == -1) {
        spdlog::warn("Can't set current profile - not found");
        return;
    }
    currentProfile = profile;
    // get count of rows in current_profile table
    auto countStatement =
      songDb->createStatement("SELECT COUNT(*) FROM current_profile");
    auto result = countStatement.executeAndGet<int>();
    if (!result.has_value()) {
        throw std::runtime_error(
          "Failed to get count of rows in current_profile table");
    }
    auto profilePath = profile->getPath();
    auto relativePath = std::filesystem::relative(profilePath, profilesFolder);
    if (result.value() == 0) {
        // insert new row
        auto statement =
          songDb->createStatement("INSERT INTO current_profile (path) "
                                  "VALUES (:path)");
        statement.bind(":path", support::pathToUtfString(relativePath));
        statement.execute();
        emit currentProfileChanged(profile);
        return;
    }
    auto statement =
      songDb->createStatement("UPDATE current_profile SET path = :path");
    statement.bind(":path", support::pathToUtfString(relativePath));
    statement.execute();
    emit currentProfileChanged(profile);
}
