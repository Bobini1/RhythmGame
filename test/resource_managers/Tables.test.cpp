#include "resource_managers/Tables.h"
#include "resource_managers/ChartFolderModel.h"
#include "db/SqliteCppDb.h"
#include "resource_managers/DefineDb.h"
#include "support/QStringToPath.h"

#include <catch2/catch_test_macros.hpp>
#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QPersistentModelIndex>
#include <QTemporaryDir>
#include <algorithm>
#include <cstring>

namespace {
void
ensureApplication()
{
    if (!QCoreApplication::instance()) {
        static int argc = 1;
        static char name[] = "Tables.test";
        static char* argv[] = { name, nullptr };
        static QCoreApplication application(argc, argv);
    }
}

class Reply final : public QNetworkReply
{
    QByteArray bytes;
    qint64 position{};

  public:
    bool aborted{};
    Reply(const QNetworkRequest& request, QObject* parent)
      : QNetworkReply(parent)
    {
        setRequest(request);
        setUrl(request.url());
        open(QIODevice::ReadOnly);
    }
    void abort() override { aborted = true; }
    void complete(QByteArray body, const QUrl& effectiveUrl = {})
    {
        bytes = std::move(body);
        if (!effectiveUrl.isEmpty()) {
            setUrl(effectiveUrl);
        }
        setFinished(true);
        emit readyRead();
        emit finished();
    }
    qint64 bytesAvailable() const override
    {
        return bytes.size() - position + QNetworkReply::bytesAvailable();
    }
    qint64 readData(char* buffer, qint64 size) override
    {
        const auto count = std::min(size, bytes.size() - position);
        if (count <= 0)
            return -1;
        std::memcpy(buffer, bytes.constData() + position, count);
        position += count;
        return count;
    }
};

class Network final : public QNetworkAccessManager
{
  public:
    QList<QPointer<Reply>> requests;
    QNetworkReply* createRequest(Operation,
                                 const QNetworkRequest& request,
                                 QIODevice*) override
    {
        auto* reply = new Reply(request, this);
        requests.append(reply);
        return reply;
    }
};

auto
header(const QJsonArray& levels = { "1", "2" }) -> QByteArray
{
    return QJsonDocument(
             QJsonObject{
               { "name", "Test" },
               { "data_url", "data.json" },
               { "level_order", levels },
               { "course",
                 QJsonArray{ QJsonArray{ QJsonObject{
                   { "name", "Course" },
                   { "md5", QJsonArray{ "A" } },
                   { "constraint",
                     QJsonArray{ "gauge_lr2", "grade_mirror" } } } } } } })
      .toJson();
}

auto
data(const QStringList& levels = { "1", "2" }) -> QByteArray
{
    QJsonArray result;
    for (const auto& level : levels) {
        result.append(QJsonObject{
          { "level", level }, { "md5", level }, { "title", level } });
    }
    return QJsonDocument(result).toJson();
}

auto
tableAt(const resource_managers::Tables& tables, int row = 0)
  -> resource_managers::Table
{
    return tables.data(tables.index(row), Qt::DisplayRole)
      .value<resource_managers::Table>();
}
}

TEST_CASE("Table reload replaces levels, charts and courses", "[tables]")
{
    ensureApplication();
    QTemporaryDir cache;
    Network network;
    auto tables = std::make_unique<resource_managers::Tables>(
      &network, QDir(cache.path()), nullptr);
    tables->add(QUrl("https://example.test/table.json"));
    network.requests.back()->complete(header());
    network.requests.back()->complete(data());
    for (int i = 0; i < 2; ++i) {
        tables->reload(0);
        network.requests.back()->complete(header());
        network.requests.back()->complete(data());
        const auto table = tableAt(*tables);
        REQUIRE(table.levels.size() == 2);
        CHECK(table.levels[0].entries.size() == 1);
        CHECK(table.courses.size() == 1);
    }
    tables->reload(0);
    network.requests.back()->complete(
      R"({"name":"Updated","data_url":"data.json","level_order":["2"]})");
    network.requests.back()->complete(data({ "2" }));
    const auto updated = tableAt(*tables);
    REQUIRE(updated.levels.size() == 1);
    CHECK(updated.levels[0].name == "2");
    CHECK(updated.courses.empty());
    tables.reset(); // Wait for the serialized cache writes.
    resource_managers::Tables reopened(&network, QDir(cache.path()), nullptr);
    CHECK(tableAt(reopened).name == "Updated");
    CHECK(tableAt(reopened).levels.size() == 1);
}

TEST_CASE("Removing a table invalidates each pending download stage",
          "[tables]")
{
    ensureApplication();
    QTemporaryDir cache;
    Network network;
    auto tables = std::make_unique<resource_managers::Tables>(
      &network, QDir(cache.path()), nullptr);
    const auto url = QUrl("https://example.test/table.json");
    tables->add(url);
    SECTION("Initial response") {}
    SECTION("Data response")
    {
        network.requests.back()->complete(header());
    }
    const auto old = network.requests.back();
    tables->removeAt(0);
    REQUIRE(old->aborted);
    const auto requestCount = network.requests.size();
    old->complete(header());
    CHECK(network.requests.size() == requestCount);
    CHECK(tables->rowCount({}) == 0);
    tables.reset();
    resource_managers::Tables reopened(&network, QDir(cache.path()), nullptr);
    CHECK(reopened.rowCount({}) == 0);
}

TEST_CASE("Readding a table ignores replies from its previous load", "[tables]")
{
    ensureApplication();
    QTemporaryDir cache;
    Network network;
    resource_managers::Tables tables(&network, QDir(cache.path()), nullptr);
    const auto url = QUrl("https://example.test/table.json");
    tables.add(url);
    network.requests.back()->complete(header());
    const auto oldData = network.requests.back();
    tables.add(url);
    network.requests.back()->complete(header());
    network.requests.back()->complete(data({ "2" }));
    oldData->complete(data({ "1" }));
    REQUIRE(tableAt(tables).levels.size() == 1);
    CHECK(tableAt(tables).levels.front().name == "2");
}

TEST_CASE("Table data uses the effective header URL as its base", "[tables]")
{
    ensureApplication();
    QTemporaryDir cache;
    Network network;
    resource_managers::Tables tables(&network, QDir(cache.path()), nullptr);
    SECTION("HTML meta link")
    {
        tables.add(QUrl("https://example.test/table.html"));
        network.requests.back()->complete(
          R"(<html><head><meta name="bmstable" content="json/header.json"></head></html>)");
        CHECK(network.requests.back()->url() ==
              QUrl("https://example.test/json/header.json"));
        network.requests.back()->complete(header());
        CHECK(network.requests.back()->url() ==
              QUrl("https://example.test/json/data.json"));
    }
    SECTION("Redirected header")
    {
        tables.add(QUrl("https://example.test/table.json"));
        network.requests.back()->complete(
          header(), QUrl("https://cdn.example.test/current/header.json"));
        CHECK(network.requests.back()->url() ==
              QUrl("https://cdn.example.test/current/data.json"));
    }
}

TEST_CASE("Table moves preserve persistent indexes and cached order",
          "[tables]")
{
    ensureApplication();
    QTemporaryDir cache;
    Network network;
    auto tables = std::make_unique<resource_managers::Tables>(
      &network, QDir(cache.path()), nullptr);
    for (const auto* name : { "a", "b", "c" }) {
        tables->add(QUrl(QString("https://example.test/%1.json").arg(name)));
    }
    const auto b = QPersistentModelIndex(tables->index(1));
    const auto c = QPersistentModelIndex(tables->index(2));
    tables->reorder(0, 2);
    CHECK(b.row() == 0);
    CHECK(c.row() == 1);
    CHECK(b.data().value<resource_managers::Table>().url.path() == "/b.json");
    CHECK(c.data().value<resource_managers::Table>().url.path() == "/c.json");
    tables->reorder(1, 1);
    CHECK(c.row() == 1);
    tables->reorder(2, 0);
    CHECK(b.row() == 1);
    tables.reset();
    resource_managers::Tables reopened(&network, QDir(cache.path()), nullptr);
    CHECK(tableAt(reopened, 0).url.path() == "/a.json");
    CHECK(tableAt(reopened, 1).url.path() == "/b.json");
    CHECK(tableAt(reopened, 2).url.path() == "/c.json");
}

TEST_CASE("Mixed table levels have a stable order", "[tables]")
{
    ensureApplication();
    QTemporaryDir cache;
    Network network;
    resource_managers::Tables tables(&network, QDir(cache.path()), nullptr);
    auto levels = QStringList{ "10", "10+", "9" };
    do {
        tables.add(QUrl("https://example.test/table.json"));
        network.requests.back()->complete(header({}));
        network.requests.back()->complete(data(levels));
        auto names = QStringList{};
        for (const auto& level : tableAt(tables).levels)
            names.append(level.name);
        CHECK(names == QStringList{ "9", "10", "10+" });
    } while (std::next_permutation(levels.begin(), levels.end()));
}

TEST_CASE("Table search accepts MD5 and SHA-256 identifiers after download and "
          "cache load",
          "[tables][TableSearch]")
{
    ensureApplication();
    QTemporaryDir cache;
    Network network;
    const auto md5 = QString(32, 'a');
    const auto sha256 = QString(64, 'b');
    const auto sha256Only = QString(64, 'c');
    const auto url = QUrl("https://example.test/table.json");
    auto tables = std::make_unique<resource_managers::Tables>(
      &network, QDir(cache.path()), nullptr);
    tables->add(url);
    network.requests.back()->complete(header());
    network.requests.back()->complete(
      QJsonDocument(QJsonArray{ QJsonObject{ { "level", "1" },
                                             { "md5", md5 },
                                             { "sha256", sha256 },
                                             { "title", "First" },
                                             { "subtitle", "Subtitle" } },
                                QJsonObject{ { "level", "2" },
                                             { "md5", md5 },
                                             { "sha256", sha256 },
                                             { "title", "Second" } },
                                QJsonObject{ { "level", "1" },
                                             { "sha256", sha256Only },
                                             { "title", "SHA-256 only" } },
                                QJsonObject{ { "level", "1" },
                                             { "title", "No identifier" } } })
        .toJson());

    const auto checkSearch = [&](resource_managers::Tables& source) {
        for (const auto& id : { md5,
                                md5.toUpper(),
                                "sha256/" + sha256,
                                "SHA256/" + sha256.toUpper() }) {
            const auto matches = source.search(id);
            REQUIRE(matches.size() == 2);
            CHECK(matches[0].tableName == "Test");
            CHECK(matches[0].tableUrl == url);
            CHECK(matches[0].levelName == "1");
            CHECK(matches[0].entry.title == "First");
            CHECK(matches[0].entry.subtitle == "Subtitle");
            CHECK(matches[1].levelName == "2");
            CHECK(matches[1].entry.title == "Second");
        }
        const auto missing = source.search("sha256/" + sha256Only);
        REQUIRE(missing.size() == 1);
        CHECK(missing[0].entry.title == "SHA-256 only");
        CHECK(source.search("").empty());
        CHECK(source.search("sha256/").empty());
        CHECK(source.search("sha256/" + QString(64, 'd')).empty());
    };
    checkSearch(*tables);
    tables.reset();
    resource_managers::Tables reopened(&network, QDir(cache.path()), nullptr);
    checkSearch(reopened);

    reopened.reload(0);
    network.requests.back()->complete(header());
    network.requests.back()->complete(data());
    CHECK(reopened.search(md5).empty());
    CHECK(reopened.search("sha256/" + sha256).empty());
    CHECK(reopened.search("sha256/" + sha256Only).empty());
}

TEST_CASE("Missing course stage identifiers find metadata in external tables",
          "[tables][TableSearch]")
{
    ensureApplication();
    QTemporaryDir cache;
    Network network;
    db::SqliteCppDb database(
      support::qStringToPath(cache.filePath("songs.sqlite")));
    resource_managers::defineDb(database);
    resource_managers::Tables tables(&network, QDir(cache.path()), &database);
    const auto md5 = QString(32, 'd');
    const auto sha256 = QString(64, 'e');
    resource_managers::Level level{ &database, "3" };
    level.addEntry({ .title = "Missing MD5 chart", .md5 = md5 });
    level.addEntry({ .title = "Missing SHA-256 chart", .sha256 = sha256 });
    resource_managers::Table table;
    table.name = "External";
    table.symbol = "EX";
    table.levels.append(level);
    table.url = QUrl("https://example.test/external.json");
    table.status = resource_managers::Table::Loaded;
    table.managedExternally = true;
    tables.setExternalTables({ table });

    resource_managers::Course course{ &database };
    course.md5s = { md5.toUpper(), "", md5.toUpper() };
    course.sha256s = { "", sha256.toUpper(), "" };
    const auto stages = course.loadCharts();
    REQUIRE(stages.size() == 3);
    const auto titles = QStringList{ "Missing MD5 chart",
                                     "Missing SHA-256 chart",
                                     "Missing MD5 chart" };
    for (qsizetype i = 0; i < stages.size(); ++i) {
        REQUIRE(stages[i].metaType() == QMetaType::fromType<QString>());
        const auto matches = tables.search(stages[i].toString());
        REQUIRE(matches.size() == 1);
        CHECK(matches[0].entry.title == titles[i]);
        CHECK(matches[0].entry.level == "3");
        CHECK(matches[0].tableName == "External");
        CHECK(matches[0].symbol == "EX");
    }
    tables.setExternalTables({});
    CHECK(tables.search(md5).empty());
    CHECK(tables.search("sha256/" + sha256).empty());
}

TEST_CASE(
  "Tables announce committed contents without publishing loading placeholders",
  "[tables]")
{
    ensureApplication();
    QTemporaryDir cache;
    Network network;
    resource_managers::Tables tables(&network, QDir(cache.path()), nullptr);
    int changes = 0;
    QObject::connect(
      &tables, &resource_managers::Tables::contentsChanged, [&] { ++changes; });
    tables.add(QUrl("https://example.test/header.json"));
    CHECK(changes == 0);
    network.requests.back()->complete(header());
    CHECK(changes == 0);
    network.requests.back()->complete(data());
    CHECK(changes == 1);
    tables.reload(0);
    CHECK(changes == 1);
    network.requests.back()->complete(header());
    network.requests.back()->complete(data());
    CHECK(changes == 2);
    tables.removeAt(0);
    CHECK(changes == 3);
}

TEST_CASE("SHA256-only courses preserve their stage identifiers",
          "[tables][Course]")
{
    resource_managers::Course course{};
    course.sha256s = { QString(64, 'a'), QString(64, 'b') };
    const auto identifiers = QStringList{ "sha256/" + course.sha256s[0],
                                          "sha256/" + course.sha256s[1] };
    CHECK(course.getChartIds() == identifiers);
    CHECK(course.getIdentifier() == identifiers.join(" ") + "+");
}

TEST_CASE("Selection recognizes SHA256-only entries after metadata changes",
          "[tables][ChartFolderModel]")
{
    resource_managers::ChartFolderModel model;
    resource_managers::Entry original{ .title = "Old title",
                                       .sha256 = QString(64, 'a') };
    auto refreshed = original;
    refreshed.title = "New title";
    resource_managers::Entry unrelated{ .title = "Other",
                                        .sha256 = QString(64, 'b') };
    const QVariantList entries{ QVariant::fromValue(unrelated),
                                QVariant::fromValue(refreshed) };
    CHECK(model.indexOfItem(entries, QVariant::fromValue(original)) == 1);
    CHECK(model.indexOfItem(
            entries, QVariant::fromValue(resource_managers::Entry{})) == -1);
}
