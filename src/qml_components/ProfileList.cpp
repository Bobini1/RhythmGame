//
// Created by bobini on 30.09.23.
//

#include "ProfileList.h"

#include "support/QStringToPath.h"
#include "support/PathToUtfString.h"

#include <QQmlEngine>
#include <utility>
#include <spdlog/spdlog.h>
#include <QUuid>

auto
qml_components::ProfileList::createInputTranslator() -> input::InputTranslator*
{
    auto* const inputTranslator = new input::InputTranslator(this);
    connect(gamepadManager,
            &input::GamepadManager::axisMoved,
            inputTranslator,
            &input::InputTranslator::handleAxis);
    connect(gamepadManager,
            &input::GamepadManager::buttonPressed,
            inputTranslator,
            &input::InputTranslator::handlePress);
    connect(gamepadManager,
            &input::GamepadManager::buttonReleased,
            inputTranslator,
            &input::InputTranslator::handleRelease);
    return inputTranslator;
}
qml_components::ProfileList::
ProfileList(db::SqliteCppDb* songDb,
            const QMap<QString, ThemeFamily>& themeFamilies,
            std::filesystem::path profilesFolder,
            input::GamepadManager* gamepadManager,
            QObject* parent)
  : QAbstractListModel(parent)
  , profilesFolder(std::move(profilesFolder))
  , songDb(songDb)
  , gamepadManager(gamepadManager)
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
                  entry.path() / "profile.sqlite",
                  themeFamilies,
                  createInputTranslator(),
                  this);
                QQmlEngine::setObjectOwnership(profile,
                                               QQmlEngine::CppOwnership);
                profile->getInputTranslator()->setParent(profile);
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
          this->profilesFolder / "default" / "profile.sqlite",
          themeFamilies,
          createInputTranslator(),
          this);
        QQmlEngine::setObjectOwnership(profile, QQmlEngine::CppOwnership);
        profile->getInputTranslator()->setParent(profile);
        profiles.append(profile);
    }
    const auto dbCurrentProfile =
      songDb->createStatement("SELECT path FROM current_profile")
        .executeAndGet<std::string>();
    if (!dbCurrentProfile.has_value()) {
        setActiveProfiles({ profiles[0] });
        return;
    }
    const auto profilePath =
      support::qStringToPath(QString::fromStdString(dbCurrentProfile.value()));
    const auto absoluteProfilePath = this->profilesFolder / profilePath;
    for (auto i = 0; i < profiles.size(); ++i) {
        if (profiles[i]->getPath() == absoluteProfilePath) {
            setActiveProfiles({ profiles[i] });
            break;
        }
    }
    if (activeProfiles.empty()) {
        setActiveProfiles({ profiles[0] });
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
    if (role == Qt::DisplayRole) {
        return QVariant::fromValue(profiles.at(index.row()));
    }
    return {};
}
auto
qml_components::ProfileList::roleNames() const -> QHash<int, QByteArray>
{
    return { { ProfileRole, "profile" }, { ActiveRole, "active" } };
}
auto
qml_components::ProfileList::createProfile() -> resource_managers::Profile*
{
    resource_managers::Profile* profile = nullptr;
    auto* inputTranslator = createInputTranslator();
    try {
        profile = new resource_managers::Profile(
          profilesFolder / QUuid::createUuid().toString().toStdString() /
            "profile.sqlite",
          themeFamilies,
          inputTranslator,
          this);
        QQmlEngine::setObjectOwnership(profile, QQmlEngine::CppOwnership);
        profile->getInputTranslator()->setParent(profile);
    } catch (const std::exception& e) {
        spdlog::error("Failed to create profile: {}", e.what());
        inputTranslator->deleteLater();
        return nullptr;
    }
    profiles.append(profile);
    emit dataChanged(index(profiles.size() - 1), index(profiles.size() - 1));
    return profile;
}
void
qml_components::ProfileList::removeProfile(resource_managers::Profile* profile)
{
    if (profile == profiles[0]) {
        spdlog::warn("Can't remove primary profile");
        return;
    }
    profile->deleteLater();
    remove_all(profile->getPath().parent_path());
    if (activeProfiles.removeAll(profile) > 0) {
        emit activeProfilesChanged();
    }
    const auto index = profiles.indexOf(profile);
    beginRemoveRows(QModelIndex(), index, index);
    profiles.remove(index);
    endRemoveRows();
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
void
qml_components::ProfileList::setActiveProfiles(
  QList<resource_managers::Profile*> profiles)
{
    if (profiles == activeProfiles) {
        return;
    }
    if (profiles.empty()) {
        spdlog::warn("Can't deactivate all profiles");
        return;
    }
    if (profiles[0] != activeProfiles[0]) {
        auto countStatement =
          songDb->createStatement("SELECT COUNT(*) FROM current_profile");
        auto result = countStatement.executeAndGet<int>();
        if (!result.has_value()) {
            throw std::runtime_error(
              "Failed to get count of rows in current_profile table");
        }
        auto profilePath = profiles[0]->getPath();
        auto relativePath = relative(profilePath, profilesFolder);
        auto pathUtf = support::pathToUtfString(relativePath);
#ifdef _WIN32
        std::ranges::replace(pathUtf, '\\', '/');
#endif
        if (result.value() == 0) {
            // insert new row
            auto statement =
              songDb->createStatement("INSERT INTO current_profile (path) "
                                      "VALUES (:path)");
            statement.bind(":path", pathUtf);
            statement.execute();
            return;
        }
        auto statement =
          songDb->createStatement("UPDATE current_profile SET path = :path");
        statement.bind(":path", support::pathToUtfString(relativePath));
        statement.execute();
    }
    activeProfiles = std::move(profiles);
    emit activeProfilesChanged();
}

auto
qml_components::ProfileList::at(int index) const -> resource_managers::Profile*
{
    return profiles.at(index);
}
