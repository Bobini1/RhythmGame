//
// Created by bobini on 27.09.23.
//

#ifndef RHYTHMGAME_PROFILE_H
#define RHYTHMGAME_PROFILE_H

#include "Vars.h"
#include "db/SqliteCppDb.h"
#include "input/InputTranslator.h"
#include "qml_components/ScoreDb.h"
#include "qml_components/ScoreSyncOperation.h"

#include <QQmlPropertyMap>
#include "qml_components/ThemeFamily.h"

#include <QNetworkRequestFactory>
#include <qt6keychain/keychain.h>
namespace gameplay_logic {
class ChartData;
}
namespace resource_managers {
class TachiData
{
    Q_GADGET
    Q_PROPERTY(qint64 userId MEMBER userId CONSTANT)
    Q_PROPERTY(QString username MEMBER username CONSTANT)
    Q_PROPERTY(QString image MEMBER image CONSTANT)
  public:
    qint64 userId;
    QString username;
    QString image;
    auto operator<=>(const TachiData&) const = default;
};
class OnlineUserData
{
    Q_GADGET
    Q_PROPERTY(qint64 userId MEMBER userId CONSTANT)
    Q_PROPERTY(QString username MEMBER username CONSTANT)
    Q_PROPERTY(QString image MEMBER image CONSTANT)
  public:
    qint64 userId;
    QString username;
    QString image;
    auto operator<=>(const OnlineUserData&) const = default;
};

class Profile final : public QObject
{
    Q_OBJECT
  public:
    enum class LoginState
    {
        NotLoggedIn,
        LoggingIn,
        LoggedIn,
        LoginFailed
    };
    Q_ENUM(LoginState)
  private:
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
    Q_PROPERTY(QVariant onlineUserData READ getOnlineUserData NOTIFY
                 onlineUserDataChanged)
    Q_PROPERTY(QVariant tachiData READ getTachiData NOTIFY tachiDataChanged)
    Q_PROPERTY(resource_managers::Profile::LoginState loginState READ
                 getLoginState NOTIFY loginStateChanged)
    Q_PROPERTY(LoginState tachiLoginState READ getTachiLoginState NOTIFY
                 loginStateChanged)
    db::SqliteCppDb db;
    std::filesystem::path dbPath;
    QQmlPropertyMap* themeConfig;
    Vars vars;
    qml_components::ScoreDb scoreDb{ &db };
    QString guid;
    QNetworkAccessManager* networkManager;
    QNetworkRequestFactory networkRequestFactory;
    QThreadPool threadPool;

    LoginState loginState{ LoginState::NotLoggedIn };
    LoginState tachiLoginState{ LoginState::NotLoggedIn };
    std::optional<OnlineUserData> userData;
    std::optional<TachiData> tachiData;
    void loadBearerToken();
    void fetchOnlineData();
    void setLoginState(LoginState state);
    void setTachiLoginState(LoginState state);
    void setOnlineUserData(std::optional<OnlineUserData> userData);
    void setTachiData(std::optional<TachiData> tachiData);

  public:
    static inline const QString keychainService = "RhythmGame";
    /**
     * @brief Creates a profile object living in the given database.
     * @details If the profile doesn't exist, it will be created.
     * @param mainDbPath Path to the main song database file. Has to exist.
     * @param dbPath Path to the profile's database file. Doesn't have to exist.
     * @param themeFamilies The available theme families.
     * @param assetsPaths Paths to the avatar folders.
     * @param networkManager The network manager to use for online operations.
     * @param parent QObject parent.
     */
    explicit Profile(
      const std::filesystem::path& mainDbPath,
      const std::filesystem::path& dbPath,
      const QMap<QString, qml_components::ThemeFamily>& themeFamilies,
      QList<QString> assetsPaths,
      QNetworkAccessManager* networkManager,
      QObject* parent = nullptr);

    auto getPath() const -> std::filesystem::path;
    auto getPathQString() const -> QString;
    auto getDb() -> db::SqliteCppDb&;
    auto getScoreDb() -> qml_components::ScoreDb*;
    auto getThemeConfig() const -> QQmlPropertyMap*;
    auto getVars() -> Vars*;
    auto getGuid() const -> QString;
    void fetchTachiData(int tachiId);
    Q_INVOKABLE void login(const QString& email, const QString& password);
    Q_INVOKABLE void logout();
    auto getLoginState() const -> LoginState;
    auto getTachiLoginState() const -> LoginState;
    auto getOnlineUserData() const -> QVariant;
    auto getTachiData() const -> QVariant;
    /**
     * @brief Upload local scores to the server.
     * @details Compares local GUIDs to the server and uploads each missing
     * score individually. Returns a ScoreSyncOperation that reports progress
     * and per-score errors.
     */
    Q_INVOKABLE qml_components::ScoreSyncOperation* uploadScores();
    /**
     * @brief Download scores from the server that are missing locally.
     * @details Compares server GUIDs to local storage and downloads each
     * missing score individually. Returns a ScoreSyncOperation that reports
     * progress and per-score errors.
     */
    Q_INVOKABLE qml_components::ScoreSyncOperation* downloadScores();

    auto submitScore(const gameplay_logic::BmsScore& score,
                     const gameplay_logic::ChartData& chartData) const
      -> QNetworkReply*;

  signals:
    void loginStateChanged();
    void tachiLoginStateChanged();
    void onlineUserDataChanged();
    void tachiDataChanged();
};

} // namespace resource_managers

#endif // RHYTHMGAME_PROFILE_H
