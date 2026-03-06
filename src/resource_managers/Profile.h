//
// Created by bobini on 27.09.23.
//

#ifndef RHYTHMGAME_PROFILE_H
#define RHYTHMGAME_PROFILE_H

#include "Vars.h"
#include "db/SqliteCppDb.h"
#include "input/InputTranslator.h"
#include "qml_components/ScoreDb.h"

#include <QQmlPropertyMap>
#include "qml_components/ThemeFamily.h"

#include <QNetworkRequestFactory>
#include <qt6keychain/keychain.h>
namespace gameplay_logic {
class ChartData;
}
namespace resource_managers {

class Profile final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString path READ getPathQString CONSTANT)
    /** @brief The themes selected for this profile. */
    Q_PROPERTY(QQmlPropertyMap* themeConfig READ getThemeConfig CONSTANT)
    /** @brief The persistent configuration variables of this profile. */
    Q_PROPERTY(Vars* vars READ getVars CONSTANT)
    /**
     * @brief The object used for querying the score database of the profile.
     */
    Q_PROPERTY(qml_components::ScoreDb* scoreDb READ getScoreDb CONSTANT)
    /** @brief The unique identifier of the profile. */
    Q_PROPERTY(QString guid READ getGuid CONSTANT)
    Q_PROPERTY(QString onlineUsername READ getOnlineUsername NOTIFY
                 onlineUsernameChanged)
    Q_PROPERTY(bool loggedIn READ getLoggedIn NOTIFY loggedInChanged)
    db::SqliteCppDb db;
    std::filesystem::path dbPath;
    QQmlPropertyMap* themeConfig;
    Vars vars;
    qml_components::ScoreDb scoreDb{ &db };
    QString guid;
    QNetworkAccessManager networkManager;
    QNetworkRequestFactory networkRequestFactory;
    QString onlineUsername;
    bool loggedIn{};
    auto loadBearerToken() -> void;
    void fetchOnlineData();
    void setOnlineUsername(const QString& username);
    void setLoggedIn(bool loggedIn);

    /**
     * @brief Upload a bulk list of local scores identified by GUIDs.
     * @param guids List of score GUIDs to upload.
     */
    void uploadScoresBulk(const QList<QString>& guids);
    /**
     * @brief Download scores that are missing locally from the server.
     * @param guids List of GUIDs the client currently has; server will
     * respond with any scores missing locally.
     */
    void downloadScoresBulk(const QList<QString>& guids);

  public:
    static inline const QString keychainService = "RhythmGame";
    /**
     * @brief Creates a profile object living in the given database.
     * @details If the profile doesn't exist, it will be created.
     * @param mainDbPath Path to the main song database file. Has to exist.
     * @param dbPath Path to the profile's database file. Doesn't have to exist.
     * @param themeFamilies The available theme families.
     * @param assetsPaths Paths to the avatar folders.
     * @param parent QObject parent.
     */
    explicit Profile(
      const std::filesystem::path& mainDbPath,
      const std::filesystem::path& dbPath,
      const QMap<QString, qml_components::ThemeFamily>& themeFamilies,
      QList<QString> assetsPaths,
      QObject* parent = nullptr);

    auto getPath() const -> std::filesystem::path;
    auto getPathQString() const -> QString;
    auto getDb() -> db::SqliteCppDb&;
    auto getScoreDb() -> qml_components::ScoreDb*;
    auto getThemeConfig() const -> QQmlPropertyMap*;
    auto getVars() -> Vars*;
    auto getGuid() const -> QString;
    Q_INVOKABLE QIfPendingReply<void> login(const QString& email,
                                            const QString& password);
    Q_INVOKABLE void logout();
    auto getOnlineUsername() const -> QString;
    auto getLoggedIn() const -> bool;
    /**
     * @brief Upload local scores to the server.
     * @return Number of scores uploaded.
     */
    Q_INVOKABLE QIfPendingReply<int> uploadScores();
    /**
     * @brief Download scores from the server that are missing locally.
     * @return Number of scores downloaded.
     */
    Q_INVOKABLE QIfPendingReply<int> downloadScores();

    auto submitScore(const gameplay_logic::BmsScore& score,
                     const gameplay_logic::ChartData& chartData)
      -> QNetworkReply*;

  signals:
    void onlineUsernameChanged();
    void loggedInChanged();
};

} // namespace resource_managers

#endif // RHYTHMGAME_PROFILE_H
