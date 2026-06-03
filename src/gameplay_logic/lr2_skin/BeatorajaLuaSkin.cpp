#include "BeatorajaLuaSkin.h"

#include "support/PathToQString.h"
#include "support/QStringToPath.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QObject>
#include <QPointer>
#include <QRegularExpression>
#include <QSharedPointer>
#include <QStringList>
#include <QTimer>
#include <QUrl>
#include <QVariantList>
#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <spdlog/spdlog.h>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

namespace gameplay_logic::lr2_skin {
namespace {

class BeatorajaLuaRuntime;

constexpr auto LuaCallbackVariantKey = "__beatorajaLuaCallback";
constexpr double LuaTimerOffValue = -9223372036854775808.0;

struct FilePathOption
{
    QString name;
    QString path;
    QString def;
    QString settingId;
};

struct FontDefinition
{
    QString path;
    QString family;
    int type = 0;
    bool bitmap = false;
};

struct LuaExecutionContext
{
    std::filesystem::path skinPath;
    QVariantMap header;
    QVariantMap settingValues;
    std::set<int> activeOptions;
    QList<FilePathOption> filePaths;
    QMap<QString, int> selectedOptions;
    BeatorajaLuaRuntime* runtime = nullptr;
};

struct LuaState
{
    lua_State* state = luaL_newstate();

    LuaState()
    {
        if (state != nullptr) {
            luaL_openlibs(state);
        }
    }

    ~LuaState()
    {
        if (state != nullptr) {
            lua_close(state);
        }
    }

    LuaState(const LuaState&) = delete;
    LuaState& operator=(const LuaState&) = delete;
};

class BeatorajaLuaRuntime : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QObject* stateProvider READ stateProvider WRITE setStateProvider NOTIFY stateProviderChanged)
    Q_PROPERTY(int revision READ revision NOTIFY revisionChanged)

  public:
    explicit BeatorajaLuaRuntime(QObject* parent = nullptr);
    ~BeatorajaLuaRuntime() override;

    QObject* stateProvider() const;
    void setStateProvider(QObject* provider);
    int revision() const;

    bool load(const std::filesystem::path& skinPath,
              const QVariantMap& header,
              const QVariantMap& settingValues,
              const std::set<int>& activeOptions,
              QVariantMap& skin);
    int registerCallback(lua_State* lua, int index);
    int registerActionScriptCallback(const QString& script);
    int registerExpressionCallback(const QString& expression);
    int registerTimerScriptCallback(const QString& script);
    int callbackIdFromVariant(const QVariant& value, const QString& scriptKind);
    void registerCustomTimer(int timerId, int callbackId);
    void registerCustomEvent(int eventId,
                             int actionCallbackId,
                             int conditionCallbackId,
                             int minIntervalMs);

    Q_INVOKABLE QVariant call(int callbackId);
    Q_INVOKABLE int callInt(int callbackId);
    Q_INVOKABLE QString callString(int callbackId);
    Q_INVOKABLE bool callBool(int callbackId, bool fallback);
    Q_INVOKABLE void callAction(int callbackId);
    Q_INVOKABLE void callFloatWriter(int callbackId, double value);
    Q_INVOKABLE bool hasCustomTimer(int timerId) const;
    Q_INVOKABLE QVariant customTimerMicroValue(int timerId) const;
    Q_INVOKABLE QVariantMap customTimerValuesMs() const;
    Q_INVOKABLE QVariantMap updateCustomObjects();
    Q_INVOKABLE bool hasCustomEvent(int eventId) const;
    Q_INVOKABLE bool callEvent(int eventId, int arg1 = 0, int arg2 = 0);

    QVariant providerValue(const char* method,
                           int id,
                           const QVariant& fallback = {}) const;
    bool providerOption(int option, bool fallback) const;
    void setCustomTimerMicroValue(int timerId, double microValue);
    double currentTimeMicro() const;
    static bool timerIsOff(double value);

  signals:
    void stateProviderChanged();
    void revisionChanged();

  private:
    struct CustomEvent
    {
        int actionCallbackId = 0;
        int conditionCallbackId = 0;
        int minIntervalMs = 0;
        double lastExecuteMicro = LuaTimerOffValue;
    };

    QVariant callCallbackVariant(int callbackId,
                                 bool keepResult,
                                 const QVariantList& arguments = {});
    void bumpRevision();

    LuaState m_lua;
    LuaExecutionContext m_context;
    QHash<int, int> m_callbackRefs;
    QHash<int, int> m_customTimerCallbacks;
    QHash<int, double> m_customTimerValuesMicro;
    QHash<int, CustomEvent> m_customEvents;
    QPointer<QObject> m_stateProvider;
    int m_nextCallbackId = 1;
    int m_revision = 0;
};

struct JudgePart
{
    int type = 0;
    int side = 1;
    int judgementIndex = -1;
    QString sourceId;
    QVariantMap destination;
};

struct ConvertState
{
    std::filesystem::path skinPath;
    QVariantMap settingValues;
    QVariantMap header;
    QList<FilePathOption> filePaths;
    QList<Lr2Element> elements;
    QMap<QString, QString> sourcePaths;
    QMap<QString, FontDefinition> fonts;
    QMap<QString, Lr2SrcImage> images;
    QMap<QString, Lr2SrcImage> imageSets;
    QMap<QString, Lr2SrcNumber> numbers;
    QMap<QString, Lr2SrcText> texts;
    QMap<QString, Lr2SrcImage> sliders;
    QMap<QString, Lr2SrcBarGraph> graphs;
    QMap<QString, Lr2SrcNoteChart> noteCharts;
    QMap<QString, Lr2SrcBpmChart> bpmCharts;
    QMap<QString, Lr2SrcImage> resultCharts;
    QMap<QString, Lr2SrcImage> gauges;
    QMap<QString, QList<JudgePart>> judges;
    BeatorajaLuaRuntime* runtime = nullptr;
    std::set<QString> hiddenCoverIds;
    std::set<QString> liftCoverIds;
    QString bgaId;
    QString noteId;
    QVariantList noteSources;
    QVariantList mineSources;
    QVariantList lnStartSources;
    QVariantList lnEndSources;
    QVariantList lnBodySources;
    QVariantList lnBodyActiveSources;
    QVariantList hcnStartSources;
    QVariantList hcnEndSources;
    QVariantList hcnBodySources;
    QVariantList hcnBodyActiveSources;
    QVariantList hcnBodyReactiveSources;
    QVariantList hcnBodyMissSources;
    QVariantList noteDsts;
    QVariantList lineSources;
    QVariantList lineDsts;
    QMap<int, QVariantList> barBodyOffDsts;
    QMap<int, QVariantList> barBodyOnDsts;
    std::set<int> activeOptions;
    std::set<int> usedOptions;
    std::set<int> usedElementOptions;
    std::set<int> barLampVariants;
    std::set<int> barTitleTypes;
    int barCenter = 0;
    int barAvailableStart = 0;
    int barAvailableEnd = -1;
    int sortId = 0;
    int skinWidth = 1280;
    int skinHeight = 720;
    int barChildHeight = 0;
};

enum class DstCoordinateSpace
{
    Screen,
    BarChild,
    Relative
};

auto
asMap(const QVariant& value) -> QVariantMap
{
    if (value.metaType().id() == QMetaType::QVariantMap) {
        return value.toMap();
    }
    if (value.metaType().id() == QMetaType::QVariantHash) {
        QVariantMap result;
        const auto hash = value.toHash();
        for (auto it = hash.cbegin(); it != hash.cend(); ++it) {
            result.insert(it.key(), it.value());
        }
        return result;
    }
    return {};
}

auto
asList(const QVariant& value) -> QVariantList
{
    if (value.metaType().id() == QMetaType::QVariantList) {
        return value.toList();
    }
    if (!value.isValid() || value.isNull()) {
        return {};
    }
    const auto map = asMap(value);
    if (map.isEmpty()) {
        return {};
    }

    QVariantList result;
    int maxKey = 0;
    for (auto it = map.cbegin(); it != map.cend(); ++it) {
        bool ok = false;
        const int key = it.key().toInt(&ok);
        if (!ok || key < 1) {
            return {};
        }
        maxKey = std::max(maxKey, key);
    }
    for (int i = 1; i <= maxKey; ++i) {
        result.append(map.value(QString::number(i)));
    }
    return result;
}

auto
toInt(const QVariant& value, const int fallback = 0) -> int
{
    if (!value.isValid() || value.isNull()) {
        return fallback;
    }
    bool ok = false;
    const int integer = value.toInt(&ok);
    if (ok) {
        return integer;
    }
    const double real = value.toDouble(&ok);
    if (ok) {
        return static_cast<int>(real);
    }
    return fallback;
}

auto
toDouble(const QVariant& value, const double fallback = 0.0) -> double
{
    if (!value.isValid() || value.isNull()) {
        return fallback;
    }
    bool ok = false;
    const double real = value.toDouble(&ok);
    return ok ? real : fallback;
}

auto
toBool(const QVariant& value, const bool fallback = false) -> bool
{
    if (!value.isValid() || value.isNull()) {
        return fallback;
    }
    bool ok = false;
    const int integer = value.toInt(&ok);
    if (ok) {
        return integer != 0;
    }
    if (value.metaType().id() == QMetaType::Bool) {
        return value.toBool();
    }
    return fallback;
}

auto
luaCallbackId(const QVariant& value) -> int
{
    const auto marker = asMap(value);
    if (marker.isEmpty()) {
        return 0;
    }
    return toInt(marker.value(QString::fromLatin1(LuaCallbackVariantKey)));
}

auto
beatorajaPropertyCallback(ConvertState& state,
                          const QVariant& value,
                          const QString& scriptKind) -> int
{
    return state.runtime != nullptr
             ? state.runtime->callbackIdFromVariant(value, scriptKind)
             : luaCallbackId(value);
}

auto
toExactInt(const QVariant& value) -> std::optional<int>
{
    if (!value.isValid() || value.isNull()) {
        return std::nullopt;
    }

    bool ok = false;
    const int integer = value.toInt(&ok);
    if (ok) {
        return integer;
    }

    const double real = value.toDouble(&ok);
    if (ok && std::isfinite(real) && std::floor(real) == real &&
        real >= std::numeric_limits<int>::min() &&
        real <= std::numeric_limits<int>::max()) {
        return static_cast<int>(real);
    }

    return std::nullopt;
}

auto
idString(const QVariant& value) -> QString
{
    if (!value.isValid() || value.isNull()) {
        return {};
    }
    bool ok = false;
    const double real = value.toDouble(&ok);
    if (ok && std::isfinite(real) &&
        std::floor(real) == real) {
        return QString::number(static_cast<qint64>(real));
    }
    return value.toString();
}

auto
makeSafeId(QString text, const QString& fallback) -> QString
{
    text = text.toLower().replace(QRegularExpression("[^a-z0-9]+"), "_");
    text = text.replace(QRegularExpression("^_|_$"), "");
    return text.isEmpty() ? fallback : text;
}

auto
genericPathString(const std::filesystem::path& path) -> QString
{
    return support::pathToQString(path).replace('\\', '/');
}

auto
relativePathString(const std::filesystem::path& path,
                   const std::filesystem::path& base) -> QString
{
    std::error_code ec;
    auto rel = std::filesystem::relative(path, base, ec);
    if (ec) {
        rel = path.lexically_relative(base);
    }
    auto text = genericPathString(rel);
    if (text == ".") {
        text.clear();
    }
    return text;
}

auto
normalizeSkinToken(QString token) -> QString
{
    token = token.trimmed();
    token.replace('\\', '/');
    while (token.startsWith(QStringLiteral("./"))) {
        token.remove(0, 2);
    }
    return token;
}

auto
resolveRawPath(const std::filesystem::path& base, const QString& token)
  -> std::filesystem::path
{
    const auto normalized = normalizeSkinToken(token);
    if (normalized.isEmpty()) {
        return {};
    }
    auto path = support::qStringToPath(normalized);
    if (path.is_relative()) {
        path = base / path;
    }
    return std::filesystem::absolute(path).lexically_normal();
}

auto
firstWildcardMatch(const std::filesystem::path& wildcardPath)
  -> std::filesystem::path
{
    const auto directory = wildcardPath.parent_path();
    const auto pattern = genericPathString(wildcardPath.filename());
    if (directory.empty() || pattern.isEmpty()) {
        return {};
    }

    QDir dir(support::pathToQString(directory));
    const auto matches =
      dir.entryList(QStringList{ pattern }, QDir::Files, QDir::Name);
    if (matches.isEmpty()) {
        return {};
    }
    return std::filesystem::absolute(directory / support::qStringToPath(matches.first()))
      .lexically_normal();
}

auto
selectedWildcardMatch(const std::filesystem::path& wildcardPath,
                      const QString& selected) -> std::filesystem::path
{
    if (selected.compare(QStringLiteral("Random"), Qt::CaseInsensitive) == 0) {
        return firstWildcardMatch(wildcardPath);
    }

    const auto directory = wildcardPath.parent_path();
    const auto pattern = genericPathString(wildcardPath.filename());
    if (directory.empty() || pattern.isEmpty()) {
        return {};
    }

    const auto selectedPath =
      support::qStringToPath(normalizeSkinToken(selected));
    if (selectedPath.is_absolute()) {
        const auto selectedName =
          support::pathToQString(selectedPath.filename());
        std::error_code ec;
        if (!std::filesystem::is_regular_file(selectedPath, ec) ||
            !QDir::match(pattern, selectedName)) {
            return {};
        }
        return std::filesystem::absolute(selectedPath).lexically_normal();
    }

    const auto selectedName =
      support::pathToQString(selectedPath.filename());
    if (selectedName.isEmpty()) {
        return {};
    }
    const auto selectedStem =
      support::pathToQString(selectedPath.stem());

    const auto exact = directory / selectedPath.filename();
    std::error_code ec;
    if (std::filesystem::is_regular_file(exact, ec) &&
        QDir::match(pattern, selectedName)) {
        return std::filesystem::absolute(exact).lexically_normal();
    }

    QDir dir(support::pathToQString(directory));
    const auto matches =
      dir.entryList(QStringList{ pattern }, QDir::Files, QDir::Name);
    for (const auto& match : matches) {
        if (match.compare(selectedName, Qt::CaseInsensitive) == 0) {
            return std::filesystem::absolute(directory /
                                             support::qStringToPath(match))
              .lexically_normal();
        }

        const auto baseName =
          support::pathToQString(support::qStringToPath(match).stem());
        if (baseName.compare(selectedName, Qt::CaseInsensitive) == 0 ||
            (!selectedStem.isEmpty() &&
             baseName.compare(selectedStem, Qt::CaseInsensitive) == 0)) {
            return std::filesystem::absolute(directory /
                                             support::qStringToPath(match))
              .lexically_normal();
        }
    }

    return {};
}

auto
pathContainsWildcard(const QString& text) -> bool
{
    return text.contains(QLatin1Char('*')) || text.contains(QLatin1Char('?'));
}

auto
selectedFilePath(const std::filesystem::path& base,
                 const FilePathOption& option,
                 const QVariantMap& settingValues) -> QString
{
    const auto pattern = resolveRawPath(base, option.path);
    const auto directory = pattern.parent_path();

    auto selected = settingValues.value(option.settingId).toString();
    if (selected.isEmpty()) {
        selected = option.def;
    }
    if (!selected.isEmpty()) {
        const bool selectedRandom =
          selected.compare(QStringLiteral("Random"), Qt::CaseInsensitive) == 0;
        if (pathContainsWildcard(option.path)) {
            const auto wildcardSelected = selectedWildcardMatch(pattern, selected);
            if (!wildcardSelected.empty()) {
                return genericPathString(wildcardSelected);
            }
            if (selectedRandom) {
                return genericPathString(pattern);
            }
            const auto wildcard = firstWildcardMatch(pattern);
            if (!wildcard.empty()) {
                return genericPathString(wildcard);
            }
            return genericPathString(pattern);
        }
        auto selectedPath = support::qStringToPath(normalizeSkinToken(selected));
        if (selectedPath.is_relative()) {
            selectedPath = directory / selectedPath.filename();
        }
        return genericPathString(std::filesystem::absolute(selectedPath)
                                   .lexically_normal());
    }

    const auto wildcard = firstWildcardMatch(pattern);
    if (!wildcard.empty()) {
        return genericPathString(wildcard);
    }
    return genericPathString(pattern);
}

auto
matchingFilePathOption(const QList<FilePathOption>& filePaths,
                       const QString& token) -> std::optional<FilePathOption>
{
    const auto normalized = normalizeSkinToken(token);
    for (const auto& filePath : filePaths) {
        if (normalizeSkinToken(filePath.path).compare(normalized,
                                                      Qt::CaseInsensitive) ==
            0) {
            return filePath;
        }
    }
    return std::nullopt;
}

auto
resolveSkinPath(const std::filesystem::path& base,
                const QList<FilePathOption>& filePaths,
                const QVariantMap& settingValues,
                const QString& token) -> QString
{
    const auto normalized = normalizeSkinToken(token);
    if (normalized.isEmpty()) {
        return {};
    }

    if (const auto filePath = matchingFilePathOption(filePaths, normalized)) {
        return selectedFilePath(base, *filePath, settingValues);
    }

    const auto absolute = resolveRawPath(base, normalized);
    if (pathContainsWildcard(genericPathString(absolute))) {
        const auto wildcard = firstWildcardMatch(absolute);
        if (!wildcard.empty()) {
            return genericPathString(wildcard);
        }
    }
    return genericPathString(absolute);
}

auto
selectedChoiceOption(const QVariantList& items,
                     const QVariant& selectedValue) -> std::optional<int>
{
    if (!selectedValue.isValid() || selectedValue.isNull()) {
        return std::nullopt;
    }

    const auto selectedName = selectedValue.toString();
    if (!selectedName.isEmpty()) {
        for (const auto& itemValue : items) {
            const auto item = asMap(itemValue);
            if (item.value(QStringLiteral("name")).toString() ==
                selectedName) {
                return toInt(item.value(QStringLiteral("op")));
            }
        }
    }

    const auto selectedOption = toExactInt(selectedValue);
    if (!selectedOption.has_value()) {
        return std::nullopt;
    }

    for (const auto& itemValue : items) {
        const auto item = asMap(itemValue);
        const auto itemOption = toExactInt(item.value(QStringLiteral("op")));
        if (itemOption.has_value() &&
            itemOption.value() == selectedOption.value()) {
            return itemOption.value();
        }
    }

    return std::nullopt;
}

auto
choiceDefaultValue(const QJsonArray& choicesArray,
                   const QVariant& rawDefault) -> QString
{
    if (choicesArray.isEmpty()) {
        return {};
    }

    const auto fallback = choicesArray.first().toObject()["value"].toString();
    const auto defaultText = rawDefault.toString();
    if (!defaultText.isEmpty()) {
        for (const auto& choiceValue : choicesArray) {
            const auto choice = choiceValue.toObject();
            if (choice["value"].toString() == defaultText) {
                return defaultText;
            }
        }
    }

    const auto defaultOption = toExactInt(rawDefault);
    if (!defaultOption.has_value()) {
        return fallback;
    }

    for (const auto& choiceValue : choicesArray) {
        const auto choice = choiceValue.toObject();
        if (choice["op"].toInt() == defaultOption.value()) {
            return choice["value"].toString();
        }
    }

    return fallback;
}

auto
filePathOptionsFromHeader(const QVariantMap& header) -> QList<FilePathOption>
{
    QList<FilePathOption> result;
    const auto entries = asList(header.value(QStringLiteral("filepath")));
    for (int i = 0; i < entries.size(); ++i) {
        const auto map = asMap(entries[i]);
        const auto name = map.value(QStringLiteral("name")).toString();
        const auto path = map.value(QStringLiteral("path")).toString();
        if (name.isEmpty() || path.isEmpty()) {
            continue;
        }
        result.append(FilePathOption{
          .name = name,
          .path = path,
          .def = map.value(QStringLiteral("def")).toString(),
          .settingId = makeSafeId(name, QStringLiteral("file_%1").arg(i)),
        });
    }
    return result;
}

auto
selectedOptionsFromHeader(const QVariantMap& header,
                          const QVariantMap& settingValues) -> QMap<QString, int>
{
    QMap<QString, int> result;
    const auto properties = asList(header.value(QStringLiteral("property")));
    for (int i = 0; i < properties.size(); ++i) {
        const auto property = asMap(properties[i]);
        const auto name = property.value(QStringLiteral("name")).toString();
        const auto items = asList(property.value(QStringLiteral("item")));
        if (name.isEmpty() || items.isEmpty()) {
            continue;
        }

        const auto settingId = makeSafeId(name, QStringLiteral("opt_%1").arg(i));
        auto selectedValue = settingValues.value(settingId);
        if (!selectedValue.isValid() || selectedValue.toString().isEmpty()) {
            selectedValue = property.value(QStringLiteral("def"));
        }
        if (const auto selected = selectedChoiceOption(items, selectedValue);
            selected.has_value()) {
            result[name] = selected.value();
            continue;
        }
        result[name] = toInt(asMap(items.first()).value(QStringLiteral("op")));
    }
    return result;
}

auto
buildSettingsData(const QVariantMap& header) -> QString
{
    const auto title = header.value(QStringLiteral("name")).toString();
    const auto maker = header.value(QStringLiteral("author")).toString();
    const auto typeId = toInt(header.value(QStringLiteral("type")), -1);
    QJsonArray itemsArray;

    const auto properties = asList(header.value(QStringLiteral("property")));
    for (int i = 0; i < properties.size(); ++i) {
        const auto property = asMap(properties[i]);
        const auto name = property.value(QStringLiteral("name")).toString();
        const auto choices = asList(property.value(QStringLiteral("item")));
        if (name.isEmpty() || choices.isEmpty()) {
            continue;
        }

        QJsonArray choicesArray;
        for (const auto& choiceValue : choices) {
            const auto choiceMap = asMap(choiceValue);
            const auto choiceName =
              choiceMap.value(QStringLiteral("name")).toString();
            if (choiceName.isEmpty()) {
                continue;
            }
            QJsonObject choice;
            choice["value"] = choiceName;
            QJsonObject choiceNameObj;
            choiceNameObj["en"] = choiceName;
            choice["name"] = choiceNameObj;
            choice["op"] = toInt(choiceMap.value(QStringLiteral("op")));
            choicesArray.append(choice);
        }
        if (choicesArray.isEmpty()) {
            continue;
        }

        QJsonObject item;
        item["type"] = "choice";
        item["id"] = makeSafeId(name, QStringLiteral("opt_%1").arg(i));
        QJsonObject nameObj;
        nameObj["en"] = name;
        item["name"] = nameObj;
        item["choices"] = choicesArray;

        item["default"] =
          choiceDefaultValue(choicesArray, property.value(QStringLiteral("def")));
        itemsArray.append(item);
    }

    const auto filePaths = asList(header.value(QStringLiteral("filepath")));
    for (int i = 0; i < filePaths.size(); ++i) {
        const auto filePath = asMap(filePaths[i]);
        const auto name = filePath.value(QStringLiteral("name")).toString();
        const auto path = filePath.value(QStringLiteral("path")).toString();
        if (name.isEmpty() || path.isEmpty()) {
            continue;
        }

        const auto absolutePattern =
          resolveRawPath(std::filesystem::current_path(), path);
        Q_UNUSED(absolutePattern);

        QJsonObject item;
        item["type"] = "file";
        item["id"] = makeSafeId(name, QStringLiteral("file_%1").arg(i));
        QJsonObject nameObj;
        nameObj["en"] = name;
        item["name"] = nameObj;

        auto defaultValue = filePath.value(QStringLiteral("def")).toString();
        if (!defaultValue.isEmpty()) {
            defaultValue =
              support::pathToQString(support::qStringToPath(defaultValue).filename());
            item["default"] = defaultValue;
        }
        item["path"] = QString{};
        itemsArray.append(item);
    }

    QJsonObject root;
    root["title"] = title;
    root["maker"] = maker;
    root["type"] = typeId;
    root["format"] = "beatoraja";
    root["sourceFormat"] = "lua";
    root["items"] = itemsArray;
    return QString::fromUtf8(
      QJsonDocument(root).toJson(QJsonDocument::Compact));
}

auto
buildSettingsData(const QVariantMap& header,
                  const std::filesystem::path& skinPath) -> QString
{
    auto data = QJsonDocument::fromJson(buildSettingsData(header).toUtf8())
                  .object();
    auto items = data["items"].toArray();

    int fileIndex = 0;
    const auto filePaths = asList(header.value(QStringLiteral("filepath")));
    for (int itemIndex = 0; itemIndex < items.size(); ++itemIndex) {
        auto item = items[itemIndex].toObject();
        if (item["type"].toString() != QStringLiteral("file")) {
            continue;
        }
        while (fileIndex < filePaths.size()) {
            const auto filePath = asMap(filePaths[fileIndex++]);
            const auto path = filePath.value(QStringLiteral("path")).toString();
            if (path.isEmpty()) {
                continue;
            }
            const auto absolutePattern =
              resolveRawPath(skinPath.parent_path(), path);
            auto rel = relativePathString(absolutePattern.parent_path(),
                                          skinPath.parent_path());
            item["path"] = rel;
            items[itemIndex] = item;
            break;
        }
    }

    data["items"] = items;
    return QString::fromUtf8(
      QJsonDocument(data).toJson(QJsonDocument::Compact));
}

auto
absoluteIndex(lua_State* lua, const int index) -> int
{
    return index > 0 || index <= LUA_REGISTRYINDEX ? index
                                                   : lua_gettop(lua) + index + 1;
}

auto
luaTableLength(lua_State* lua, const int index) -> std::size_t
{
#if LUA_VERSION_NUM >= 502
    return lua_rawlen(lua, index);
#else
    return lua_objlen(lua, index);
#endif
}

auto
luaValueToVariant(lua_State* lua,
                  int index,
                  BeatorajaLuaRuntime* runtime = nullptr,
                  const int depth = 0) -> QVariant
{
    if (depth > 64) {
        return {};
    }
    index = absoluteIndex(lua, index);

    switch (lua_type(lua, index)) {
        case LUA_TNIL:
        case LUA_TNONE:
            return {};
        case LUA_TBOOLEAN:
            return lua_toboolean(lua, index) != 0;
        case LUA_TNUMBER: {
            const double number = lua_tonumber(lua, index);
            if (std::isfinite(number) && std::floor(number) == number &&
                number >= std::numeric_limits<int>::min() &&
                number <= std::numeric_limits<int>::max()) {
                return static_cast<int>(number);
            }
            return number;
        }
        case LUA_TSTRING:
            return QString::fromUtf8(lua_tostring(lua, index));
        case LUA_TFUNCTION:
            if (runtime != nullptr) {
                QVariantMap callback;
                callback.insert(QString::fromLatin1(LuaCallbackVariantKey),
                                runtime->registerCallback(lua, index));
                return callback;
            }
            return {};
        case LUA_TTABLE:
            break;
        default:
            return {};
    }

    QMap<int, QVariant> arrayValues;
    QVariantMap mapValues;
    bool hasMapKeys = false;
    int maxArrayKey = 0;

    lua_pushnil(lua);
    while (lua_next(lua, index) != 0) {
        const auto value = luaValueToVariant(lua, -1, runtime, depth + 1);
        bool arrayKey = false;
        int numericKey = 0;
        if (lua_type(lua, -2) == LUA_TNUMBER) {
            const double key = lua_tonumber(lua, -2);
            if (std::isfinite(key) && std::floor(key) == key && key >= 1 &&
                key <= std::numeric_limits<int>::max()) {
                arrayKey = true;
                numericKey = static_cast<int>(key);
                maxArrayKey = std::max(maxArrayKey, numericKey);
                arrayValues[numericKey] = value;
            }
        }

        if (!arrayKey) {
            hasMapKeys = true;
            QString key;
            if (lua_type(lua, -2) == LUA_TSTRING) {
                key = QString::fromUtf8(lua_tostring(lua, -2));
            } else if (lua_type(lua, -2) == LUA_TNUMBER) {
                key = QString::number(lua_tonumber(lua, -2));
            }
            if (!key.isEmpty()) {
                mapValues.insert(key, value);
            }
        }
        lua_pop(lua, 1);
    }

    if (!hasMapKeys && maxArrayKey > 0 &&
        static_cast<std::size_t>(maxArrayKey) == luaTableLength(lua, index)) {
        QVariantList list;
        for (int i = 1; i <= maxArrayKey; ++i) {
            list.append(arrayValues.value(i));
        }
        return list;
    }

    for (auto it = arrayValues.cbegin(); it != arrayValues.cend(); ++it) {
        mapValues.insert(QString::number(it.key()), it.value());
    }
    return mapValues;
}

auto
preprocessLuaJitSource(const QByteArray& source) -> QByteArray
{
    auto text = QString::fromUtf8(source);
    text.replace(QRegularExpression(QStringLiteral(
                   R"((?m)^(\s*)local\s+([A-Z][A-Z0-9_]*)\s*=)")),
                 QStringLiteral(R"(\1\2 =)"));
    return text.toUtf8();
}

auto
loadLuaBuffer(lua_State* lua,
              const QByteArray& source,
              const QByteArray& chunkName) -> int
{
    int status = luaL_loadbuffer(lua,
                                 source.constData(),
                                 static_cast<std::size_t>(source.size()),
                                 chunkName.constData());
    if (status == 0) {
        return status;
    }

    const auto error = QString::fromUtf8(lua_tostring(lua, -1));
    if (!error.contains(QStringLiteral("more than 60 upvalues"))) {
        return status;
    }

    lua_pop(lua, 1);
    const auto preprocessed = preprocessLuaJitSource(source);
    return luaL_loadbuffer(lua,
                           preprocessed.constData(),
                           static_cast<std::size_t>(preprocessed.size()),
                           chunkName.constData());
}

void
luaPushQString(lua_State* lua, const QString& value)
{
    const auto bytes = value.toUtf8();
    lua_pushlstring(lua, bytes.constData(), static_cast<std::size_t>(bytes.size()));
}

void
luaPushVariant(lua_State* lua, const QVariant& value)
{
    if (!value.isValid() || value.isNull()) {
        lua_pushnil(lua);
        return;
    }
    if (value.metaType().id() == QMetaType::Bool) {
        lua_pushboolean(lua, value.toBool());
        return;
    }
    bool ok = false;
    const double number = value.toDouble(&ok);
    if (ok) {
        lua_pushnumber(lua, number);
        return;
    }
    luaPushQString(lua, value.toString());
}

auto
contextFromUpvalue(lua_State* lua) -> LuaExecutionContext*
{
    return static_cast<LuaExecutionContext*>(
      lua_touserdata(lua, lua_upvalueindex(1)));
}

int
luaReturnZero(lua_State* lua)
{
    lua_pushinteger(lua, 0);
    return 1;
}

int
luaReturnFalse(lua_State* lua)
{
    lua_pushboolean(lua, 0);
    return 1;
}

int
luaReturnTrue(lua_State* lua)
{
    lua_pushboolean(lua, 1);
    return 1;
}

int
luaReturnEmptyString(lua_State* lua)
{
    lua_pushliteral(lua, "");
    return 1;
}

int
luaReturnZeroFunction(lua_State* lua)
{
    lua_pushcfunction(lua, luaReturnZero);
    return 1;
}

int
luaReturnFalseFunction(lua_State* lua)
{
    lua_pushcfunction(lua, luaReturnFalse);
    return 1;
}

int
luaMainStateOption(lua_State* lua)
{
    auto* context = contextFromUpvalue(lua);
    const int option = static_cast<int>(luaL_optinteger(lua, 1, 0));
    if (context != nullptr && context->runtime != nullptr) {
        lua_pushboolean(lua,
                        context->runtime->providerOption(option, false));
        return 1;
    }
    const bool contains =
      context != nullptr && context->activeOptions.contains(std::abs(option));
    lua_pushboolean(lua, option < 0 ? !contains : contains);
    return 1;
}

int
luaMainStateText(lua_State* lua)
{
    auto* context = contextFromUpvalue(lua);
    const int id = static_cast<int>(luaL_optinteger(lua, 1, 0));
    const auto value = context != nullptr && context->runtime != nullptr
                         ? context->runtime->providerValue("beatorajaLuaTextValue",
                                                           id,
                                                           QString{})
                         : QVariant(QString{});
    luaPushQString(lua, value.toString());
    return 1;
}

int
luaMainStateNumber(lua_State* lua)
{
    auto* context = contextFromUpvalue(lua);
    const int id = static_cast<int>(luaL_optinteger(lua, 1, 0));
    const auto value = context != nullptr && context->runtime != nullptr
                         ? context->runtime->providerValue("beatorajaLuaNumberValue",
                                                           id,
                                                           0)
                         : QVariant(0);
    luaPushVariant(lua, value);
    return 1;
}

int
luaMainStateTimer(lua_State* lua)
{
    auto* context = contextFromUpvalue(lua);
    const int id = static_cast<int>(luaL_optinteger(lua, 1, 0));
    const auto value = context != nullptr && context->runtime != nullptr
                         ? context->runtime->providerValue("beatorajaLuaTimerValue",
                                                           id,
                                                           0)
                         : QVariant(0);
    luaPushVariant(lua, value);
    return 1;
}

int
luaMainStateSetTimer(lua_State* lua)
{
    auto* context = contextFromUpvalue(lua);
    if (context == nullptr || context->runtime == nullptr) {
        lua_pushboolean(lua, false);
        return 1;
    }

    const int id = static_cast<int>(luaL_optinteger(lua, 1, 0));
    const double value = luaL_optnumber(lua, 2, LuaTimerOffValue);
    context->runtime->setCustomTimerMicroValue(id, value);
    lua_pushboolean(lua, true);
    return 1;
}

int
luaMainStateEventExec(lua_State* lua)
{
    auto* context = contextFromUpvalue(lua);
    if (context == nullptr || context->runtime == nullptr) {
        lua_pushboolean(lua, false);
        return 1;
    }

    const int id = static_cast<int>(luaL_optinteger(lua, 1, 0));
    const int arg1 = static_cast<int>(luaL_optinteger(lua, 2, 0));
    const int arg2 = static_cast<int>(luaL_optinteger(lua, 3, 0));
    lua_pushboolean(lua, context->runtime->callEvent(id, arg1, arg2));
    return 1;
}

int
luaMainStateTime(lua_State* lua)
{
    auto* context = contextFromUpvalue(lua);
    const auto value = context != nullptr && context->runtime != nullptr
                         ? context->runtime->providerValue("beatorajaLuaTimeValue",
                                                           0,
                                                           0)
                         : QVariant(0);
    luaPushVariant(lua, value);
    return 1;
}

int
luaTimerUtilNowTimer(lua_State* lua)
{
    auto* context = contextFromUpvalue(lua);
    const double timerValue = luaL_optnumber(lua, 1, LuaTimerOffValue);
    if (BeatorajaLuaRuntime::timerIsOff(timerValue)) {
        lua_pushnumber(lua, 0.0);
        return 1;
    }
    const double now = context != nullptr && context->runtime != nullptr
                         ? context->runtime->currentTimeMicro()
                         : 0.0;
    lua_pushnumber(lua, std::max(0.0, now - timerValue));
    return 1;
}

int
luaTimerUtilIsTimerOn(lua_State* lua)
{
    lua_pushboolean(lua, !BeatorajaLuaRuntime::timerIsOff(
                           luaL_optnumber(lua, 1, LuaTimerOffValue)));
    return 1;
}

int
luaTimerUtilIsTimerOff(lua_State* lua)
{
    lua_pushboolean(lua, BeatorajaLuaRuntime::timerIsOff(
                           luaL_optnumber(lua, 1, LuaTimerOffValue)));
    return 1;
}

int
luaTimerFunctionClosure(lua_State* lua)
{
    auto* context = static_cast<LuaExecutionContext*>(
      lua_touserdata(lua, lua_upvalueindex(1)));
    const int id = static_cast<int>(lua_tointeger(lua, lua_upvalueindex(2)));
    const auto value = context != nullptr && context->runtime != nullptr
                         ? context->runtime->providerValue("beatorajaLuaTimerValue",
                                                           id,
                                                           LuaTimerOffValue)
                         : QVariant(LuaTimerOffValue);
    luaPushVariant(lua, value);
    return 1;
}

int
luaTimerUtilTimerFunction(lua_State* lua)
{
    auto* context = contextFromUpvalue(lua);
    const int id = static_cast<int>(luaL_optinteger(lua, 1, 0));
    lua_pushlightuserdata(lua, context);
    lua_pushinteger(lua, id);
    lua_pushcclosure(lua, luaTimerFunctionClosure, 2);
    return 1;
}

double
luaTimerStateValue(lua_State* lua, const int tableIndex)
{
    lua_getfield(lua, tableIndex, "timerValue");
    const double result = luaL_optnumber(lua, -1, LuaTimerOffValue);
    lua_pop(lua, 1);
    return result;
}

void
luaSetTimerStateValue(lua_State* lua, const int tableIndex, const double value)
{
    lua_pushnumber(lua, value);
    lua_setfield(lua, tableIndex, "timerValue");
}

int
luaTimerObserveBooleanClosure(lua_State* lua)
{
    auto* context = static_cast<LuaExecutionContext*>(
      lua_touserdata(lua, lua_upvalueindex(1)));
    lua_pushvalue(lua, lua_upvalueindex(2));
    bool on = false;
    if (lua_pcall(lua, 0, 1, 0) == 0) {
        on = lua_toboolean(lua, -1);
        lua_pop(lua, 1);
    } else {
        spdlog::warn("Failed to execute timer_observe_boolean callback: {}",
                     lua_tostring(lua, -1));
        lua_pop(lua, 1);
    }

    const int stateIndex = lua_upvalueindex(3);
    double value = luaTimerStateValue(lua, stateIndex);
    if (on && BeatorajaLuaRuntime::timerIsOff(value)) {
        value = context != nullptr && context->runtime != nullptr
                  ? context->runtime->currentTimeMicro()
                  : 0.0;
        luaSetTimerStateValue(lua, stateIndex, value);
    } else if (!on && !BeatorajaLuaRuntime::timerIsOff(value)) {
        value = LuaTimerOffValue;
        luaSetTimerStateValue(lua, stateIndex, value);
    }
    lua_pushnumber(lua, value);
    return 1;
}

int
luaTimerUtilTimerObserveBoolean(lua_State* lua)
{
    auto* context = contextFromUpvalue(lua);
    if (!lua_isfunction(lua, 1)) {
        lua_pushcfunction(lua, luaReturnZero);
        return 1;
    }
    lua_pushlightuserdata(lua, context);
    lua_pushvalue(lua, 1);
    lua_newtable(lua);
    luaSetTimerStateValue(lua, -1, LuaTimerOffValue);
    lua_pushcclosure(lua, luaTimerObserveBooleanClosure, 3);
    return 1;
}

int
luaPassiveTimerClosure(lua_State* lua)
{
    lua_pushnumber(lua, luaTimerStateValue(lua, lua_upvalueindex(1)));
    return 1;
}

int
luaPassiveTimerTurnOnClosure(lua_State* lua)
{
    auto* context = static_cast<LuaExecutionContext*>(
      lua_touserdata(lua, lua_upvalueindex(1)));
    const int stateIndex = lua_upvalueindex(2);
    const double value = luaTimerStateValue(lua, stateIndex);
    if (BeatorajaLuaRuntime::timerIsOff(value)) {
        luaSetTimerStateValue(
          lua,
          stateIndex,
          context != nullptr && context->runtime != nullptr
            ? context->runtime->currentTimeMicro()
            : 0.0);
    }
    lua_pushboolean(lua, true);
    return 1;
}

int
luaPassiveTimerTurnOnResetClosure(lua_State* lua)
{
    auto* context = static_cast<LuaExecutionContext*>(
      lua_touserdata(lua, lua_upvalueindex(1)));
    luaSetTimerStateValue(
      lua,
      lua_upvalueindex(2),
      context != nullptr && context->runtime != nullptr
        ? context->runtime->currentTimeMicro()
        : 0.0);
    lua_pushboolean(lua, true);
    return 1;
}

int
luaPassiveTimerTurnOffClosure(lua_State* lua)
{
    luaSetTimerStateValue(lua, lua_upvalueindex(1), LuaTimerOffValue);
    lua_pushboolean(lua, true);
    return 1;
}

int
luaTimerUtilNewPassiveTimer(lua_State* lua)
{
    auto* context = contextFromUpvalue(lua);
    lua_newtable(lua);
    const int resultIndex = absoluteIndex(lua, -1);

    lua_newtable(lua);
    luaSetTimerStateValue(lua, -1, LuaTimerOffValue);
    const int stateIndex = absoluteIndex(lua, -1);

    lua_pushvalue(lua, stateIndex);
    lua_pushcclosure(lua, luaPassiveTimerClosure, 1);
    lua_setfield(lua, resultIndex, "timer");

    lua_pushlightuserdata(lua, context);
    lua_pushvalue(lua, stateIndex);
    lua_pushcclosure(lua, luaPassiveTimerTurnOnClosure, 2);
    lua_setfield(lua, resultIndex, "turn_on");

    lua_pushlightuserdata(lua, context);
    lua_pushvalue(lua, stateIndex);
    lua_pushcclosure(lua, luaPassiveTimerTurnOnResetClosure, 2);
    lua_setfield(lua, resultIndex, "turn_on_reset");

    lua_pushvalue(lua, stateIndex);
    lua_pushcclosure(lua, luaPassiveTimerTurnOffClosure, 1);
    lua_setfield(lua, resultIndex, "turn_off");

    lua_pop(lua, 1);
    return 1;
}

int
luaSkinConfigGetPath(lua_State* lua)
{
    const auto* context = contextFromUpvalue(lua);
    const auto token = QString::fromUtf8(luaL_optstring(lua, 1, ""));
    if (context == nullptr) {
        luaPushQString(lua, token);
        return 1;
    }

    luaPushQString(lua,
                   resolveSkinPath(context->skinPath.parent_path(),
                                   context->filePaths,
                                   context->settingValues,
                                   token));
    return 1;
}

int
luaOffsetIndex(lua_State* lua)
{
    lua_newtable(lua);
    for (const auto* field : { "x", "y", "w", "h", "r", "a" }) {
        lua_pushinteger(lua, 0);
        lua_setfield(lua, -2, field);
    }
    lua_pushvalue(lua, 2);
    lua_pushvalue(lua, -2);
    lua_rawset(lua, 1);
    return 1;
}

using LuaCFunction = int (*)(lua_State*);

void
setFunctionField(lua_State* lua,
                 const char* name,
                 LuaCFunction function,
                 LuaExecutionContext* context = nullptr);

void
setFallbackMetatable(lua_State* lua, LuaCFunction fallback);

void
pushGenericJavaObject(lua_State* lua);

void
applyBeatorajaLuaDefaultOptions(std::set<int>& options);

int
luaJavaObjectFallback(lua_State* lua)
{
    lua_pushcfunction(lua, [](lua_State* inner) -> int {
        if (lua_gettop(inner) > 0) {
            pushGenericJavaObject(inner);
            return 1;
        }
        lua_pushnil(inner);
        return 1;
    });
    return 1;
}

int
luaJavaFileToString(lua_State* lua)
{
    lua_getfield(lua, 1, "__path");
    return 1;
}

int
luaJavaFileMkdir(lua_State* lua)
{
    lua_pushboolean(lua, 1);
    return 1;
}

int
luaJavaFileListFiles(lua_State* lua)
{
    lua_getfield(lua, 1, "__path");
    const auto path = QString::fromUtf8(lua_tostring(lua, -1));
    lua_pop(lua, 1);

    QDir dir(path);
    lua_newtable(lua);
    int index = 1;
    for (const auto& entry : dir.entryInfoList(QDir::Files | QDir::Dirs |
                                                 QDir::NoDotAndDotDot,
                                               QDir::Name)) {
        luaPushQString(lua, entry.absoluteFilePath().replace('\\', '/'));
        lua_rawseti(lua, -2, index++);
    }
    return 1;
}

auto
luaTableStringField(lua_State* lua, int index, const char* field) -> QString
{
    index = absoluteIndex(lua, index);
    lua_getfield(lua, index, field);
    const auto result =
      QString::fromUtf8(luaL_optstring(lua, -1, ""));
    lua_pop(lua, 1);
    return result;
}

auto
luaTableIntegerField(lua_State* lua,
                     int index,
                     const char* field,
                     const int fallback = 0) -> int
{
    index = absoluteIndex(lua, index);
    lua_getfield(lua, index, field);
    const auto result = static_cast<int>(luaL_optinteger(lua, -1, fallback));
    lua_pop(lua, 1);
    return result;
}

auto
luaTableByteArrayField(lua_State* lua, int index, const char* field)
  -> QByteArray
{
    index = absoluteIndex(lua, index);
    lua_getfield(lua, index, field);
    std::size_t length = 0;
    const auto* bytes = lua_tolstring(lua, -1, &length);
    auto result = QByteArray{};
    if (bytes != nullptr) {
        result = QByteArray(bytes, static_cast<qsizetype>(length));
    }
    lua_pop(lua, 1);
    return result;
}

void
luaSetTableByteArrayField(lua_State* lua,
                          const int index,
                          const char* field,
                          const QByteArray& value)
{
    lua_pushlstring(lua,
                    value.constData(),
                    static_cast<std::size_t>(value.size()));
    lua_setfield(lua, index, field);
}

struct LuaHttpResponse
{
    int status = 0;
    QByteArray body;
    QString error;
};

auto
executeLuaHttpRequest(const QString& urlText,
                      const QString& methodText,
                      const int timeoutMs) -> LuaHttpResponse
{
    if (QCoreApplication::instance() == nullptr) {
        return { .error = QStringLiteral("Qt application is not available") };
    }

    const auto url = QUrl(urlText);
    if (!url.isValid() || url.scheme().isEmpty()) {
        return { .error = QStringLiteral("invalid URL: %1").arg(urlText) };
    }

    auto timeout = timeoutMs <= 0 ? 5000 : timeoutMs;
    timeout = std::clamp(timeout, 1, 30000);

    QNetworkAccessManager manager;
    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "RhythmGame Lua Skin Loader");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);

    const auto method = methodText.isEmpty()
                          ? QByteArrayLiteral("GET")
                          : methodText.toUpper().toUtf8();
    QNetworkReply* reply = method == QByteArrayLiteral("GET")
                             ? manager.get(request)
                             : manager.sendCustomRequest(request, method);

    QEventLoop loop;
    QTimer timer;
    bool timedOut = false;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &loop, [&] {
        timedOut = true;
        reply->abort();
        loop.quit();
    });
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

    timer.start(timeout);
    loop.exec();
    if (timer.isActive()) {
        timer.stop();
    }

    bool hasStatus = false;
    const auto status =
      reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(
        &hasStatus);
    const auto body = reply->isOpen() ? reply->readAll() : QByteArray{};
    const auto replyError = reply->error();
    const auto errorString = reply->errorString();
    reply->deleteLater();

    if (timedOut) {
        return {
          .status = hasStatus ? status : 0,
          .body = body,
          .error = QStringLiteral("HTTP request timed out after %1ms")
                     .arg(timeout),
        };
    }
    if (!hasStatus && replyError != QNetworkReply::NoError) {
        return { .body = body, .error = errorString };
    }

    return { .status = hasStatus ? status : 0, .body = body };
}

int
luaJavaHttpConnectionSetRequestMethod(lua_State* lua)
{
    const auto method = QString::fromUtf8(luaL_optstring(lua, 2, "GET"));
    luaPushQString(lua, method);
    lua_setfield(lua, 1, "__request_method");
    return 0;
}

int
luaJavaHttpConnectionSetConnectTimeout(lua_State* lua)
{
    const auto timeout = static_cast<int>(luaL_optinteger(lua, 2, 0));
    lua_pushinteger(lua, timeout);
    lua_setfield(lua, 1, "__connect_timeout");
    return 0;
}

int
luaJavaHttpConnectionConnect(lua_State* lua)
{
    const auto url = luaTableStringField(lua, 1, "__url");
    const auto method = luaTableStringField(lua, 1, "__request_method");
    const auto timeout = luaTableIntegerField(lua, 1, "__connect_timeout", 5000);
    const auto response = executeLuaHttpRequest(url, method, timeout);

    lua_pushinteger(lua, response.status);
    lua_setfield(lua, 1, "__response_code");
    luaSetTableByteArrayField(lua, 1, "__response_body", response.body);
    lua_pushinteger(lua, response.error.isEmpty() ? 1 : 0);
    lua_setfield(lua, 1, "__connected");

    if (!response.error.isEmpty()) {
        const auto errorBytes = response.error.toUtf8();
        return luaL_error(lua, "%s", errorBytes.constData());
    }
    return 0;
}

int
luaJavaHttpConnectionResponseCode(lua_State* lua)
{
    lua_pushinteger(lua, luaTableIntegerField(lua, 1, "__response_code", 0));
    return 1;
}

void
pushJavaInputStreamObject(lua_State* lua, const QByteArray& body)
{
    lua_newtable(lua);
    luaSetTableByteArrayField(lua, -2, "__body", body);
    setFallbackMetatable(lua, luaJavaObjectFallback);
}

int
luaJavaHttpConnectionInputStream(lua_State* lua)
{
    const auto connected = luaTableIntegerField(lua, 1, "__connected", 0) != 0;
    if (!connected) {
        luaJavaHttpConnectionConnect(lua);
    }
    pushJavaInputStreamObject(lua,
                              luaTableByteArrayField(lua, 1, "__response_body"));
    return 1;
}

void
pushJavaHttpConnectionObject(lua_State* lua, const QString& url)
{
    lua_newtable(lua);
    luaPushQString(lua, url);
    lua_setfield(lua, -2, "__url");
    lua_pushinteger(lua, 5000);
    lua_setfield(lua, -2, "__connect_timeout");
    setFunctionField(lua, "setRequestMethod", luaJavaHttpConnectionSetRequestMethod);
    setFunctionField(lua, "setConnectTimeout", luaJavaHttpConnectionSetConnectTimeout);
    setFunctionField(lua, "connect", luaJavaHttpConnectionConnect);
    setFunctionField(lua, "getResponseCode", luaJavaHttpConnectionResponseCode);
    setFunctionField(lua, "getInputStream", luaJavaHttpConnectionInputStream);
    setFallbackMetatable(lua, luaJavaObjectFallback);
}

int
luaJavaUrlOpenConnection(lua_State* lua)
{
    pushJavaHttpConnectionObject(lua, luaTableStringField(lua, 1, "__url"));
    return 1;
}

void
pushJavaUrlObject(lua_State* lua, const QString& url)
{
    lua_newtable(lua);
    luaPushQString(lua, url);
    lua_setfield(lua, -2, "__url");
    setFunctionField(lua, "openConnection", luaJavaUrlOpenConnection);
    setFallbackMetatable(lua, luaJavaObjectFallback);
}

void
pushJavaInputStreamReaderObject(lua_State* lua, const QByteArray& body)
{
    lua_newtable(lua);
    luaSetTableByteArrayField(lua, -2, "__body", body);
    setFallbackMetatable(lua, luaJavaObjectFallback);
}

int
luaJavaBufferedReaderReadLine(lua_State* lua)
{
    const auto self = absoluteIndex(lua, 1);
    const auto lineIndex =
      luaTableIntegerField(lua, self, "__line_index", 1);

    lua_getfield(lua, self, "__lines");
    if (!lua_istable(lua, -1)) {
        lua_pop(lua, 1);
        lua_pushnil(lua);
        return 1;
    }

    lua_rawgeti(lua, -1, lineIndex);
    if (lua_isnil(lua, -1)) {
        lua_pop(lua, 2);
        lua_pushnil(lua);
        return 1;
    }

    lua_pushinteger(lua, lineIndex + 1);
    lua_setfield(lua, self, "__line_index");
    lua_remove(lua, -2);
    return 1;
}

void
pushJavaBufferedReaderObject(lua_State* lua, const QByteArray& body)
{
    lua_newtable(lua);
    const auto readerIndex = absoluteIndex(lua, -1);

    lua_newtable(lua);
    int lineNumber = 1;
    qsizetype start = 0;
    while (start < body.size()) {
        const auto newline = body.indexOf('\n', start);
        const auto end = newline < 0 ? body.size() : newline;
        auto line = body.mid(start, end - start);
        if (line.endsWith('\r')) {
            line.chop(1);
        }
        lua_pushlstring(lua,
                        line.constData(),
                        static_cast<std::size_t>(line.size()));
        lua_rawseti(lua, -2, lineNumber++);
        if (newline < 0) {
            break;
        }
        start = newline + 1;
    }
    lua_setfield(lua, readerIndex, "__lines");

    lua_pushinteger(lua, 1);
    lua_setfield(lua, readerIndex, "__line_index");
    setFunctionField(lua, "readLine", luaJavaBufferedReaderReadLine);
    setFallbackMetatable(lua, luaJavaObjectFallback);
}

auto
javaBodyFromArgument(lua_State* lua, const int index) -> QByteArray
{
    if (lua_istable(lua, index)) {
        return luaTableByteArrayField(lua, index, "__body");
    }
    if (lua_isstring(lua, index)) {
        std::size_t length = 0;
        const auto* bytes = lua_tolstring(lua, index, &length);
        return QByteArray(bytes, static_cast<qsizetype>(length));
    }
    return {};
}

int
luaJavaThreadStart(lua_State* lua)
{
    const auto self = absoluteIndex(lua, 1);
    lua_getfield(lua, self, "__runnable");
    if (lua_istable(lua, -1)) {
        lua_getfield(lua, -1, "run");
        if (lua_isfunction(lua, -1)) {
            if (lua_pcall(lua, 0, 0, 0) != 0) {
                return lua_error(lua);
            }
            lua_pop(lua, 1);
            return 0;
        }
        lua_pop(lua, 2);
        return 0;
    }
    if (lua_isfunction(lua, -1)) {
        if (lua_pcall(lua, 0, 0, 0) != 0) {
            return lua_error(lua);
        }
        return 0;
    }
    lua_pop(lua, 1);
    return 0;
}

void
pushJavaThreadObject(lua_State* lua, const int runnableIndex)
{
    lua_newtable(lua);
    if (lua_istable(lua, runnableIndex) || lua_isfunction(lua, runnableIndex)) {
        lua_pushvalue(lua, runnableIndex);
        lua_setfield(lua, -2, "__runnable");
    }
    setFunctionField(lua, "start", luaJavaThreadStart);
    setFallbackMetatable(lua, luaJavaObjectFallback);
}

void
pushGenericJavaObject(lua_State* lua)
{
    lua_newtable(lua);
    setFallbackMetatable(lua, luaJavaObjectFallback);
}

int
luaJavaInputArgument(lua_State* lua)
{
    const int argIndex = lua_istable(lua, 1) && lua_gettop(lua) >= 2 ? 2 : 1;
    return static_cast<int>(luaL_optinteger(lua, argIndex, 0));
}

int
luaJavaInputIsKeyPressed(lua_State* lua)
{
    auto* context = contextFromUpvalue(lua);
    const int keyCode = luaJavaInputArgument(lua);
    const bool pressed = context != nullptr && context->runtime != nullptr
                           ? context->runtime
                               ->providerValue("beatorajaLuaKeyPressed",
                                               keyCode,
                                               false)
                               .toBool()
                           : false;
    lua_pushboolean(lua, pressed);
    return 1;
}

int
luaJavaInputIsKeyJustPressed(lua_State* lua)
{
    auto* context = contextFromUpvalue(lua);
    const int keyCode = luaJavaInputArgument(lua);
    const bool pressed = context != nullptr && context->runtime != nullptr
                           ? context->runtime
                               ->providerValue("beatorajaLuaKeyJustPressed",
                                               keyCode,
                                               false)
                               .toBool()
                           : false;
    lua_pushboolean(lua, pressed);
    return 1;
}

int
luaJavaInputIsButtonPressed(lua_State* lua)
{
    auto* context = contextFromUpvalue(lua);
    const int button = luaJavaInputArgument(lua);
    const bool pressed = context != nullptr && context->runtime != nullptr
                           ? context->runtime
                               ->providerValue("beatorajaLuaMouseButtonPressed",
                                               button,
                                               false)
                               .toBool()
                           : false;
    lua_pushboolean(lua, pressed);
    return 1;
}

int
luaJavaInputValue(lua_State* lua, const char* providerMethod)
{
    auto* context = contextFromUpvalue(lua);
    const auto value = context != nullptr && context->runtime != nullptr
                         ? context->runtime->providerValue(providerMethod, 0, 0)
                         : QVariant(0);
    luaPushVariant(lua, value);
    return 1;
}

int
luaJavaInputDeltaX(lua_State* lua)
{
    return luaJavaInputValue(lua, "beatorajaLuaMouseDeltaX");
}

int
luaJavaInputDeltaY(lua_State* lua)
{
    return luaJavaInputValue(lua, "beatorajaLuaMouseDeltaY");
}

int
luaJavaInputX(lua_State* lua)
{
    return luaJavaInputValue(lua, "beatorajaLuaMouseX");
}

int
luaJavaInputY(lua_State* lua)
{
    return luaJavaInputValue(lua, "beatorajaLuaMouseY");
}

void
setIntegerField(lua_State* lua, const char* name, const int value)
{
    lua_pushinteger(lua, value);
    lua_setfield(lua, -2, name);
}

void
pushLibGdxInputObject(lua_State* lua, LuaExecutionContext* context)
{
    lua_newtable(lua);
    setFunctionField(lua, "isKeyPressed", luaJavaInputIsKeyPressed, context);
    setFunctionField(lua, "isKeyJustPressed", luaJavaInputIsKeyJustPressed, context);
    setFunctionField(lua, "isButtonPressed", luaJavaInputIsButtonPressed, context);
    setFunctionField(lua, "getDeltaX", luaJavaInputDeltaX, context);
    setFunctionField(lua, "getDeltaY", luaJavaInputDeltaY, context);
    setFunctionField(lua, "getX", luaJavaInputX, context);
    setFunctionField(lua, "getY", luaJavaInputY, context);
    setFallbackMetatable(lua, luaJavaObjectFallback);
}

void
pushLibGdxInputClassObject(lua_State* lua)
{
    lua_newtable(lua);

    lua_newtable(lua);
    setIntegerField(lua, "UP", 19);
    setIntegerField(lua, "DOWN", 20);
    setIntegerField(lua, "LEFT", 21);
    setIntegerField(lua, "RIGHT", 22);
    setIntegerField(lua, "Enter", 66);
    setIntegerField(lua, "ENTER", 66);
    lua_setfield(lua, -2, "Keys");

    lua_newtable(lua);
    setIntegerField(lua, "LEFT", 0);
    setIntegerField(lua, "RIGHT", 1);
    setIntegerField(lua, "MIDDLE", 2);
    lua_setfield(lua, -2, "Buttons");

    setFallbackMetatable(lua, luaJavaObjectFallback);
}

void
pushLibGdxClassObject(lua_State* lua, LuaExecutionContext* context)
{
    lua_newtable(lua);
    pushLibGdxInputObject(lua, context);
    lua_setfield(lua, -2, "input");
    setFallbackMetatable(lua, luaJavaObjectFallback);
}

void
pushJavaFileObject(lua_State* lua, const QString& path)
{
    lua_newtable(lua);
    luaPushQString(lua, path);
    lua_setfield(lua, -2, "__path");
    setFunctionField(lua, "mkdir", luaJavaFileMkdir);
    setFunctionField(lua, "listFiles", luaJavaFileListFiles);

    lua_newtable(lua);
    lua_pushcfunction(lua, luaJavaFileToString);
    lua_setfield(lua, -2, "__tostring");
    lua_pushvalue(lua, -2);
    lua_setfield(lua, -2, "__index");
    lua_setmetatable(lua, -2);
}

int
luaJavaDesktopBrowse(lua_State* lua)
{
    QString url = luaTableStringField(lua, 2, "__url");
    if (url.isEmpty() && lua_isstring(lua, 2)) {
        url = QString::fromUtf8(lua_tostring(lua, 2));
    }
    if (!url.isEmpty()) {
        QDesktopServices::openUrl(QUrl(url));
    }
    return 0;
}

void
pushJavaDesktopObject(lua_State* lua)
{
    lua_newtable(lua);
    setFunctionField(lua, "browse", luaJavaDesktopBrowse);
    setFallbackMetatable(lua, luaJavaObjectFallback);
}

int
luaJavaDesktopGetDesktop(lua_State* lua)
{
    pushJavaDesktopObject(lua);
    return 1;
}

void
pushJavaDesktopClassObject(lua_State* lua)
{
    lua_newtable(lua);
    setFunctionField(lua, "getDesktop", luaJavaDesktopGetDesktop);
    setFallbackMetatable(lua, luaJavaObjectFallback);
}

int
luaLuajavaBindClass(lua_State* lua)
{
    auto* context = contextFromUpvalue(lua);
    const auto className = QString::fromUtf8(luaL_optstring(lua, 1, ""));
    if (className == QStringLiteral("java.awt.Desktop")) {
        pushJavaDesktopClassObject(lua);
        return 1;
    }
    if (className == QStringLiteral("com.badlogic.gdx.Gdx")) {
        pushLibGdxClassObject(lua, context);
        return 1;
    }
    if (className == QStringLiteral("com.badlogic.gdx.Input")) {
        pushLibGdxInputClassObject(lua);
        return 1;
    }
    lua_pushvalue(lua, 1);
    return 1;
}

int
luaLuajavaNew(lua_State* lua)
{
    const auto className = QString::fromUtf8(luaL_optstring(lua, 1, ""));
    if (className == QStringLiteral("java.net.URL")) {
        pushJavaUrlObject(lua, QString::fromUtf8(luaL_optstring(lua, 2, "")));
        return 1;
    }
    if (className == QStringLiteral("java.net.URI")) {
        pushJavaUrlObject(lua, QString::fromUtf8(luaL_optstring(lua, 2, "")));
        return 1;
    }
    if (className == QStringLiteral("java.lang.Thread")) {
        pushJavaThreadObject(lua, 2);
        return 1;
    }
    if (className == QStringLiteral("java.io.InputStreamReader")) {
        pushJavaInputStreamReaderObject(lua, javaBodyFromArgument(lua, 2));
        return 1;
    }
    if (className == QStringLiteral("java.io.BufferedReader")) {
        pushJavaBufferedReaderObject(lua, javaBodyFromArgument(lua, 2));
        return 1;
    }
    if (className == QStringLiteral("java.io.File")) {
        pushJavaFileObject(lua, QString::fromUtf8(luaL_optstring(lua, 2, "")));
        return 1;
    }
    pushGenericJavaObject(lua);
    return 1;
}

int
luaLuajavaNewInstance(lua_State* lua)
{
    const auto className = QString::fromUtf8(luaL_optstring(lua, 1, ""));
    if (className == QStringLiteral("java.net.URL")) {
        pushJavaUrlObject(lua, QString::fromUtf8(luaL_optstring(lua, 2, "")));
        return 1;
    }
    if (className == QStringLiteral("java.net.URI")) {
        pushJavaUrlObject(lua, QString::fromUtf8(luaL_optstring(lua, 2, "")));
        return 1;
    }
    if (className == QStringLiteral("java.lang.Thread")) {
        pushJavaThreadObject(lua, 2);
        return 1;
    }
    if (className == QStringLiteral("java.io.InputStreamReader")) {
        pushJavaInputStreamReaderObject(lua, javaBodyFromArgument(lua, 2));
        return 1;
    }
    if (className == QStringLiteral("java.io.BufferedReader")) {
        pushJavaBufferedReaderObject(lua, javaBodyFromArgument(lua, 2));
        return 1;
    }
    if (className == QStringLiteral("java.io.File")) {
        pushJavaFileObject(lua, QString::fromUtf8(luaL_optstring(lua, 2, "")));
        return 1;
    }
    pushGenericJavaObject(lua);
    return 1;
}

int
luaLuajavaCreateProxy(lua_State* lua)
{
    if (lua_istable(lua, 2)) {
        lua_pushvalue(lua, 2);
        return 1;
    }
    pushGenericJavaObject(lua);
    return 1;
}

auto
readLuaFile(const std::filesystem::path& path) -> QByteArray
{
    QFile file(support::pathToQString(path));
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    return file.readAll();
}

auto
luaModuleCandidates(const LuaExecutionContext& context, const QString& module)
  -> QList<std::filesystem::path>
{
    auto modulePath = module;
    modulePath.replace('.', '/');
    modulePath.replace('\\', '/');
    const auto relative = support::qStringToPath(modulePath);
    const auto base = context.skinPath.parent_path();
    auto luaFile = base / relative;
    luaFile += ".lua";
    auto luaSkinFile = base / relative;
    luaSkinFile += ".luaskin";
    return { luaFile, base / relative / "init.lua", luaSkinFile };
}

int
luaSkinModuleLoader(lua_State* lua)
{
    const auto* context = contextFromUpvalue(lua);
    const auto module = QString::fromUtf8(luaL_checkstring(lua, 1));
    if (context == nullptr) {
        lua_pushliteral(lua, "\n\tmissing RhythmGame Lua skin loader context");
        return 1;
    }

    for (const auto& candidate : luaModuleCandidates(*context, module)) {
        std::error_code ec;
        if (!std::filesystem::is_regular_file(candidate, ec)) {
            continue;
        }

        const auto source = readLuaFile(candidate);
        const auto chunkName = QByteArray("@") + genericPathString(candidate).toUtf8();
        if (loadLuaBuffer(lua, source, chunkName) != 0) {
            return lua_error(lua);
        }
        return 1;
    }

    luaPushQString(lua,
                   QStringLiteral("\n\tno RhythmGame Lua skin module '%1'")
                     .arg(module));
    return 1;
}

int
luaSkinDofile(lua_State* lua)
{
    const auto path = support::qStringToPath(
      QString::fromUtf8(luaL_checkstring(lua, 1)));
    const auto source = readLuaFile(path);
    if (source.isEmpty()) {
        return luaL_error(lua, "could not open %s", path.string().c_str());
    }

    lua_settop(lua, 0);
    const auto chunkName = QByteArray("@") + genericPathString(path).toUtf8();
    if (loadLuaBuffer(lua, source, chunkName) != 0) {
        return lua_error(lua);
    }
    if (lua_pcall(lua, 0, LUA_MULTRET, 0) != 0) {
        return lua_error(lua);
    }
    return lua_gettop(lua);
}

void
setFunctionField(lua_State* lua,
                 const char* name,
                 LuaCFunction function,
                 LuaExecutionContext* context)
{
    if (context != nullptr) {
        lua_pushlightuserdata(lua, context);
        lua_pushcclosure(lua, function, 1);
    } else {
        lua_pushcfunction(lua, function);
    }
    lua_setfield(lua, -2, name);
}

void
setFallbackMetatable(lua_State* lua, LuaCFunction fallback)
{
    lua_newtable(lua);
    lua_pushcfunction(lua, fallback);
    lua_setfield(lua, -2, "__index");
    lua_setmetatable(lua, -2);
}

void
registerLoadedModule(lua_State* lua,
                     const char* name,
                     LuaExecutionContext* context,
                     const QList<std::pair<const char*, LuaCFunction>>& fields,
                     LuaCFunction fallback)
{
    lua_getglobal(lua, "package");
    lua_getfield(lua, -1, "loaded");
    lua_newtable(lua);
    for (const auto& [fieldName, function] : fields) {
        setFunctionField(lua, fieldName, function, context);
    }
    setFallbackMetatable(lua, fallback);
    lua_pushvalue(lua, -1);
    lua_setfield(lua, -3, name);
    lua_setglobal(lua, name);
    lua_pop(lua, 2);
}

void
installSkinLuaLoader(lua_State* lua, LuaExecutionContext& context)
{
    lua_getglobal(lua, "package");
    lua_getfield(lua, -1, "loaders");
    if (!lua_istable(lua, -1)) {
        lua_pop(lua, 1);
        lua_getfield(lua, -1, "searchers");
    }
    if (lua_istable(lua, -1)) {
        const int loadersIndex = absoluteIndex(lua, -1);
        const int loaderCount = static_cast<int>(luaTableLength(lua, loadersIndex));
        for (int i = loaderCount + 1; i > 2; --i) {
            lua_rawgeti(lua, loadersIndex, i - 1);
            lua_rawseti(lua, loadersIndex, i);
        }
        lua_pushlightuserdata(lua, &context);
        lua_pushcclosure(lua, luaSkinModuleLoader, 1);
        lua_rawseti(lua, loadersIndex, 2);
    }
    lua_pop(lua, 2);

    lua_pushlightuserdata(lua, &context);
    lua_pushcclosure(lua, luaSkinDofile, 1);
    lua_setglobal(lua, "dofile");
}

void
pushOffsetTable(lua_State* lua)
{
    lua_newtable(lua);
    for (const auto* field : { "x", "y", "w", "h", "r", "a" }) {
        lua_pushinteger(lua, 0);
        lua_setfield(lua, -2, field);
    }
}

void
installSkinConfig(lua_State* lua, LuaExecutionContext& context)
{
    lua_newtable(lua);

    lua_newtable(lua);
    for (auto it = context.selectedOptions.cbegin();
         it != context.selectedOptions.cend();
         ++it) {
        lua_pushinteger(lua, it.value());
        lua_setfield(lua, -2, it.key().toUtf8().constData());
    }
    lua_setfield(lua, -2, "option");

    lua_newtable(lua);
    int optionIndex = 1;
    for (const int option : context.activeOptions) {
        lua_pushinteger(lua, option);
        lua_rawseti(lua, -2, optionIndex++);
    }
    lua_setfield(lua, -2, "enabled_options");

    lua_newtable(lua);
    for (const auto& filePath : context.filePaths) {
        luaPushQString(lua,
                       selectedFilePath(context.skinPath.parent_path(),
                                        filePath,
                                        context.settingValues));
        lua_setfield(lua, -2, filePath.name.toUtf8().constData());
    }
    lua_setfield(lua, -2, "file_path");

    lua_newtable(lua);
    const auto offsets = asList(context.header.value(QStringLiteral("offset")));
    for (const auto& offsetValue : offsets) {
        const auto offset = asMap(offsetValue);
        const auto name = offset.value(QStringLiteral("name")).toString();
        if (name.isEmpty()) {
            continue;
        }
        pushOffsetTable(lua);
        lua_setfield(lua, -2, name.toUtf8().constData());
    }
    lua_newtable(lua);
    lua_pushcfunction(lua, luaOffsetIndex);
    lua_setfield(lua, -2, "__index");
    lua_setmetatable(lua, -2);
    lua_setfield(lua, -2, "offset");

    setFunctionField(lua, "get_path", luaSkinConfigGetPath, &context);
    luaPushQString(lua, genericPathString(context.skinPath.parent_path()) + '/');
    lua_setfield(lua, -2, "skin_path");

    lua_setglobal(lua, "skin_config");
}

void
installPackagePath(lua_State* lua, const std::filesystem::path& base)
{
    lua_getglobal(lua, "package");
    lua_getfield(lua, -1, "path");
    const auto oldPath = QString::fromUtf8(lua_tostring(lua, -1));
    lua_pop(lua, 1);

    const auto basePath = genericPathString(base);
    const auto newPath =
      basePath + "/?.lua;" + basePath + "/?/init.lua;" + basePath +
      "/?.luaskin;" + oldPath;
    luaPushQString(lua, newPath);
    lua_setfield(lua, -2, "path");
    lua_pop(lua, 1);
}

void
installStubs(lua_State* lua, LuaExecutionContext& context)
{
    registerLoadedModule(lua,
                         "main_state",
                         &context,
                         {
                           { "option", luaMainStateOption },
                           { "text", luaMainStateText },
                           { "number", luaMainStateNumber },
                           { "float_number", luaMainStateNumber },
                           { "timer", luaMainStateTimer },
                           { "set_timer", luaMainStateSetTimer },
                           { "event_exec", luaMainStateEventExec },
                           { "time", luaMainStateTime },
                           { "gauge_type", luaReturnZero },
                           { "rate", luaMainStateNumber },
                           { "volume_sys", luaReturnZero },
                           { "volume_key", luaReturnZero },
                           { "volume_bg", luaReturnZero },
                           { "audio_play", luaReturnTrue },
                           { "audio_loop", luaReturnTrue },
                           { "audio_stop", luaReturnTrue },
                           { "set_volume_sys", luaReturnTrue },
                           { "set_volume_key", luaReturnTrue },
                           { "set_volume_bg", luaReturnTrue },
                         },
                         luaReturnZeroFunction);
    lua_getglobal(lua, "main_state");
    lua_pushnumber(lua, LuaTimerOffValue);
    lua_setfield(lua, -2, "timer_off_value");
    lua_pop(lua, 1);

    registerLoadedModule(lua,
                         "timer_util",
                         &context,
                         {
                           { "timer_observe_boolean", luaTimerUtilTimerObserveBoolean },
                           { "timer_function", luaTimerUtilTimerFunction },
                           { "now_timer", luaTimerUtilNowTimer },
                           { "new_passive_timer", luaTimerUtilNewPassiveTimer },
                           { "is_timer_on", luaTimerUtilIsTimerOn },
                           { "is_timer_off", luaTimerUtilIsTimerOff },
                         },
                         luaReturnZeroFunction);

    registerLoadedModule(lua,
                         "event_util",
                         &context,
                         {
                           { "event", luaReturnTrue },
                           { "event_index", luaReturnZero },
                           { "event_function", luaReturnZeroFunction },
                         },
                         luaReturnZeroFunction);

    registerLoadedModule(lua,
                         "luajava",
                         &context,
                          {
                            { "bindClass", luaLuajavaBindClass },
                            { "new", luaLuajavaNew },
                            { "newInstance", luaLuajavaNewInstance },
                            { "createProxy", luaLuajavaCreateProxy },
                          },
                          luaJavaObjectFallback);
}

BeatorajaLuaRuntime::BeatorajaLuaRuntime(QObject* parent)
  : QObject(parent)
{
    m_context.runtime = this;
}

BeatorajaLuaRuntime::~BeatorajaLuaRuntime() = default;

QObject*
BeatorajaLuaRuntime::stateProvider() const
{
    return m_stateProvider;
}

void
BeatorajaLuaRuntime::setStateProvider(QObject* provider)
{
    if (m_stateProvider == provider) {
        return;
    }
    m_stateProvider = provider;
    emit stateProviderChanged();
    bumpRevision();
}

int
BeatorajaLuaRuntime::revision() const
{
    return m_revision;
}

void
BeatorajaLuaRuntime::bumpRevision()
{
    ++m_revision;
    emit revisionChanged();
}

int
BeatorajaLuaRuntime::registerCallback(lua_State* lua, int index)
{
    if (lua == nullptr || !lua_isfunction(lua, index)) {
        return 0;
    }
    index = absoluteIndex(lua, index);
    lua_pushvalue(lua, index);
    const int ref = luaL_ref(lua, LUA_REGISTRYINDEX);
    const int id = m_nextCallbackId++;
    m_callbackRefs.insert(id, ref);
    return id;
}

int
BeatorajaLuaRuntime::registerActionScriptCallback(const QString& script)
{
    if (m_lua.state == nullptr || script.trimmed().isEmpty()) {
        return 0;
    }
    const auto source = script.toUtf8();
    if (loadLuaBuffer(m_lua.state, source, QByteArrayLiteral("@beatoraja-action")) != 0) {
        spdlog::warn("Failed to compile Beatoraja Lua action '{}': {}",
                     script.toStdString(),
                     lua_tostring(m_lua.state, -1));
        lua_pop(m_lua.state, 1);
        return 0;
    }
    const int callback = registerCallback(m_lua.state, -1);
    lua_pop(m_lua.state, 1);
    return callback;
}

int
BeatorajaLuaRuntime::registerExpressionCallback(const QString& expression)
{
    if (m_lua.state == nullptr || expression.trimmed().isEmpty()) {
        return 0;
    }
    const auto source = QStringLiteral("return %1").arg(expression).toUtf8();
    if (loadLuaBuffer(m_lua.state, source, QByteArrayLiteral("@beatoraja-expression")) != 0) {
        spdlog::warn("Failed to compile Beatoraja Lua expression '{}': {}",
                     expression.toStdString(),
                     lua_tostring(m_lua.state, -1));
        lua_pop(m_lua.state, 1);
        return 0;
    }
    const int callback = registerCallback(m_lua.state, -1);
    lua_pop(m_lua.state, 1);
    return callback;
}

int
BeatorajaLuaRuntime::registerTimerScriptCallback(const QString& script)
{
    if (m_lua.state == nullptr || script.trimmed().isEmpty()) {
        return 0;
    }

    const auto source = QStringLiteral("return %1").arg(script).toUtf8();
    if (loadLuaBuffer(m_lua.state, source, QByteArrayLiteral("@beatoraja-timer")) != 0) {
        spdlog::warn("Failed to compile Beatoraja Lua timer '{}': {}",
                     script.toStdString(),
                     lua_tostring(m_lua.state, -1));
        lua_pop(m_lua.state, 1);
        return 0;
    }

    lua_pushvalue(m_lua.state, -1);
    if (lua_pcall(m_lua.state, 0, 1, 0) != 0) {
        spdlog::warn("Failed to initialize Beatoraja Lua timer '{}': {}",
                     script.toStdString(),
                     lua_tostring(m_lua.state, -1));
        lua_pop(m_lua.state, 1);
        const int chunkCallback = registerCallback(m_lua.state, -1);
        lua_pop(m_lua.state, 1);
        return chunkCallback;
    }

    if (lua_isfunction(m_lua.state, -1)) {
        const int callback = registerCallback(m_lua.state, -1);
        lua_pop(m_lua.state, 2);
        return callback;
    }

    lua_pop(m_lua.state, 1);
    const int callback = registerCallback(m_lua.state, -1);
    lua_pop(m_lua.state, 1);
    return callback;
}

int
BeatorajaLuaRuntime::callbackIdFromVariant(const QVariant& value,
                                           const QString& scriptKind)
{
    const int callbackId = luaCallbackId(value);
    if (callbackId > 0) {
        return callbackId;
    }

    if (!value.isValid() || value.isNull()) {
        return 0;
    }
    if (value.metaType().id() != QMetaType::QString) {
        return 0;
    }
    const QString script = value.toString();
    if (script.isEmpty()) {
        return 0;
    }
    if (scriptKind == QStringLiteral("action")) {
        return registerActionScriptCallback(script);
    }
    if (scriptKind == QStringLiteral("timer")) {
        return registerTimerScriptCallback(script);
    }
    return registerExpressionCallback(script);
}

void
BeatorajaLuaRuntime::registerCustomTimer(const int timerId,
                                         const int callbackId)
{
    if (timerId == 0) {
        return;
    }
    if (callbackId > 0) {
        m_customTimerCallbacks.insert(timerId, callbackId);
    }
    if (!m_customTimerValuesMicro.contains(timerId)) {
        m_customTimerValuesMicro.insert(timerId, LuaTimerOffValue);
    }
}

void
BeatorajaLuaRuntime::registerCustomEvent(const int eventId,
                                         const int actionCallbackId,
                                         const int conditionCallbackId,
                                         const int minIntervalMs)
{
    if (eventId == 0 || actionCallbackId <= 0) {
        return;
    }
    m_customEvents.insert(eventId,
                          CustomEvent{
                            .actionCallbackId = actionCallbackId,
                            .conditionCallbackId = conditionCallbackId,
                            .minIntervalMs = std::max(0, minIntervalMs),
                          });
}

QVariant
BeatorajaLuaRuntime::providerValue(const char* method,
                                   const int id,
                                   const QVariant& fallback) const
{
    if (m_stateProvider == nullptr || method == nullptr) {
        return fallback;
    }

    QVariant result;
    const bool invoked =
      QMetaObject::invokeMethod(m_stateProvider,
                                method,
                                Qt::DirectConnection,
                                Q_RETURN_ARG(QVariant, result),
                                Q_ARG(QVariant, QVariant(id)));
    return invoked ? result : fallback;
}

bool
BeatorajaLuaRuntime::providerOption(const int option, const bool fallback) const
{
    if (m_stateProvider == nullptr) {
        return fallback;
    }
    QVariant result;
    const bool invoked =
      QMetaObject::invokeMethod(m_stateProvider,
                                "beatorajaLuaOptionValue",
                                Qt::DirectConnection,
                                Q_RETURN_ARG(QVariant, result),
                                Q_ARG(QVariant, QVariant(option)));
    return invoked ? result.toBool() : fallback;
}

QVariant
BeatorajaLuaRuntime::callCallbackVariant(const int callbackId,
                                         const bool keepResult,
                                         const QVariantList& arguments)
{
    if (m_lua.state == nullptr || !m_callbackRefs.contains(callbackId)) {
        return {};
    }

    lua_rawgeti(m_lua.state, LUA_REGISTRYINDEX, m_callbackRefs.value(callbackId));
    for (const auto& argument : arguments) {
        luaPushVariant(m_lua.state, argument);
    }
    if (lua_pcall(m_lua.state,
                  arguments.size(),
                  keepResult ? 1 : 0,
                  0) != 0) {
        spdlog::warn("Failed to execute Beatoraja Lua callback {}: {}",
                     callbackId,
                     lua_tostring(m_lua.state, -1));
        lua_pop(m_lua.state, 1);
        return {};
    }

    if (!keepResult) {
        return {};
    }

    auto result = luaValueToVariant(m_lua.state, -1, this);
    lua_pop(m_lua.state, 1);
    return result;
}

QVariant
BeatorajaLuaRuntime::call(const int callbackId)
{
    return callCallbackVariant(callbackId, true);
}

int
BeatorajaLuaRuntime::callInt(const int callbackId)
{
    const auto value = callCallbackVariant(callbackId, true);
    bool ok = false;
    const double number = value.toDouble(&ok);
    if (!ok || !std::isfinite(number)) {
        return 0;
    }
    if (number >= 2147483648.0 ||
        number <= static_cast<double>(std::numeric_limits<int>::min())) {
        return std::numeric_limits<int>::min();
    }
    return static_cast<int>(number);
}

QString
BeatorajaLuaRuntime::callString(const int callbackId)
{
    const auto value = callCallbackVariant(callbackId, true);
    return value.isValid() && !value.isNull() ? value.toString() : QString{};
}

bool
BeatorajaLuaRuntime::callBool(const int callbackId, const bool fallback)
{
    const auto value = callCallbackVariant(callbackId, true);
    if (!value.isValid() || value.isNull()) {
        return fallback;
    }
    if (value.metaType().id() == QMetaType::Bool) {
        return value.toBool();
    }
    bool ok = false;
    const double number = value.toDouble(&ok);
    if (ok) {
        return number != 0.0;
    }
    return !value.toString().isEmpty();
}

void
BeatorajaLuaRuntime::callAction(const int callbackId)
{
    callCallbackVariant(callbackId, false);
    bumpRevision();
}

void
BeatorajaLuaRuntime::callFloatWriter(const int callbackId, const double value)
{
    callCallbackVariant(callbackId, false, { QVariant(value) });
    bumpRevision();
}

bool
BeatorajaLuaRuntime::hasCustomTimer(const int timerId) const
{
    return m_customTimerValuesMicro.contains(timerId);
}

QVariant
BeatorajaLuaRuntime::customTimerMicroValue(const int timerId) const
{
    return m_customTimerValuesMicro.value(timerId, LuaTimerOffValue);
}

QVariantMap
BeatorajaLuaRuntime::customTimerValuesMs() const
{
    QVariantMap result;
    for (auto it = m_customTimerValuesMicro.cbegin();
         it != m_customTimerValuesMicro.cend();
         ++it) {
        const double micro = it.value();
        result.insert(QString::number(it.key()),
                      timerIsOff(micro)
                        ? -1
                        : static_cast<int>(std::floor(micro / 1000.0)));
    }
    return result;
}

QVariantMap
BeatorajaLuaRuntime::updateCustomObjects()
{
    bool changed = false;

    QList<int> timerIds = m_customTimerCallbacks.keys();
    std::sort(timerIds.begin(), timerIds.end());
    for (const int timerId : timerIds) {
        const int callbackId = m_customTimerCallbacks.value(timerId);
        const auto value = callCallbackVariant(callbackId, true);
        bool ok = false;
        const double micro = value.toDouble(&ok);
        const double next = ok && std::isfinite(micro)
                              ? micro
                              : LuaTimerOffValue;
        if (m_customTimerValuesMicro.value(timerId, LuaTimerOffValue) != next) {
            m_customTimerValuesMicro.insert(timerId, next);
            changed = true;
        }
    }

    QList<int> eventIds = m_customEvents.keys();
    std::sort(eventIds.begin(), eventIds.end());
    const double now = currentTimeMicro();
    for (const int eventId : eventIds) {
        auto event = m_customEvents.value(eventId);
        if (event.conditionCallbackId <= 0 ||
            !callBool(event.conditionCallbackId, false)) {
            continue;
        }
        const bool canRun = timerIsOff(event.lastExecuteMicro) ||
          (now - event.lastExecuteMicro) / 1000.0 >= event.minIntervalMs;
        if (!canRun) {
            continue;
        }
        callCallbackVariant(event.actionCallbackId, false);
        event.lastExecuteMicro = now;
        m_customEvents.insert(eventId, event);
        changed = true;
    }

    if (!m_customTimerCallbacks.isEmpty() || changed) {
        bumpRevision();
    }
    return customTimerValuesMs();
}

bool
BeatorajaLuaRuntime::hasCustomEvent(const int eventId) const
{
    return m_customEvents.contains(eventId);
}

bool
BeatorajaLuaRuntime::callEvent(const int eventId, const int arg1, const int arg2)
{
    if (!m_customEvents.contains(eventId)) {
        return false;
    }

    auto event = m_customEvents.value(eventId);
    callCallbackVariant(event.actionCallbackId,
                        false,
                        { QVariant(arg1), QVariant(arg2) });
    event.lastExecuteMicro = currentTimeMicro();
    m_customEvents.insert(eventId, event);
    bumpRevision();
    return true;
}

void
BeatorajaLuaRuntime::setCustomTimerMicroValue(const int timerId,
                                              const double microValue)
{
    if (timerId == 0) {
        return;
    }
    if (m_customTimerCallbacks.contains(timerId)) {
        return;
    }
    const double next = std::isfinite(microValue)
                          ? microValue
                          : LuaTimerOffValue;
    if (m_customTimerValuesMicro.value(timerId, LuaTimerOffValue) == next) {
        return;
    }
    m_customTimerValuesMicro.insert(timerId, next);
    bumpRevision();
}

double
BeatorajaLuaRuntime::currentTimeMicro() const
{
    const auto value = providerValue("beatorajaLuaTimeValue", 0, 0);
    bool ok = false;
    const double result = value.toDouble(&ok);
    return ok && std::isfinite(result) ? result : 0.0;
}

bool
BeatorajaLuaRuntime::timerIsOff(const double value)
{
    return value <= LuaTimerOffValue / 2.0;
}

bool
BeatorajaLuaRuntime::load(const std::filesystem::path& skinPath,
                          const QVariantMap& header,
                          const QVariantMap& settingValues,
                          const std::set<int>& activeOptions,
                          QVariantMap& skin)
{
    QFile file(support::pathToQString(skinPath));
    if (!file.open(QIODevice::ReadOnly)) {
        spdlog::warn("Could not open beatoraja Lua skin: {}", skinPath.string());
        return false;
    }

    if (m_lua.state == nullptr) {
        spdlog::warn("Could not initialize LuaJIT for skin: {}",
                     skinPath.string());
        return false;
    }

    m_context.skinPath = skinPath;
    m_context.header = header;
    m_context.settingValues = settingValues;
    m_context.activeOptions = activeOptions;
    applyBeatorajaLuaDefaultOptions(m_context.activeOptions);
    m_context.filePaths = filePathOptionsFromHeader(header);
    m_context.selectedOptions =
      selectedOptionsFromHeader(header, settingValues);
    for (const auto option : m_context.selectedOptions) {
        m_context.activeOptions.insert(option);
    }

    installPackagePath(m_lua.state, skinPath.parent_path());
    installSkinLuaLoader(m_lua.state, m_context);
    installStubs(m_lua.state, m_context);
    installSkinConfig(m_lua.state, m_context);

    const auto source = file.readAll();
    const auto chunkName = QByteArray("@") + genericPathString(skinPath).toUtf8();
    if (loadLuaBuffer(m_lua.state, source, chunkName) != 0) {
        spdlog::warn("Failed to load Lua skin {}: {}",
                     skinPath.string(),
                     lua_tostring(m_lua.state, -1));
        return false;
    }
    if (lua_pcall(m_lua.state, 0, 1, 0) != 0) {
        spdlog::warn("Failed to execute Lua skin {}: {}",
                     skinPath.string(),
                     lua_tostring(m_lua.state, -1));
        return false;
    }

    skin = asMap(luaValueToVariant(m_lua.state, -1, this));
    lua_pop(m_lua.state, 1);
    return !skin.isEmpty();
}

auto
executeLuaSkin(const std::filesystem::path& skinPath,
               const QVariantMap& header,
               const QVariantMap& settingValues,
               const std::set<int>& activeOptions,
               const bool fullPass) -> QVariantMap
{
    QFile file(support::pathToQString(skinPath));
    if (!file.open(QIODevice::ReadOnly)) {
        spdlog::warn("Could not open beatoraja Lua skin: {}", skinPath.string());
        return {};
    }

    LuaState lua;
    if (lua.state == nullptr) {
        spdlog::warn("Could not initialize LuaJIT for skin: {}", skinPath.string());
        return {};
    }

    LuaExecutionContext context;
    context.skinPath = skinPath;
    context.header = header;
    context.settingValues = settingValues;
    context.activeOptions = activeOptions;
    applyBeatorajaLuaDefaultOptions(context.activeOptions);
    context.filePaths = filePathOptionsFromHeader(header);
    context.selectedOptions = selectedOptionsFromHeader(header, settingValues);
    for (const auto option : context.selectedOptions) {
        context.activeOptions.insert(option);
    }

    installPackagePath(lua.state, skinPath.parent_path());
    installSkinLuaLoader(lua.state, context);
    installStubs(lua.state, context);
    if (fullPass) {
        installSkinConfig(lua.state, context);
    }

    const auto source = file.readAll();
    const auto chunkName = QByteArray("@") + genericPathString(skinPath).toUtf8();
    if (loadLuaBuffer(lua.state, source, chunkName) != 0) {
        spdlog::warn("Failed to load Lua skin {}: {}",
                     skinPath.string(),
                     lua_tostring(lua.state, -1));
        return {};
    }
    if (lua_pcall(lua.state, 0, 1, 0) != 0) {
        spdlog::warn("Failed to execute Lua skin {}: {}",
                     skinPath.string(),
                     lua_tostring(lua.state, -1));
        return {};
    }

    return asMap(luaValueToVariant(lua.state, -1));
}

auto
builtinImageSource(const int id) -> std::optional<Lr2SrcImage>
{
    const int positive = std::abs(id);
    Lr2SrcImage src;
    src.gr = positive;
    src.x = -1;
    src.y = -1;
    src.w = -1;
    src.h = -1;
    switch (positive) {
        case 100:
            src.specialType = Lr2SrcImage::StageFile;
            return src;
        case 101:
            src.specialType = Lr2SrcImage::BackBmp;
            return src;
        case 102:
            src.specialType = Lr2SrcImage::Banner;
            return src;
        case 110:
            src.specialType = Lr2SrcImage::SolidBlack;
            return src;
        case 111:
            src.specialType = Lr2SrcImage::SolidWhite;
            return src;
        default:
            return std::nullopt;
    }
}

void
applyBeatorajaButton(ConvertState& state,
                     Lr2SrcImage& src,
                     const QVariantMap& object)
{
    const auto actionValue = object.value(QStringLiteral("act"));
    const int callback =
      beatorajaPropertyCallback(state, actionValue, QStringLiteral("action"));
    const int action = callback > 0 ? 0 : toInt(actionValue);
    if (action == 0 && callback == 0) {
        return;
    }

    src.button = true;
    src.buttonId = action;
    src.buttonActionCallback = callback;
    src.buttonClickEnabled = true;
    src.buttonClickMode = std::clamp(toInt(object.value(QStringLiteral("click"))), 0, 3);
}

void
recordOption(std::set<int>& options, const int option)
{
    if (option != 0) {
        options.insert(std::abs(option));
    }
}

void
applyBeatorajaLuaDefaultOptions(std::set<int>& options)
{
    bool hasDifficultyOption = false;
    for (int option = 150; option <= 155; ++option) {
        hasDifficultyOption = hasDifficultyOption || options.contains(option);
    }
    if (!hasDifficultyOption) {
        options.insert(150);
    }
}

void
recordDstOptions(ConvertState& state,
                 const Lr2Dst& dst,
                 const bool elementOptions)
{
    for (const int option : { dst.op1, dst.op2, dst.op3, dst.op4 }) {
        recordOption(state.usedOptions, option);
        if (elementOptions) {
            recordOption(state.usedElementOptions, option);
        }
    }
}

void
addDstOptionGate(Lr2Dst& dst, const int option)
{
    if (option == 0) {
        return;
    }
    if (dst.op1 == 0) {
        dst.op1 = option;
    } else if (dst.op2 == 0) {
        dst.op2 = option;
    } else if (dst.op3 == 0) {
        dst.op3 = option;
    } else if (dst.op4 == 0) {
        dst.op4 = option;
    }
}

void
recordBarChildHeight(ConvertState& state, const QVariantList& dsts)
{
    for (const auto& value : dsts) {
        if (!value.canConvert<Lr2Dst>()) {
            continue;
        }
        const auto dst = value.value<Lr2Dst>();
        const int height = std::abs(dst.h);
        if (height > 0) {
            state.barChildHeight = std::max(state.barChildHeight, height);
        }
    }
}

auto
beatorajaDstToRendererDst(const ConvertState& state,
                          const Lr2Dst& raw,
                          const DstCoordinateSpace coordinateSpace) -> Lr2Dst
{
    auto dst = raw;
    if (coordinateSpace == DstCoordinateSpace::BarChild) {
        dst.y = state.barChildHeight - raw.y - raw.h;
    } else if (coordinateSpace == DstCoordinateSpace::Relative) {
        dst.y = raw.y;
    } else {
        dst.y = state.skinHeight - raw.y - raw.h;
    }
    return dst;
}

auto
dstOffsetsFromMap(const QVariantMap& destination) -> QVariantList
{
    QVariantList result;
    for (const auto& offset : asList(destination.value(QStringLiteral("offsets")))) {
        const int value = toInt(offset);
        if (value != 0) {
            result.append(value);
        }
    }
    const int offset = toInt(destination.value(QStringLiteral("offset")));
    if (offset != 0) {
        result.append(offset);
    }
    return result;
}

auto
optionListFromVariant(const QVariant& value) -> QVariantList
{
    auto list = asList(value);
    if (!list.isEmpty()) {
        return list;
    }
    if (!value.isValid() || value.isNull()) {
        return {};
    }
    bool ok = false;
    value.toInt(&ok);
    return ok ? QVariantList{ value } : QVariantList{};
}

auto
destinationDsts(const QVariantMap& destination,
                ConvertState& state,
                const bool elementOptions = true,
                const DstCoordinateSpace coordinateSpace =
                  DstCoordinateSpace::Screen) -> QVariantList
{
    QVariantList result;
    const auto animations = asList(destination.value(QStringLiteral("dst")));
    if (animations.isEmpty()) {
        return result;
    }

    Lr2Dst previousRaw;
    previousRaw.sortId = state.sortId;
    const auto offsets = dstOffsetsFromMap(destination);
    const auto draw = destination.value(QStringLiteral("draw"));
    const bool drawIsStaticBool = draw.metaType().id() == QMetaType::Bool;
    const int drawCallback =
      beatorajaPropertyCallback(state, draw, QStringLiteral("expression"));
    if (draw.isValid() && !draw.isNull() && drawIsStaticBool && !draw.toBool()) {
        ++state.sortId;
        return result;
    }
    const auto ops = draw.isValid() && !draw.isNull() && !drawIsStaticBool &&
                         drawCallback == 0
                       ? optionListFromVariant(draw)
                       : optionListFromVariant(destination.value(QStringLiteral("op")));
    const auto mouseRect = asMap(destination.value(QStringLiteral("mouseRect")));

    for (const auto& animationValue : animations) {
        const auto animation = asMap(animationValue);
        auto rawDst = previousRaw;
        rawDst.sortId = state.sortId;
        if (animation.contains(QStringLiteral("time"))) {
            rawDst.time = toInt(animation.value(QStringLiteral("time")));
        }
        if (animation.contains(QStringLiteral("x"))) {
            rawDst.x = toInt(animation.value(QStringLiteral("x")));
        }
        if (animation.contains(QStringLiteral("y"))) {
            rawDst.y = toInt(animation.value(QStringLiteral("y")));
        }
        if (animation.contains(QStringLiteral("w"))) {
            rawDst.w = toInt(animation.value(QStringLiteral("w")));
        }
        if (animation.contains(QStringLiteral("h"))) {
            rawDst.h = toInt(animation.value(QStringLiteral("h")));
        }
        if (animation.contains(QStringLiteral("acc"))) {
            rawDst.acc = toInt(animation.value(QStringLiteral("acc")));
        }
        if (animation.contains(QStringLiteral("a"))) {
            rawDst.a = toInt(animation.value(QStringLiteral("a")), 255);
        }
        if (animation.contains(QStringLiteral("r"))) {
            rawDst.r = toInt(animation.value(QStringLiteral("r")), 255);
        }
        if (animation.contains(QStringLiteral("g"))) {
            rawDst.g = toInt(animation.value(QStringLiteral("g")), 255);
        }
        if (animation.contains(QStringLiteral("b"))) {
            rawDst.b = toInt(animation.value(QStringLiteral("b")), 255);
        }
        if (animation.contains(QStringLiteral("angle"))) {
            rawDst.angle = toInt(animation.value(QStringLiteral("angle")));
        }

        rawDst.blend = toInt(destination.value(QStringLiteral("blend")));
        rawDst.filter = toInt(destination.value(QStringLiteral("filter")));
        const auto timer = destination.value(QStringLiteral("timer"));
        rawDst.timerCallback =
          beatorajaPropertyCallback(state, timer, QStringLiteral("timer"));
        rawDst.timer = rawDst.timerCallback > 0 ? 0 : toInt(timer);
        rawDst.loop = toInt(destination.value(QStringLiteral("loop")));
        rawDst.center = toInt(destination.value(QStringLiteral("center")));
        rawDst.offsets = offsets;
        rawDst.stretch = toInt(destination.value(QStringLiteral("stretch")), -1);
        rawDst.drawCallback = drawCallback;
        if (!mouseRect.isEmpty()) {
            rawDst.hasMouseRect = true;
            rawDst.mouseRectX = toInt(mouseRect.value(QStringLiteral("x")));
            rawDst.mouseRectY = toInt(mouseRect.value(QStringLiteral("y")));
            rawDst.mouseRectW = toInt(mouseRect.value(QStringLiteral("w")));
            rawDst.mouseRectH = toInt(mouseRect.value(QStringLiteral("h")));
        }
        if (ops.size() > 0) {
            rawDst.op1 = toInt(ops[0]);
        }
        if (ops.size() > 1) {
            rawDst.op2 = toInt(ops[1]);
        }
        if (ops.size() > 2) {
            rawDst.op3 = toInt(ops[2]);
        }
        if (ops.size() > 3) {
            rawDst.op4 = toInt(ops[3]);
        }

        const auto dst = beatorajaDstToRendererDst(state, rawDst, coordinateSpace);
        recordDstOptions(state, dst, elementOptions);
        result.append(QVariant::fromValue(dst));
        previousRaw = rawDst;
    }

    ++state.sortId;
    return result;
}

auto
withDstOffsets(const QVariantList& dsts, const QList<int>& offsets) -> QVariantList
{
    if (offsets.isEmpty()) {
        return dsts;
    }
    QVariantList result;
    for (const auto& value : dsts) {
        if (!value.canConvert<Lr2Dst>()) {
            result.append(value);
            continue;
        }
        auto dst = value.value<Lr2Dst>();
        for (const int offset : offsets) {
            if (offset != 0 && !dst.offsets.contains(offset)) {
                dst.offsets.append(offset);
            }
        }
        result.append(QVariant::fromValue(dst));
    }
    return result;
}

auto
withJudgeRuntimeState(ConvertState& state,
                      const QVariantList& dsts,
                      const int option,
                      const int timer) -> QVariantList
{
    QVariantList result;
    for (const auto& value : dsts) {
        if (!value.canConvert<Lr2Dst>()) {
            result.append(value);
            continue;
        }
        auto dst = value.value<Lr2Dst>();
        dst.timer = timer;
        addDstOptionGate(dst, option);
        recordDstOptions(state, dst, true);
        result.append(QVariant::fromValue(dst));
    }
    return result;
}

auto
withBasePosition(const QVariantList& dsts, const QVariantList& baseDsts)
  -> QVariantList
{
    if (baseDsts.isEmpty()) {
        return dsts;
    }

    QVariantList result;
    for (int i = 0; i < dsts.size(); ++i) {
        const auto& value = dsts[i];
        if (!value.canConvert<Lr2Dst>()) {
            result.append(value);
            continue;
        }
        const int baseIndex =
          std::min(i, static_cast<int>(baseDsts.size()) - 1);
        const auto& baseValue = baseDsts[baseIndex];
        if (!baseValue.canConvert<Lr2Dst>()) {
            result.append(value);
            continue;
        }
        auto dst = value.value<Lr2Dst>();
        const auto base = baseValue.value<Lr2Dst>();
        dst.x += base.x;
        dst.y += base.y;
        result.append(QVariant::fromValue(dst));
    }
    return result;
}

auto
animationAsDstList(const QVariant& animationValue, ConvertState& state)
  -> QVariantList
{
    QVariantMap destination;
    destination[QStringLiteral("dst")] = QVariantList{ animationValue };
    return destinationDsts(destination, state, false);
}

auto
sourcePath(ConvertState& state, const QVariant& srcId) -> QString
{
    const auto id = idString(srcId);
    if (id.isEmpty()) {
        return {};
    }
    if (state.sourcePaths.contains(id)) {
        return state.sourcePaths.value(id);
    }
    return resolveSkinPath(state.skinPath.parent_path(),
                           state.filePaths,
                           state.settingValues,
                           id);
}

auto
imageSourceVariant(ConvertState& state, const QString& id) -> QVariant
{
    if (state.images.contains(id)) {
        return QVariant::fromValue(state.images.value(id));
    }
    if (state.imageSets.contains(id)) {
        return QVariant::fromValue(state.imageSets.value(id));
    }
    bool ok = false;
    const int numericId = id.toInt(&ok);
    if (ok) {
        if (const auto builtin = builtinImageSource(numericId)) {
            return QVariant::fromValue(*builtin);
        }
    }
    return {};
}

auto
barImageSourceVariants(const QVariant& source) -> QVariantList
{
    if (!source.canConvert<Lr2SrcImage>()) {
        return {};
    }

    const auto image = source.value<Lr2SrcImage>();
    if (image.imageSet && !image.imageSetSources.isEmpty()) {
        return image.imageSetSources;
    }

    return { source };
}

auto
makeImageSource(ConvertState& state, const QVariantMap& image) -> Lr2SrcImage
{
    Lr2SrcImage src;
    src.x = toInt(image.value(QStringLiteral("x")));
    src.y = toInt(image.value(QStringLiteral("y")));
    src.w = toInt(image.value(QStringLiteral("w")));
    src.h = toInt(image.value(QStringLiteral("h")));
    src.div_x = std::max(1, toInt(image.value(QStringLiteral("divx")), 1));
    src.div_y = std::max(1, toInt(image.value(QStringLiteral("divy")), 1));
    src.cycle = toInt(image.value(QStringLiteral("cycle")));
    const auto timer = image.value(QStringLiteral("timer"));
    src.timerCallback =
      beatorajaPropertyCallback(state, timer, QStringLiteral("timer"));
    src.timer = src.timerCallback > 0 ? 0 : toInt(timer);
    src.imageSetRef = toInt(image.value(QStringLiteral("ref")));

    const auto sourceId = idString(image.value(QStringLiteral("src")));
    bool ok = false;
    const int numericSource = sourceId.toInt(&ok);
    if (ok) {
        if (const auto builtin = builtinImageSource(numericSource)) {
            src.specialType = builtin->specialType;
        }
    }
    if (src.specialType == Lr2SrcImage::None) {
        src.source = sourcePath(state, image.value(QStringLiteral("src")));
    }

    applyBeatorajaButton(state, src, image);
    return src;
}

auto
makeImageSetSource(ConvertState& state, const QVariantMap& imageSet)
  -> Lr2SrcImage
{
    Lr2SrcImage src;
    src.imageSet = true;
    src.imageSetRef = toInt(imageSet.value(QStringLiteral("ref")));
    src.imageSetValueCallback = beatorajaPropertyCallback(
      state,
      imageSet.value(QStringLiteral("value")),
      QStringLiteral("expression"));
    src.cycle = toInt(imageSet.value(QStringLiteral("cycle")));
    const auto timer = imageSet.value(QStringLiteral("timer"));
    src.timerCallback =
      beatorajaPropertyCallback(state, timer, QStringLiteral("timer"));
    src.timer = src.timerCallback > 0 ? 0 : toInt(timer);
    bool copiedFallback = false;
    for (const auto& idValue : asList(imageSet.value(QStringLiteral("images")))) {
        const auto sourceId = idString(idValue);
        const auto source = imageSourceVariant(state, sourceId);
        if (!source.isValid()) {
            continue;
        }
        src.imageSetSources.append(source);
        if (!copiedFallback && source.canConvert<Lr2SrcImage>()) {
            const auto image = source.value<Lr2SrcImage>();
            src.gr = image.gr;
            src.x = image.x;
            src.y = image.y;
            src.w = image.w;
            src.h = image.h;
            src.div_x = image.div_x;
            src.div_y = image.div_y;
            src.specialType = image.specialType;
            src.source = image.source;
            copiedFallback = true;
        }
    }
    applyBeatorajaButton(state, src, imageSet);
    return src;
}

auto
makeNumberSource(ConvertState& state, const QVariantMap& number) -> Lr2SrcNumber
{
    Lr2SrcNumber src;
    src.x = toInt(number.value(QStringLiteral("x")));
    src.y = toInt(number.value(QStringLiteral("y")));
    src.w = toInt(number.value(QStringLiteral("w")));
    src.h = toInt(number.value(QStringLiteral("h")));
    src.div_x = std::max(1, toInt(number.value(QStringLiteral("divx")), 1));
    src.div_y = std::max(1, toInt(number.value(QStringLiteral("divy")), 1));
    src.cycle = toInt(number.value(QStringLiteral("cycle")));
    const auto timer = number.value(QStringLiteral("timer"));
    src.timerCallback =
      beatorajaPropertyCallback(state, timer, QStringLiteral("timer"));
    src.timer = src.timerCallback > 0 ? 0 : toInt(timer);
    src.num = toInt(number.value(QStringLiteral("ref")));
    const auto value = number.value(QStringLiteral("value"));
    src.valueCallback =
      beatorajaPropertyCallback(state, value, QStringLiteral("expression"));
    if (src.num == 0 && src.valueCallback == 0 && value.isValid() &&
        !value.isNull()) {
        src.constantValueEnabled = true;
        src.constantValue = toInt(value);
        src.num = src.constantValue;
    }
    src.align = toInt(number.value(QStringLiteral("align")));
    src.keta = toInt(number.value(QStringLiteral("digit")));
    if (src.keta == 0) {
        const auto integerDigits = toInt(number.value(QStringLiteral("iketa")));
        const auto fractionDigits = toInt(number.value(QStringLiteral("fketa")));
        if (integerDigits > 0 || fractionDigits > 0) {
            src.keta = integerDigits + fractionDigits + (fractionDigits > 0 ? 1 : 0);
        }
    }
    src.zeropadding = toInt(number.value(QStringLiteral("zeropadding")),
                            toInt(number.value(QStringLiteral("padding")), -1));
    src.source = sourcePath(state, number.value(QStringLiteral("src")));
    return src;
}

auto
makeTextSource(ConvertState& state, const QVariantMap& text) -> Lr2SrcText
{
    Lr2SrcText src;
    src.st = toInt(text.value(QStringLiteral("ref")));
    src.valueCallback = beatorajaPropertyCallback(
      state,
      text.value(QStringLiteral("value")),
      QStringLiteral("expression"));
    const auto constantText = text.value(QStringLiteral("constantText"));
    if (constantText.isValid() && !constantText.isNull()) {
        src.constantText = constantText.toString();
    } else if (src.st == 0 && src.valueCallback == 0 &&
               text.value(QStringLiteral("value")).isValid() &&
               !text.value(QStringLiteral("value")).isNull()) {
        src.constantText = text.value(QStringLiteral("value")).toString();
    }
    src.align = toInt(text.value(QStringLiteral("align")));
    src.fontSize = toInt(text.value(QStringLiteral("size")));
    const auto fontId = idString(text.value(QStringLiteral("font")));
    if (state.fonts.contains(fontId)) {
        const auto font = state.fonts.value(fontId);
        src.fontPath = font.path;
        src.fontFamily = font.family;
        src.fontType = font.type;
        src.bitmapFont = font.bitmap;
    }
    return src;
}

auto
barTextSourceFromText(const Lr2SrcText& text, const int titleType)
  -> Lr2SrcBarText
{
    Lr2SrcBarText src;
    src.titleType = titleType;
    src.font = text.font;
    src.st = text.st;
    src.align = text.align;
    src.edit = text.edit;
    src.panel = text.panel;
    src.fontPath = text.fontPath;
    src.fontFamily = text.fontFamily;
    src.fontSize = text.fontSize;
    src.fontThickness = text.fontThickness;
    src.fontType = text.fontType;
    src.bitmapFont = text.bitmapFont;
    src.constantText = text.constantText;
    src.valueCallback = text.valueCallback;
    return src;
}

auto
makeSliderSource(ConvertState& state, const QVariantMap& slider) -> Lr2SrcImage
{
    auto src = makeImageSource(state, slider);
    src.slider = true;
    src.sliderDirection = toInt(slider.value(QStringLiteral("angle")));
    src.sliderRange = toInt(slider.value(QStringLiteral("range")));
    src.sliderType = toInt(slider.value(QStringLiteral("type")));
    src.sliderDisabled = toBool(slider.value(QStringLiteral("changeable")), true) ? 0 : 1;
    src.sliderRefNumber = toBool(slider.value(QStringLiteral("isRefNum")));
    src.sliderMinValue = toInt(slider.value(QStringLiteral("min")));
    src.sliderMaxValue = toInt(slider.value(QStringLiteral("max")));
    src.sliderValueCallback = beatorajaPropertyCallback(
      state,
      slider.value(QStringLiteral("value")),
      QStringLiteral("expression"));
    src.sliderEventCallback = beatorajaPropertyCallback(
      state,
      slider.value(QStringLiteral("event")),
      QStringLiteral("action"));
    return src;
}

auto
makeGraphSource(ConvertState& state, const QVariantMap& graph)
  -> Lr2SrcBarGraph
{
    Lr2SrcBarGraph src;
    src.x = toInt(graph.value(QStringLiteral("x")));
    src.y = toInt(graph.value(QStringLiteral("y")));
    src.w = toInt(graph.value(QStringLiteral("w")));
    src.h = toInt(graph.value(QStringLiteral("h")));
    src.div_x = std::max(1, toInt(graph.value(QStringLiteral("divx")), 1));
    src.div_y = std::max(1, toInt(graph.value(QStringLiteral("divy")), 1));
    src.cycle = toInt(graph.value(QStringLiteral("cycle")));
    const auto timer = graph.value(QStringLiteral("timer"));
    src.timerCallback =
      beatorajaPropertyCallback(state, timer, QStringLiteral("timer"));
    src.timer = src.timerCallback > 0 ? 0 : toInt(timer);
    src.graphType = toInt(graph.value(QStringLiteral("type")));
    src.direction = toInt(graph.value(QStringLiteral("angle")), 1);
    src.graphRefNumber = toBool(graph.value(QStringLiteral("isRefNum")));
    src.graphMinValue = toInt(graph.value(QStringLiteral("min")));
    src.graphMaxValue = toInt(graph.value(QStringLiteral("max")));
    src.valueCallback = beatorajaPropertyCallback(
      state,
      graph.value(QStringLiteral("value")),
      QStringLiteral("expression"));
    src.source = sourcePath(state, graph.value(QStringLiteral("src")));
    return src;
}

int
beatorajaDistributionGraphType(const int graphType)
{
    if (graphType < 0) {
        return graphType == -1 ? 0 : 1;
    }
    return graphType;
}

auto
makeNoteChartSource(const QVariantMap& graph) -> Lr2SrcNoteChart
{
    Lr2SrcNoteChart src;
    src.chartType = toInt(graph.value(QStringLiteral("type")));
    src.delay = toInt(graph.value(QStringLiteral("delay")), 500);
    src.backTexOff = toInt(graph.value(QStringLiteral("backTexOff")));
    src.orderReverse = toInt(graph.value(QStringLiteral("orderReverse")));
    src.noGap = toInt(graph.value(QStringLiteral("noGap")));
    return src;
}

auto
makeBpmChartSource(const QVariantMap& graph) -> Lr2SrcBpmChart
{
    auto colorValue = [&graph](const QString& key,
                               const QString& fallback) -> QString {
        const auto value = graph.value(key).toString();
        return value.isEmpty() ? fallback : value;
    };

    Lr2SrcBpmChart src;
    src.delay = toInt(graph.value(QStringLiteral("delay")));
    src.lineWidth =
      std::max(1, toInt(graph.value(QStringLiteral("lineWidth")), 2));
    src.mainBpmColor =
      colorValue(QStringLiteral("mainBPMColor"), QStringLiteral("00ff00"));
    src.minBpmColor =
      colorValue(QStringLiteral("minBPMColor"), QStringLiteral("0000ff"));
    src.maxBpmColor =
      colorValue(QStringLiteral("maxBPMColor"), QStringLiteral("ff0000"));
    src.otherBpmColor =
      colorValue(QStringLiteral("otherBPMColor"), QStringLiteral("ffff00"));
    src.stopLineColor =
      colorValue(QStringLiteral("stopLineColor"), QStringLiteral("ff00ff"));
    src.transitionLineColor =
      colorValue(QStringLiteral("transitionLineColor"), QStringLiteral("7f7f7f"));
    return src;
}

auto
makeGaugeGraphSource(const QVariantMap& graph) -> Lr2SrcImage
{
    Lr2SrcImage src;
    src.resultChartType = 1;
    src.resultChartIndex = -1;
    src.side = std::max(1, toInt(graph.value(QStringLiteral("side")), 1));
    return src;
}

auto
makeTimingDistributionGraphSource(const QVariantMap& graph) -> Lr2SrcImage
{
    auto colorValue = [&graph](const QString& key,
                               const QString& fallback) -> QString {
        const auto value = graph.value(key).toString();
        return value.isEmpty() ? fallback : value;
    };

    Lr2SrcImage src;
    src.resultChartType = 2;
    src.timingGraphWidth =
      std::max(1, toInt(graph.value(QStringLiteral("width")), 301));
    src.timingGraphLineWidth =
      std::max(1, toInt(graph.value(QStringLiteral("lineWidth")), 1));
    src.timingGraphColor =
      colorValue(QStringLiteral("graphColor"), QStringLiteral("00FF00FF"));
    src.timingAverageColor =
      colorValue(QStringLiteral("averageColor"), QStringLiteral("FFFFFFFF"));
    src.timingDevColor =
      colorValue(QStringLiteral("devColor"), QStringLiteral("FFFFFFFF"));
    src.timingPGColor =
      colorValue(QStringLiteral("PGColor"), QStringLiteral("000088FF"));
    src.timingGRColor =
      colorValue(QStringLiteral("GRColor"), QStringLiteral("008800FF"));
    src.timingGDColor =
      colorValue(QStringLiteral("GDColor"), QStringLiteral("888800FF"));
    src.timingBDColor =
      colorValue(QStringLiteral("BDColor"), QStringLiteral("880000FF"));
    src.timingPRColor =
      colorValue(QStringLiteral("PRColor"), QStringLiteral("000000FF"));
    src.timingDrawAverage =
      toInt(graph.value(QStringLiteral("drawAverage")), 1);
    src.timingDrawDev =
      toInt(graph.value(QStringLiteral("drawDev")), 1);
    return src;
}

auto
makeTimingVisualizerSource(const QVariantMap& graph, const int chartType)
  -> Lr2SrcImage
{
    auto colorValue = [&graph](const QString& key,
                               const QString& fallback) -> QString {
        const auto value = graph.value(key).toString();
        return value.isEmpty() ? fallback : value;
    };

    Lr2SrcImage src;
    src.resultChartType = chartType;
    src.timingGraphWidth =
      std::max(1, toInt(graph.value(QStringLiteral("width")), 301));
    src.op1 =
      std::max(1, toInt(graph.value(QStringLiteral("judgeWidthMillis")), 150));
    src.timingGraphLineWidth =
      std::max(1, toInt(graph.value(QStringLiteral("lineWidth")), 1));
    src.timingGraphColor =
      colorValue(QStringLiteral("lineColor"), QStringLiteral("00FF00FF"));
    src.timingAverageColor =
      colorValue(QStringLiteral("centerColor"), QStringLiteral("FFFFFFFF"));
    src.timingPGColor =
      colorValue(QStringLiteral("PGColor"), QStringLiteral("000088FF"));
    src.timingGRColor =
      colorValue(QStringLiteral("GRColor"), QStringLiteral("008800FF"));
    src.timingGDColor =
      colorValue(QStringLiteral("GDColor"), QStringLiteral("888800FF"));
    src.timingBDColor =
      colorValue(QStringLiteral("BDColor"), QStringLiteral("880000FF"));
    src.timingPRColor =
      colorValue(QStringLiteral("PRColor"), QStringLiteral("000000FF"));
    src.timingDrawDev = toInt(graph.value(QStringLiteral("drawDecay")), 1);
    return src;
}

auto
makeGaugeSource(ConvertState& state, const QVariantMap& gauge) -> Lr2SrcImage
{
    Lr2SrcImage src;
    src.grooveGaugeEx = true;
    src.cycle = toInt(gauge.value(QStringLiteral("cycle")));
    const auto nodes = asList(gauge.value(QStringLiteral("nodes")));
    if (!nodes.isEmpty()) {
        const auto source = imageSourceVariant(state, idString(nodes.first()));
        if (source.canConvert<Lr2SrcImage>()) {
            src = source.value<Lr2SrcImage>();
            src.grooveGaugeEx = true;
        }
    }
    return src;
}

void
appendElement(ConvertState& state,
              const int type,
              const QVariant& source,
              const QVariantList& dsts)
{
    if (dsts.isEmpty()) {
        return;
    }
    Lr2Element element;
    element.type = type;
    element.src = source;
    element.dsts = dsts;
    state.elements.append(element);
}

auto
beatorajaJudgeIndexToJudgement(const int index) -> int
{
    switch (index) {
        case 0:
            return 5;
        case 1:
            return 4;
        case 2:
            return 3;
        case 3:
            return 2;
        case 4:
            return 0;
        case 5:
            return 1;
        case 6:
            return 5;
        default:
            return -1;
    }
}

auto
judgeTimerForSide(const int side) -> int
{
    return side == 2 ? 47 : 46;
}

auto
judgeOptionForSideAndJudgement(const int side, const int judgement) -> int
{
    if (judgement < 0 || judgement > 5) {
        return 0;
    }
    const int base = side == 2 ? 261 : 241;
    return base + (5 - judgement);
}

void
loadCoverSources(ConvertState& state,
                 const QVariantMap& skin,
                 const QString& field,
                 std::set<QString>& ids)
{
    for (const auto& coverValue : asList(skin.value(field))) {
        const auto cover = asMap(coverValue);
        const auto id = idString(cover.value(QStringLiteral("id")));
        if (id.isEmpty()) {
            continue;
        }
        state.images[id] = makeImageSource(state, cover);
        ids.insert(id);
    }
}

void
loadJudges(ConvertState& state, const QVariantMap& skin)
{
    for (const auto& judgeValue : asList(skin.value(QStringLiteral("judge")))) {
        const auto judge = asMap(judgeValue);
        const auto id = idString(judge.value(QStringLiteral("id")));
        if (id.isEmpty()) {
            continue;
        }

        const int side = std::clamp(toInt(judge.value(QStringLiteral("index"))) + 1,
                                    1,
                                    2);
        QList<JudgePart> parts;
        int index = 0;
        for (const auto& imageValue : asList(judge.value(QStringLiteral("images")))) {
            const auto image = asMap(imageValue);
            const auto sourceId = idString(image.value(QStringLiteral("id")));
            if (!sourceId.isEmpty()) {
                parts.append(JudgePart{
                  .type = 0,
                  .side = side,
                  .judgementIndex = beatorajaJudgeIndexToJudgement(index),
                  .sourceId = sourceId,
                  .destination = image,
                });
            }
            ++index;
        }

        index = 0;
        for (const auto& numberValue : asList(judge.value(QStringLiteral("numbers")))) {
            const auto number = asMap(numberValue);
            const auto sourceId = idString(number.value(QStringLiteral("id")));
            if (!sourceId.isEmpty()) {
                parts.append(JudgePart{
                  .type = 1,
                  .side = side,
                  .judgementIndex = beatorajaJudgeIndexToJudgement(index),
                  .sourceId = sourceId,
                  .destination = number,
                });
            }
            ++index;
        }

        if (!parts.isEmpty()) {
            state.judges[id] = parts;
        }
    }
}

void
loadSources(ConvertState& state, const QVariantMap& skin)
{
    for (const auto& sourceValue : asList(skin.value(QStringLiteral("source")))) {
        const auto source = asMap(sourceValue);
        const auto id = idString(source.value(QStringLiteral("id")));
        const auto path = source.value(QStringLiteral("path")).toString();
        if (id.isEmpty()) {
            continue;
        }
        state.sourcePaths[id] = resolveSkinPath(state.skinPath.parent_path(),
                                                state.filePaths,
                                                state.settingValues,
                                                path);
    }

    for (const auto& fontValue : asList(skin.value(QStringLiteral("font")))) {
        const auto font = asMap(fontValue);
        const auto id = idString(font.value(QStringLiteral("id")));
        if (id.isEmpty()) {
            continue;
        }
        const auto path = font.value(QStringLiteral("path")).toString();
        const auto resolvedPath = resolveSkinPath(state.skinPath.parent_path(),
                                                  state.filePaths,
                                                  state.settingValues,
                                                  path);
        const bool bitmap =
          resolvedPath.endsWith(QStringLiteral(".fnt"), Qt::CaseInsensitive) ||
          resolvedPath.endsWith(QStringLiteral(".png"), Qt::CaseInsensitive);
        state.fonts[id] = FontDefinition{
          .path = resolvedPath,
          .family = bitmap ? QString{} : resolvedPath,
          .type = toInt(font.value(QStringLiteral("type"))),
          .bitmap = bitmap,
        };
    }

    for (const auto& imageValue : asList(skin.value(QStringLiteral("image")))) {
        const auto image = asMap(imageValue);
        const auto id = idString(image.value(QStringLiteral("id")));
        if (!id.isEmpty()) {
            state.images[id] = makeImageSource(state, image);
        }
    }
    loadCoverSources(state,
                     skin,
                     QStringLiteral("hiddenCover"),
                     state.hiddenCoverIds);
    loadCoverSources(state,
                     skin,
                     QStringLiteral("liftCover"),
                     state.liftCoverIds);

    for (const auto& imageSetValue : asList(skin.value(QStringLiteral("imageset")))) {
        const auto imageSet = asMap(imageSetValue);
        const auto id = idString(imageSet.value(QStringLiteral("id")));
        if (!id.isEmpty()) {
            state.imageSets[id] = makeImageSetSource(state, imageSet);
        }
    }

    for (const auto& value : asList(skin.value(QStringLiteral("value")))) {
        const auto number = asMap(value);
        const auto id = idString(number.value(QStringLiteral("id")));
        if (!id.isEmpty()) {
            state.numbers[id] = makeNumberSource(state, number);
        }
    }
    for (const auto& value : asList(skin.value(QStringLiteral("floatvalue")))) {
        const auto number = asMap(value);
        const auto id = idString(number.value(QStringLiteral("id")));
        if (!id.isEmpty()) {
            state.numbers[id] = makeNumberSource(state, number);
        }
    }

    for (const auto& textValue : asList(skin.value(QStringLiteral("text")))) {
        const auto text = asMap(textValue);
        const auto id = idString(text.value(QStringLiteral("id")));
        if (!id.isEmpty()) {
            state.texts[id] = makeTextSource(state, text);
        }
    }

    for (const auto& sliderValue : asList(skin.value(QStringLiteral("slider")))) {
        const auto slider = asMap(sliderValue);
        const auto id = idString(slider.value(QStringLiteral("id")));
        if (!id.isEmpty()) {
            state.sliders[id] = makeSliderSource(state, slider);
        }
    }

    for (const auto& graphValue : asList(skin.value(QStringLiteral("graph")))) {
        const auto graph = asMap(graphValue);
        const auto id = idString(graph.value(QStringLiteral("id")));
        if (!id.isEmpty()) {
            state.graphs[id] = makeGraphSource(state, graph);
        }
    }
    for (const auto& graphValue : asList(skin.value(QStringLiteral("judgegraph")))) {
        const auto graph = asMap(graphValue);
        const auto id = idString(graph.value(QStringLiteral("id")));
        if (!id.isEmpty()) {
            state.noteCharts[id] = makeNoteChartSource(graph);
        }
    }
    for (const auto& graphValue : asList(skin.value(QStringLiteral("bpmgraph")))) {
        const auto graph = asMap(graphValue);
        const auto id = idString(graph.value(QStringLiteral("id")));
        if (!id.isEmpty()) {
            state.bpmCharts[id] = makeBpmChartSource(graph);
        }
    }
    for (const auto& graphValue : asList(skin.value(QStringLiteral("gaugegraph")))) {
        const auto graph = asMap(graphValue);
        const auto id = idString(graph.value(QStringLiteral("id")));
        if (!id.isEmpty()) {
            state.resultCharts[id] = makeGaugeGraphSource(graph);
        }
    }
    for (const auto& graphValue :
         asList(skin.value(QStringLiteral("timingdistributiongraph")))) {
        const auto graph = asMap(graphValue);
        const auto id = idString(graph.value(QStringLiteral("id")));
        if (!id.isEmpty()) {
            state.resultCharts[id] = makeTimingDistributionGraphSource(graph);
        }
    }
    for (const auto& graphValue :
         asList(skin.value(QStringLiteral("timingvisualizer")))) {
        const auto graph = asMap(graphValue);
        const auto id = idString(graph.value(QStringLiteral("id")));
        if (!id.isEmpty()) {
            state.resultCharts[id] = makeTimingVisualizerSource(graph, 3);
        }
    }
    for (const auto& graphValue :
         asList(skin.value(QStringLiteral("hiterrorvisualizer")))) {
        const auto graph = asMap(graphValue);
        const auto id = idString(graph.value(QStringLiteral("id")));
        if (!id.isEmpty()) {
            state.resultCharts[id] = makeTimingVisualizerSource(graph, 4);
        }
    }

    const auto gauge = asMap(skin.value(QStringLiteral("gauge")));
    const auto gaugeId = idString(gauge.value(QStringLiteral("id")));
    if (!gaugeId.isEmpty()) {
        state.gauges[gaugeId] = makeGaugeSource(state, gauge);
    }

    const auto bga = asMap(skin.value(QStringLiteral("bga")));
    state.bgaId = idString(bga.value(QStringLiteral("id")));
    loadJudges(state, skin);
}

auto
imageListFromIds(ConvertState& state, const QVariantList& ids) -> QVariantList
{
    QVariantList result;
    for (const auto& id : ids) {
        const auto source = imageSourceVariant(state, idString(id));
        if (source.isValid()) {
            result.append(source);
        }
    }
    return result;
}

void
loadNotes(ConvertState& state, const QVariantMap& skin)
{
    const auto note = asMap(skin.value(QStringLiteral("note")));
    if (note.isEmpty()) {
        return;
    }
    state.noteId = idString(note.value(QStringLiteral("id")));
    state.noteSources = imageListFromIds(state, asList(note.value(QStringLiteral("note"))));
    state.mineSources = imageListFromIds(state, asList(note.value(QStringLiteral("mine"))));
    state.lnStartSources =
      imageListFromIds(state, asList(note.value(QStringLiteral("lnstart"))));
    state.lnEndSources =
      imageListFromIds(state, asList(note.value(QStringLiteral("lnend"))));
    state.lnBodySources =
      imageListFromIds(state, asList(note.value(QStringLiteral("lnbody"))));
    auto activeBody =
      imageListFromIds(state, asList(note.value(QStringLiteral("lnbodyActive"))));
    if (activeBody.isEmpty()) {
        activeBody =
          imageListFromIds(state, asList(note.value(QStringLiteral("lnactive"))));
    }
    state.lnBodyActiveSources = activeBody;
    state.hcnStartSources =
      imageListFromIds(state, asList(note.value(QStringLiteral("hcnstart"))));
    state.hcnEndSources =
      imageListFromIds(state, asList(note.value(QStringLiteral("hcnend"))));
    state.hcnBodySources =
      imageListFromIds(state, asList(note.value(QStringLiteral("hcnbody"))));
    auto hcnActiveBody =
      imageListFromIds(state, asList(note.value(QStringLiteral("hcnbodyActive"))));
    const bool hasExplicitHcnActiveBody = !hcnActiveBody.isEmpty();
    if (hcnActiveBody.isEmpty()) {
        hcnActiveBody =
          imageListFromIds(state, asList(note.value(QStringLiteral("hcnactive"))));
    }
    state.hcnBodyActiveSources = hcnActiveBody;
    if (hasExplicitHcnActiveBody) {
        state.hcnBodyReactiveSources =
          imageListFromIds(state, asList(note.value(QStringLiteral("hcnbodyReactive"))));
        state.hcnBodyMissSources =
          imageListFromIds(state, asList(note.value(QStringLiteral("hcnbodyMiss"))));
    } else {
        state.hcnBodyReactiveSources =
          imageListFromIds(state, asList(note.value(QStringLiteral("hcndamage"))));
        state.hcnBodyMissSources =
          imageListFromIds(state, asList(note.value(QStringLiteral("hcnreactive"))));
    }
    if (state.hcnBodyReactiveSources.isEmpty()) {
        state.hcnBodyReactiveSources = state.hcnBodyActiveSources;
    }
    if (state.hcnBodyMissSources.isEmpty()) {
        state.hcnBodyMissSources = state.hcnBodySources;
    }

    for (const auto& animation : asList(note.value(QStringLiteral("dst")))) {
        state.noteDsts.append(animationAsDstList(animation, state));
    }

    int lineIndex = 0;
    for (const auto& destinationValue : asList(note.value(QStringLiteral("group")))) {
        const auto destination = asMap(destinationValue);
        const auto source = imageSourceVariant(state,
                                               idString(destination.value(QStringLiteral("id"))));
        if (source.isValid()) {
            while (state.lineSources.size() <= lineIndex) {
                state.lineSources.append(QVariant{});
            }
            state.lineSources[lineIndex] = source;
        }
        state.lineDsts.append(destinationDsts(destination, state, false));
        ++lineIndex;
    }
}

void
appendBarImage(ConvertState& state,
               const QVariantMap& destination,
               const int kind,
               const int row,
               const int variant,
               const DstCoordinateSpace coordinateSpace)
{
    const auto source =
      imageSourceVariant(state, idString(destination.value(QStringLiteral("id"))));
    if (!source.isValid()) {
        return;
    }
    Lr2SrcBarImage barSource;
    barSource.kind = kind;
    barSource.row = row;
    barSource.variant = variant;
    barSource.source = source;
    barSource.sources = barImageSourceVariants(source);
    const auto dsts = destinationDsts(destination, state, false, coordinateSpace);
    appendElement(state, 3, QVariant::fromValue(barSource), dsts);
    if (kind == Lr2SrcBarImage::BodyOff) {
        for (const auto& dst : dsts) {
            state.barBodyOffDsts[row].append(dst);
        }
        recordBarChildHeight(state, dsts);
    } else if (kind == Lr2SrcBarImage::BodyOn) {
        for (const auto& dst : dsts) {
            state.barBodyOnDsts[row].append(dst);
        }
        recordBarChildHeight(state, dsts);
    } else if (kind == Lr2SrcBarImage::Lamp) {
        state.barLampVariants.insert(variant);
    }
}

void
appendBarText(ConvertState& state,
              const QVariantMap& destination,
              const int titleType)
{
    const auto id = idString(destination.value(QStringLiteral("id")));
    if (!state.texts.contains(id)) {
        return;
    }
    const auto source = barTextSourceFromText(state.texts.value(id), titleType);
    const auto dsts = destinationDsts(
      destination,
      state,
      false,
      DstCoordinateSpace::BarChild);
    appendElement(state, 4, QVariant::fromValue(source), dsts);
    state.barTitleTypes.insert(titleType);
}

void
appendBarNumber(ConvertState& state,
                const QVariantMap& destination,
                const int variant)
{
    const auto id = idString(destination.value(QStringLiteral("id")));
    if (!state.numbers.contains(id)) {
        return;
    }
    Lr2SrcBarNumber source;
    source.kind = Lr2SrcBarNumber::Level;
    source.variant = variant;
    source.source = QVariant::fromValue(state.numbers.value(id));
    appendElement(state,
                  5,
                  QVariant::fromValue(source),
                  destinationDsts(destination,
                                  state,
                                  false,
                                  DstCoordinateSpace::BarChild));
}

void
appendSongList(ConvertState& state, const QVariantMap& songList)
{
    if (songList.isEmpty()) {
        return;
    }
    state.barCenter = toInt(songList.value(QStringLiteral("center")));
    const auto clickableRows = asList(songList.value(QStringLiteral("clickable")));
    for (const auto& clickableRow : clickableRows) {
        const int row = toInt(clickableRow, -1);
        if (row < 0) {
            continue;
        }
        if (state.barAvailableEnd < state.barAvailableStart) {
            state.barAvailableStart = row;
            state.barAvailableEnd = row;
        } else {
            state.barAvailableStart = std::min(state.barAvailableStart, row);
            state.barAvailableEnd = std::max(state.barAvailableEnd, row);
        }
    }

    int row = 0;
    for (const auto& destination : asList(songList.value(QStringLiteral("listoff")))) {
        appendBarImage(state,
                       asMap(destination),
                       Lr2SrcBarImage::BodyOff,
                       row++,
                       0,
                       DstCoordinateSpace::Screen);
    }
    row = 0;
    for (const auto& destination : asList(songList.value(QStringLiteral("liston")))) {
        appendBarImage(state,
                       asMap(destination),
                       Lr2SrcBarImage::BodyOn,
                       row++,
                       0,
                       DstCoordinateSpace::Screen);
    }

    int variant = 0;
    for (const auto& destination : asList(songList.value(QStringLiteral("text")))) {
        appendBarText(state, asMap(destination), variant++);
    }

    variant = 0;
    for (const auto& destination : asList(songList.value(QStringLiteral("level")))) {
        appendBarNumber(state, asMap(destination), variant++);
    }

    const QList<std::pair<QString, int>> imageGroups{
        { QStringLiteral("lamp"), Lr2SrcBarImage::Lamp },
        { QStringLiteral("playerlamp"), Lr2SrcBarImage::MyLamp },
        { QStringLiteral("rivallamp"), Lr2SrcBarImage::RivalLamp },
        { QStringLiteral("trophy"), Lr2SrcBarImage::Rank },
        { QStringLiteral("label"), Lr2SrcBarImage::Label },
    };
    for (const auto& [field, kind] : imageGroups) {
        variant = 0;
        for (const auto& destination : asList(songList.value(field))) {
            appendBarImage(state,
                           asMap(destination),
                           kind,
                           -1,
                           variant++,
                           DstCoordinateSpace::BarChild);
        }
    }

    const auto graphDestination = asMap(songList.value(QStringLiteral("graph")));
    const auto graphId = idString(graphDestination.value(QStringLiteral("id")));
    if (state.graphs.contains(graphId)) {
        auto source = state.graphs.value(graphId);
        source.specialType = Lr2SrcImage::None;
        source.graphType = beatorajaDistributionGraphType(source.graphType);
        appendElement(state,
                      13,
                      QVariant::fromValue(source),
                      destinationDsts(graphDestination,
                                      state,
                                      false,
                                      DstCoordinateSpace::BarChild));
    }
}

auto
isSongListDestination(const QVariantMap& songList,
                      const QVariantMap& destination) -> bool
{
    if (songList.isEmpty()) {
        return false;
    }
    const auto songListId = idString(songList.value(QStringLiteral("id")));
    return !songListId.isEmpty() &&
      idString(destination.value(QStringLiteral("id"))) == songListId;
}

auto
centeredJudgeNumberDestination(const QVariantMap& destination,
                               const Lr2SrcNumber& source) -> QVariantMap
{
    auto result = destination;
    QVariantList animations;
    for (const auto& animationValue : asList(destination.value(QStringLiteral("dst")))) {
        auto animation = asMap(animationValue);
        if (source.keta > 0 && animation.contains(QStringLiteral("w"))) {
            animation[QStringLiteral("x")] =
              toInt(animation.value(QStringLiteral("x"))) -
              toInt(animation.value(QStringLiteral("w"))) * source.keta / 2;
        }
        animations.append(animation);
    }
    result[QStringLiteral("dst")] = animations;
    return result;
}

void
appendJudgeComposite(ConvertState& state, const QString& id)
{
    const auto parts = state.judges.value(id);
    QMap<int, QVariantList> imageDstsByJudgement;

    for (const auto& part : parts) {
        if (part.type != 0 || !state.images.contains(part.sourceId)) {
            continue;
        }
        auto source = state.images.value(part.sourceId);
        const int timer = judgeTimerForSide(part.side);
        source.timer = timer;
        auto dsts = destinationDsts(part.destination, state, true);
        dsts = withJudgeRuntimeState(
          state,
          dsts,
          judgeOptionForSideAndJudgement(part.side, part.judgementIndex),
          timer);
        imageDstsByJudgement[part.judgementIndex] = dsts;
        appendElement(state, 0, QVariant::fromValue(source), dsts);
    }

    for (const auto& part : parts) {
        if (part.type != 1 || !state.numbers.contains(part.sourceId)) {
            continue;
        }
        auto source = state.numbers.value(part.sourceId);
        const int timer = judgeTimerForSide(part.side);
        source.timer = timer;
        source.nowCombo = true;
        source.side = part.side;
        source.judgementIndex = part.judgementIndex;
        auto dsts = destinationDsts(centeredJudgeNumberDestination(part.destination,
                                                                   source),
                                    state,
                                    true,
                                    DstCoordinateSpace::Relative);
        dsts = withBasePosition(dsts,
                                imageDstsByJudgement.value(part.judgementIndex));
        dsts = withJudgeRuntimeState(
          state,
          dsts,
          judgeOptionForSideAndJudgement(part.side, part.judgementIndex),
          timer);
        appendElement(state, 1, QVariant::fromValue(source), dsts);
    }
}

void
appendRegularDestination(ConvertState& state, const QVariantMap& destination)
{
    const auto id = idString(destination.value(QStringLiteral("id")));
    if (id.isEmpty()) {
        return;
    }
    if (state.judges.contains(id)) {
        appendJudgeComposite(state, id);
        return;
    }
    const auto dsts = destinationDsts(destination, state);
    if (dsts.isEmpty()) {
        return;
    }

    if (state.hiddenCoverIds.contains(id) && state.images.contains(id)) {
        appendElement(state,
                      0,
                      QVariant::fromValue(state.images.value(id)),
                      withDstOffsets(dsts, { 3, 5 }));
    } else if (state.liftCoverIds.contains(id) && state.images.contains(id)) {
        appendElement(state,
                      0,
                      QVariant::fromValue(state.images.value(id)),
                      withDstOffsets(dsts, { 3 }));
    } else if (state.images.contains(id)) {
        appendElement(state, 0, QVariant::fromValue(state.images.value(id)), dsts);
    } else if (state.imageSets.contains(id)) {
        appendElement(state, 0, QVariant::fromValue(state.imageSets.value(id)), dsts);
    } else if (state.sliders.contains(id)) {
        appendElement(state, 0, QVariant::fromValue(state.sliders.value(id)), dsts);
    } else if (state.numbers.contains(id)) {
        appendElement(state, 1, QVariant::fromValue(state.numbers.value(id)), dsts);
    } else if (state.texts.contains(id)) {
        appendElement(state, 2, QVariant::fromValue(state.texts.value(id)), dsts);
    } else if (state.graphs.contains(id)) {
        appendElement(state, 6, QVariant::fromValue(state.graphs.value(id)), dsts);
    } else if (state.noteCharts.contains(id)) {
        appendElement(state,
                      11,
                      QVariant::fromValue(state.noteCharts.value(id)),
                      dsts);
    } else if (state.bpmCharts.contains(id)) {
        appendElement(state,
                      12,
                      QVariant::fromValue(state.bpmCharts.value(id)),
                      dsts);
    } else if (state.resultCharts.contains(id)) {
        appendElement(state,
                      10,
                      QVariant::fromValue(state.resultCharts.value(id)),
                      dsts);
    } else if (state.gauges.contains(id)) {
        appendElement(state, 9, QVariant::fromValue(state.gauges.value(id)), dsts);
    } else if (id == state.bgaId) {
        appendElement(state, 7, QVariant{}, dsts);
    } else if (id == state.noteId) {
        Lr2Element element;
        element.type = 8;
        state.elements.append(element);
    } else {
        bool ok = false;
        const int numericId = id.toInt(&ok);
        if (ok) {
            if (const auto builtin = builtinImageSource(numericId)) {
                appendElement(state, 0, QVariant::fromValue(*builtin), dsts);
            }
        }
    }
}

auto
setToVariantList(const std::set<int>& values) -> QVariantList
{
    QVariantList result;
    for (const int value : values) {
        result.append(value);
    }
    return result;
}

auto
barRowsFromState(const ConvertState& state) -> QVariantList
{
    QVariantList rows;
    int maxRow = -1;
    for (auto it = state.barBodyOffDsts.cbegin();
         it != state.barBodyOffDsts.cend();
         ++it) {
        maxRow = std::max(maxRow, it.key());
    }
    for (auto it = state.barBodyOnDsts.cbegin();
         it != state.barBodyOnDsts.cend();
         ++it) {
        maxRow = std::max(maxRow, it.key());
    }
    for (int row = 0; row <= maxRow; ++row) {
        QVariantMap rowData;
        rowData["row"] = row;
        rowData["offDsts"] = state.barBodyOffDsts.value(row);
        rowData["onDsts"] = state.barBodyOnDsts.value(row);
        rows.append(rowData);
    }
    return rows;
}

void
registerCustomObjects(ConvertState& state, const QVariantMap& skin)
{
    if (state.runtime == nullptr) {
        return;
    }

    for (const auto& timerValue : asList(skin.value(QStringLiteral("customTimers")))) {
        const auto timer = asMap(timerValue);
        const int id = toInt(timer.value(QStringLiteral("id")));
        if (id == 0) {
            continue;
        }
        const int callbackId = state.runtime->callbackIdFromVariant(
          timer.value(QStringLiteral("timer")),
          QStringLiteral("timer"));
        state.runtime->registerCustomTimer(id, callbackId);
    }

    for (const auto& eventValue : asList(skin.value(QStringLiteral("customEvents")))) {
        const auto event = asMap(eventValue);
        const int id = toInt(event.value(QStringLiteral("id")));
        if (id == 0) {
            continue;
        }
        const int actionCallbackId = state.runtime->callbackIdFromVariant(
          event.value(QStringLiteral("action")),
          QStringLiteral("action"));
        const int conditionCallbackId = state.runtime->callbackIdFromVariant(
          event.value(QStringLiteral("condition")),
          QStringLiteral("expression"));
        state.runtime->registerCustomEvent(
          id,
          actionCallbackId,
          conditionCallbackId,
          toInt(event.value(QStringLiteral("minInterval"))));
    }
}

auto
convertLuaSkin(const std::filesystem::path& skinPath,
               const QVariantMap& header,
               const QVariantMap& skin,
               const QVariantMap& settingValues,
               const std::set<int>& initialOptions,
               const QSharedPointer<QObject>& luaRuntime = {}) -> Lr2SkinData
{
    ConvertState state;
    state.skinPath = skinPath;
    state.header = header;
    state.settingValues = settingValues;
    state.runtime =
      qobject_cast<BeatorajaLuaRuntime*>(luaRuntime.data());
    state.filePaths = filePathOptionsFromHeader(header);
    state.skinWidth = toInt(skin.value(QStringLiteral("w")),
                            toInt(header.value(QStringLiteral("w")), 1280));
    state.skinHeight = toInt(skin.value(QStringLiteral("h")),
                             toInt(header.value(QStringLiteral("h")), 720));
    state.activeOptions = initialOptions;
    applyBeatorajaLuaDefaultOptions(state.activeOptions);

    const auto selected = selectedOptionsFromHeader(header, settingValues);
    for (auto it = selected.cbegin(); it != selected.cend(); ++it) {
        state.activeOptions.insert(it.value());
    }

    registerCustomObjects(state, skin);
    loadSources(state, skin);
    loadNotes(state, skin);
    const auto songList = asMap(skin.value(QStringLiteral("songlist")));
    for (const auto& destination : asList(skin.value(QStringLiteral("destination")))) {
        const auto destinationMap = asMap(destination);
        if (isSongListDestination(songList, destinationMap)) {
            appendSongList(state, songList);
            continue;
        }
        appendRegularDestination(state, destinationMap);
    }

    return Lr2SkinData{
      .elements = state.elements,
      .skinWidth = state.skinWidth,
      .skinHeight = state.skinHeight,
      .activeOptions = setToVariantList(state.activeOptions),
      .usedOptions = setToVariantList(state.usedOptions),
      .usedElementOptions = setToVariantList(state.usedElementOptions),
      .barLampVariants = setToVariantList(state.barLampVariants),
      .barRows = barRowsFromState(state),
      .barTitleTypes = setToVariantList(state.barTitleTypes),
      .startInput = toInt(skin.value(QStringLiteral("input"))),
      .sceneTime = toInt(skin.value(QStringLiteral("scene"))),
      .loadEnd = toInt(skin.value(QStringLiteral("loadend"))),
      .playStart = toInt(skin.value(QStringLiteral("playstart")), 2000),
      .fadeOut = toInt(skin.value(QStringLiteral("fadeout"))),
      .barCenter = state.barCenter,
      .barAvailableStart = state.barAvailableStart,
      .barAvailableEnd = state.barAvailableEnd,
      .noteSources = state.noteSources,
      .mineSources = state.mineSources,
      .lnStartSources = state.lnStartSources,
      .lnEndSources = state.lnEndSources,
      .lnBodySources = state.lnBodySources,
      .lnBodyActiveSources = state.lnBodyActiveSources,
      .hcnStartSources = state.hcnStartSources,
      .hcnEndSources = state.hcnEndSources,
      .hcnBodySources = state.hcnBodySources,
      .hcnBodyActiveSources = state.hcnBodyActiveSources,
      .hcnBodyReactiveSources = state.hcnBodyReactiveSources,
      .hcnBodyMissSources = state.hcnBodyMissSources,
      .noteDsts = state.noteDsts,
      .lineSources = state.lineSources,
      .lineDsts = state.lineDsts,
      .luaRuntime = luaRuntime,
    };
}

} // namespace

auto
isBeatorajaLuaSkinPath(const std::filesystem::path& path) -> bool
{
    return QString::fromStdString(path.extension().generic_string())
             .compare(QStringLiteral(".luaskin"), Qt::CaseInsensitive) == 0;
}

auto
loadBeatorajaLuaSkinHeader(const std::filesystem::path& skinPath)
  -> BeatorajaLuaSkinHeader
{
    const auto header = executeLuaSkin(skinPath, {}, {}, {}, false);
    if (header.isEmpty()) {
        return {};
    }
    return BeatorajaLuaSkinHeader{
      .raw = header,
      .settingsData = buildSettingsData(header, skinPath),
      .typeId = toInt(header.value(QStringLiteral("type")), -1),
      .title = header.value(QStringLiteral("name")).toString(),
      .maker = header.value(QStringLiteral("author")).toString(),
      .valid = true,
    };
}

auto
parseBeatorajaLuaSkin(const std::filesystem::path& skinPath,
                      const QVariantMap& settingValues,
                      const std::set<int>& initialOptions) -> Lr2SkinData
{
    const auto headerInfo = loadBeatorajaLuaSkinHeader(skinPath);
    if (!headerInfo.valid) {
        return {};
    }

    auto runtime = QSharedPointer<BeatorajaLuaRuntime>::create();
    QVariantMap skin;
    const bool loaded =
      runtime->load(skinPath,
                    headerInfo.raw,
                    settingValues,
                    initialOptions,
                    skin);
    if (!loaded || skin.isEmpty()) {
        auto fallbackOptions = initialOptions;
        applyBeatorajaLuaDefaultOptions(fallbackOptions);
        return Lr2SkinData{
          .skinWidth = toInt(headerInfo.raw.value(QStringLiteral("w")), 1280),
          .skinHeight = toInt(headerInfo.raw.value(QStringLiteral("h")), 720),
          .activeOptions = setToVariantList(fallbackOptions),
        };
    }

    return convertLuaSkin(skinPath,
                          headerInfo.raw,
                          skin,
                          settingValues,
                          initialOptions,
                          qSharedPointerObjectCast<QObject>(runtime));
}

} // namespace gameplay_logic::lr2_skin

#include "BeatorajaLuaSkin.moc"
