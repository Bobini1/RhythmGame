//
// Created by PC on 05/03/2025.
//

#include "Tables.h"

#include "gameplay_logic/ChartData.h"
#include "support/GeneratePermutation.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <libxml2/libxml/HTMLparser.h>
#include <libxml2/libxml/xpath.h>
#include <QUrl>
#include <spdlog/spdlog.h>
#include <spdlog/stopwatch.h>
auto
getBmstableLink(const QByteArray& html) -> QString
{
    // Parse the HTML content using htmlReadMemory
    const htmlDocPtr doc =
      htmlReadMemory(html.data(),
                     html.size(),
                     NULL,
                     NULL,
                     HTML_PARSE_RECOVER | HTML_PARSE_NOBLANKS |
                       HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);
    if (doc == nullptr) {
        spdlog::error("Failed to parse HTML document");
        return {};
    }

    auto* xpathCtx = xmlXPathNewContext(doc);
    if (xpathCtx == nullptr) {
        spdlog::error("Failed to create XPath context");
        xmlFreeDoc(doc);
        return {};
    }

    const auto* xpathExpr =
      reinterpret_cast<const xmlChar*>("//meta[@name='bmstable']");
    auto* xpathObj = xmlXPathEvalExpression(xpathExpr, xpathCtx);
    if (xpathObj == nullptr) {
        spdlog::error("Failed to evaluate XPath expression");
        xmlXPathFreeContext(xpathCtx);
        xmlFreeDoc(doc);
        return {};
    }

    if (const auto* nodes = xpathObj->nodesetval;
        nodes != nullptr && nodes->nodeNr > 0) {
        xmlChar* content = xmlGetProp(
          nodes->nodeTab[0], reinterpret_cast<const xmlChar*>("content"));
        auto bmstable = QString::fromUtf8(content);
        xmlFree(content);
        xmlXPathFreeObject(xpathObj);
        xmlXPathFreeContext(xpathCtx);
        xmlFreeDoc(doc);
        return bmstable;
    }

    // Clean up
    xmlXPathFreeObject(xpathObj);
    xmlXPathFreeContext(xpathCtx);
    xmlFreeDoc(doc);
    return {};
}

auto
resource_managers::Level::getEntries() const -> QVariantList
{
    auto list = QVariantList{};
    for (const auto& entry : entries) {
        list.append(QVariant::fromValue(entry));
    }
    return list;
}
auto
resource_managers::Level::loadCharts() const -> QVariantList
{
    if (entries.empty()) {
        return {};
    }

    auto sw = spdlog::stopwatch{};
    auto ret = QVariantList{};
    auto md5List = QStringList{};

    for (const auto& chart : entries) {
        ret.append(QVariant::fromValue(chart));
        md5List.append(chart.md5.toUpper());
    }

    struct ChartDTOWithIndex
    {
        int64_t index;
        gameplay_logic::ChartData::DTO chartData;
    };

    // Create a single query with an IN clause
    auto queryStr = std::string("WITH md5_list(md5, idx) AS (VALUES ");
    auto value = std::string("");
    for (const auto& md5 : md5List) {
        value += "(?, ?), ";
    }
    if (value.size() > 2) {
        value =
          value.substr(0, value.size() - 2); // Remove the last comma and space
    }
    queryStr += value;

    queryStr += ") SELECT min(md5_list.idx), charts.id, charts.title, "
                "charts.artist, charts.subtitle, charts.subartist, "
                "charts.genre, charts.stage_file, charts.banner, "
                "charts.back_bmp, charts.rank, charts.total, "
                "charts.play_level, charts.difficulty, charts.is_random, "
                "charts.random_sequence, charts.normal_note_count, "
                "charts.ln_count, charts.mine_count, charts.length, "
                "charts.initial_bpm, charts.max_bpm, "
                "charts.min_bpm, charts.path, charts.directory, charts.sha256, "
                "charts.md5, charts.keymode "
                "FROM md5_list JOIN charts ON md5_list.md5 = charts.md5 GROUP "
                "BY md5_list.idx";

    auto query = db->createStatement(queryStr);
    for (const auto& [index, md5] : std::ranges::views::enumerate(md5List)) {
        query.bind(index * 2 + 1, md5.toStdString());
        query.bind(index * 2 + 2, index);
    }
    const auto queryResult = query.executeAndGetAll<ChartDTOWithIndex>();
    for (const auto& result : queryResult) {
        auto chartData = gameplay_logic::ChartData::load(result.chartData);
        ret[result.index] = QVariant::fromValue(chartData.release());
    }
    // sort by title, subtitle
    std::ranges::sort(ret, [](QVariant& a, QVariant& b) {
        auto getTitle = [](QVariant& chart) {
            if (chart.canView<gameplay_logic::ChartData*>()) {
                return chart.value<gameplay_logic::ChartData*>()->getTitle();
            }
            if (chart.canView<Entry>()) {
                return chart.view<Entry>().title;
            }
            throw std::runtime_error(
              "ChartData or Entry not found in QVariant");
        };
        const auto titleA = getTitle(a);
        const auto titleB = getTitle(b);
        auto getSubtitle = [](QVariant& chart) {
            if (chart.canView<gameplay_logic::ChartData*>()) {
                return chart.value<gameplay_logic::ChartData*>()->getSubtitle();
            }
            if (chart.canView<Entry>()) {
                return chart.view<Entry>().subtitle;
            }
            throw std::runtime_error(
              "ChartData or Entry not found in QVariant");
        };
        const auto subtitleA = getSubtitle(a);
        const auto subtitleB = getSubtitle(b);
        if (titleA == titleB) {
            return subtitleA < subtitleB;
        }
        return titleA < titleB;
    });

    spdlog::debug("Loaded {} charts in {} s", queryResult.size(), sw);
    return ret;
}
auto
resource_managers::Course::getTrophies() const -> QVariantList
{
    auto list = QVariantList{};
    for (const auto& trophy : trophies) {
        list.append(QVariant::fromValue(trophy));
    }
    return list;
}
auto
resource_managers::Table::getLevels() const -> QVariantList
{
    auto list = QVariantList{};
    for (const auto& level : levels) {
        list.append(QVariant::fromValue(level));
    }
    return list;
}
auto
resource_managers::Table::getCourses() const -> QVariantList
{
    auto list = QVariantList{};
    for (const auto& courseList : courses) {
        auto chartList = QVariantList{};
        for (const auto& course : courseList) {
            chartList.append(QVariant::fromValue(course));
        }
        list.append(chartList);
    }
    return list;
}
void
resource_managers::Tables::handleInitialReply(QNetworkReply* reply,
                                              const QUrl& url)
{
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        spdlog::error("Network error: {}", reply->errorString().toStdString());
        setErrorFlag(url);
        return;
    }

    const auto type = reply->header(QNetworkRequest::ContentTypeHeader);
    if (reply->url().toString().endsWith(".json") ||
        type.toString().startsWith("application/json")) {
        handleHeaderReply(url, reply->readAll());
        return;
    }

    const QByteArray html = reply->readAll();
    auto bmstableLink = getBmstableLink(html);
    if (bmstableLink.isEmpty()) {
        spdlog::error("No bmstable link found");
        setErrorFlag(url);
        return;
    }

    auto headerUrl = reply->url().resolved(QUrl(bmstableLink));
    const auto request = QNetworkRequest(headerUrl);
    auto* newReply = networkManager->get(request);
    connect(newReply, &QNetworkReply::finished, this, [this, newReply, url] {
        newReply->deleteLater();
        if (newReply->error() == QNetworkReply::NoError) {
            handleHeaderReply(url, newReply->readAll());
        } else {
            spdlog::error("Network error: {}",
                          newReply->errorString().toStdString());
            setErrorFlag(url);
        }
    });
}
void
resource_managers::Tables::setErrorFlag(const QUrl& url)
{
    for (const auto& [index, table] : std::ranges::views::enumerate(tables)) {
        if (table.url == url) {
            table.status = Table::Error;
            emit dataChanged(createIndex(index, 0), createIndex(index, 0));
        }
    }
}
static void
save(const QDir& tableLocation,
     const QUrl& url,
     const QString& keyName,
     std::optional<QJsonValue> json)
{
    // create table location if it doesn't exist
    if (!tableLocation.mkpath(".")) {
        spdlog::error("Failed to create folder for tables file: {}",
                      tableLocation.filePath("tables.json").toStdString());
    }
    const auto filePath = tableLocation.filePath("tables.json");
    QFile file(filePath);
    if (!file.open(QIODevice::ReadWrite)) {
        spdlog::error("Failed to open file for reading and writing: {}",
                      filePath.toStdString());
        return;
    }
    auto existingJson = QJsonDocument::fromJson(file.readAll());
    auto existingArray = existingJson.array();
    auto thisTableObject = QJsonObject{};
    auto entryIndex = -1;
    for (const auto& [index, entry] :
         std::ranges::views::enumerate(existingJson.array())) {
        if (entry.toObject()["url"].toString() == url.toString()) {
            thisTableObject = entry.toObject();
            entryIndex = index;
        }
    }
    if (json) {
        thisTableObject[keyName] = *json;
    }
    if (entryIndex == -1) {
        thisTableObject["url"] = url.toString();
        existingArray.append(thisTableObject);
    } else {
        existingArray[entryIndex] = thisTableObject;
    }
    existingJson.setArray(existingArray);
    file.resize(0);
    file.write(existingJson.toJson());
}
static void
reorderInFile(const QDir& tableLocation, const QUrl& url1, const QUrl& url2)
{
    const auto filePath = tableLocation.filePath("tables.json");
    QFile file(filePath);
    if (!file.open(QIODevice::ReadWrite)) {
        spdlog::error("Failed to open file for reading and writing: {}",
                      filePath.toStdString());
        return;
    }
    auto existingJson = QJsonDocument::fromJson(file.readAll());
    auto existingArray = existingJson.array();

    int index1 = -1;
    int index2 = -1;
    for (const auto& [index, entry] :
         std::ranges::views::enumerate(existingArray)) {
        if (entry.toObject()["url"].toString() == url1.toString()) {
            index1 = index;
        }
        if (entry.toObject()["url"].toString() == url2.toString()) {
            index2 = index;
        }
    }

    if (index1 != -1 && index2 != -1) {
        const auto temp = existingArray[index1].toObject();
        existingArray[index1] = existingArray[index2].toObject();
        existingArray[index2] = temp;
        existingJson.setArray(existingArray);
        file.resize(0);
        file.write(existingJson.toJson());
    } else {
        spdlog::error(
          "Failed to find one or both entries to reorder in the JSON file");
    }
}

void
resource_managers::Tables::handleData(const QUrl& url, const QJsonArray& data)
{
    for (const auto& [index, table] : std::ranges::views::enumerate(tables)) {
        if (table.url == url) {
            auto map = QHash<QString, Level*>{};
            for (auto& level : table.levels) {
                map[level.name] = &level;
            }
            // levels not declared in level_order
            auto extraLevels = QHash<QString, Level>{};
            for (const auto& chart : data) {
                auto chartObj = chart.toObject();
                auto levelStr = chartObj["level"].toString();
                auto* level = map[levelStr];
                if (level == nullptr) {
                    level = &extraLevels[levelStr];
                    level->db = db;
                    level->name = levelStr;
                }
                auto entry = Entry{};
                entry.md5 = chartObj["md5"].toString();
                entry.sha256 = chartObj["sha256"].toString();
                entry.title = chartObj["title"].toString();
                entry.artist = chartObj["artist"].toString();
                entry.subtitle = chartObj["subtitle"].toString();
                entry.subartist = chartObj["subartist"].toString();
                entry.level = levelStr;
                entry.url = chartObj["url"].toString();
                entry.urlDiff = chartObj["url_diff"].toString();
                entry.comment = chartObj["comment"].toString();

                level->entries.push_back(entry);
            }
            auto extraLevelValues = extraLevels.values();
            std::ranges::sort(extraLevelValues,
                              [](const auto& a, const auto& b) {
                                  bool ok1;
                                  bool ok2;
                                  auto aNum = a.name.toDouble(&ok1);
                                  auto bNum = b.name.toDouble(&ok2);
                                  if (ok1 && ok2) {
                                      return aNum < bNum;
                                  }
                                  return a.name < b.name;
                              });
            for (const auto& level : extraLevelValues) {
                table.levels.push_back(level);
            }
            if (tables[index].status != Table::Error) {
                tables[index].status = Table::Loaded;
            }
            emit dataChanged(createIndex(index, 0), createIndex(index, 0));
        }
    }
}
void
resource_managers::Tables::handleHeaderReply(const QUrl& url,
                                             const QByteArray& reply)
{
    auto json = QJsonDocument::fromJson(reply);
    if (json.isNull()) {
        spdlog::error("Failed to parse json");
        setErrorFlag(url);
        return;
    }
    const auto dataUrl = json["data_url"].toString();
    if (dataUrl.isEmpty()) {
        spdlog::error("No data url found");
        setErrorFlag(url);
    }
    fileOperationThreadPool.start(
      [tableLocation = tableLocation, url, obj = json.object()] {
          save(tableLocation, url, QStringLiteral("header"), obj);
      });
    handleHeader(url, json.object());
    auto dataRequest = QNetworkRequest(url.resolved(QUrl(dataUrl)));
    auto* dataReply = networkManager->get(dataRequest);
    connect(dataReply, &QNetworkReply::finished, this, [this, dataReply, url] {
        dataReply->deleteLater();
        if (dataReply->error() != QNetworkReply::NoError) {
            spdlog::error("Network error: {}",
                          dataReply->errorString().toStdString());
            setErrorFlag(url);
            return;
        }
        const auto json = QJsonDocument::fromJson(dataReply->readAll());
        if (json.isNull()) {
            spdlog::error("Failed to parse json");
            setErrorFlag(url);
            return;
        }
        fileOperationThreadPool.start(
          [tableLocation = tableLocation, url, obj = json.array()] {
              save(tableLocation, url, QStringLiteral("data"), obj);
          });
        handleData(url, json.array());
    });
}
void
resource_managers::Tables::handleHeader(const QUrl& url,
                                        const QJsonObject& header)
{
    for (const auto& [index, table] : std::ranges::views::enumerate(tables)) {
        if (table.url == url) {
            table.name = header["name"].toString();
            table.tag = header["tag"].toString();
            table.symbol = header["symbol"].toString();
            table.keymode = header["keymode"];
            for (const auto& level : header["level_order"].toArray()) {
                auto levelObj = Level{};
                levelObj.name = level.isString() ? level.toString()
                                                : QString::number(level.toInt());
                levelObj.db = db;
                table.levels.push_back(levelObj);
            }

            for (const auto& courseList : header["course"].toArray()) {
                auto courseListObj = QList<Course>{};
                for (const auto& course : courseList.toArray()) {
                    auto courseObj = Course{};
                    courseObj.name = course.toObject()["name"].toString();
                    for (const auto& md5 : course.toObject()["md5"].toArray()) {
                        courseObj.md5s.push_back(md5.toString());
                    }
                    for (const auto& trophy :
                         course.toObject()["trophy"].toArray()) {
                        auto trophyObj = Trophy{};
                        trophyObj.name = trophy.toObject()["name"].toString();
                        trophyObj.missRate =
                          trophy.toObject()["missrate"].toDouble();
                        trophyObj.scoreRate =
                          trophy.toObject()["scorerate"].toDouble();
                        courseObj.trophies.push_back(trophyObj);
                    }
                    for (const auto& constraint :
                         course.toObject()["constraint"].toArray()) {
                        courseObj.constraints.push_back(constraint.toString());
                    }
                    courseListObj.push_back(courseObj);
                }
                table.courses.push_back(courseListObj);
            }
            emit dataChanged(createIndex(index, 0), createIndex(index, 0));
        }
    }
}
resource_managers::Tables::Tables(QNetworkAccessManager* networkManager,
                                  const QDir& tableLocation,
                                  db::SqliteCppDb* db,
                                  QObject* parent)
  : QAbstractListModel(parent)
  , networkManager(networkManager)
  , tableLocation(tableLocation)
  , db(db)
{
    fileOperationThreadPool.setMaxThreadCount(1);
    if (!tableLocation.mkpath(".")) {
        spdlog::error("Failed to create folder for tables file: {}",
                      tableLocation.filePath("tables.json").toStdString());
    }
    const auto filePath = tableLocation.filePath("tables.json");
    QFile file(filePath);
    if (!file.open(QIODevice::ReadWrite)) {
        spdlog::error("Failed to open/create tables file: {}",
                      filePath.toStdString());
        return;
    }
    file.close();
    if (!file.open(QIODevice::ReadOnly)) {
        spdlog::warn("Failed to open file for reading: {}",
                     file.fileName().toStdString());
        return;
    }
    for (const auto json = QJsonDocument::fromJson(file.readAll());
         const auto& table : json.array()) {
        const auto& tableObj = table.toObject();
        tables.push_back({ .url = QUrl(table.toObject()["url"].toString()) });
        if (tableObj.contains("header")) {
            handleHeader(QUrl(tableObj["url"].toString()),
                         tableObj["header"].toObject());
        } else {
            setErrorFlag(QUrl(tableObj["url"].toString()));
        }
        if (tableObj.contains("data")) {
            handleData(QUrl(tableObj["url"].toString()),
                       tableObj["data"].toArray());
        } else {
            setErrorFlag(QUrl(tableObj["url"].toString()));
        }
    }
}
auto
resource_managers::Tables::rowCount(const QModelIndex& parent) const -> int
{
    return tables.size();
}
auto
resource_managers::Tables::data(const QModelIndex& index, const int role) const
  -> QVariant
{
    if (!index.isValid()) {
        return {};
    }
    if (index.row() >= tables.size()) {
        return {};
    }
    const auto& table = tables[index.row()];
    switch (role) {
        case Qt::DisplayRole:
            return QVariant::fromValue(table);
        default:
            break;
    }
    return {};
}
void
removeFromFile(const QDir& tableLocation, const QUrl& url)
{
    const auto filePath = tableLocation.filePath("tables.json");
    QFile file(filePath);
    if (!file.open(QIODevice::ReadWrite)) {
        spdlog::error("Failed to open file for reading and writing: {}",
                      filePath.toStdString());
        return;
    }
    auto existingJson = QJsonDocument::fromJson(file.readAll());
    auto existingArray = existingJson.array();
    // don't trust the file to have the entry at the same index
    for (const auto& [index, entry] :
         std::ranges::views::enumerate(existingJson.array())) {
        if (entry.toObject()["url"].toString() == url.toString()) {
            existingArray.removeAt(index);
        }
    }
    existingJson.setArray(existingArray);
    file.resize(0);
    file.write(existingJson.toJson());
}
void
resource_managers::Tables::removeAt(int index)
{
    if (index >= tables.size()) {
        return;
    }
    if (index < 0) {
        return;
    }
    spdlog::info("Removing table: {}",
                 tables[index].url.toString().toStdString());

    fileOperationThreadPool.start(
      [tableLocation = tableLocation, url = tables[index].url] {
          removeFromFile(tableLocation, url);
      });

    beginRemoveRows(QModelIndex(), index, index);
    tables.removeAt(index);
    endRemoveRows();
}
void
resource_managers::Tables::add(const QUrl& url)
{
    int indexToRemove = -1;
    for (const auto& [index, table] : std::ranges::views::enumerate(tables)) {
        if (table.url == url) {
            indexToRemove = index;
        }
    }
    if (indexToRemove != -1) {
        removeAt(indexToRemove);
    }
    spdlog::info("Adding table: {}", url.toString().toStdString());
    const QNetworkRequest request(url);
    auto* reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, url] {
        handleInitialReply(reply, url);
    });
    // add to file
    fileOperationThreadPool.start([tableLocation = tableLocation, url] {
        save(tableLocation, url, QStringLiteral("url"), std::nullopt);
    });
    beginInsertRows(QModelIndex(), tables.size(), tables.size());
    tables.push_back(Table{ .url = url });
    endInsertRows();
}
void
resource_managers::Tables::reload(int index)
{
    if (index >= tables.size()) {
        return;
    }
    if (index < 0) {
        return;
    }
    auto& table = tables[index];
    if (table.status == Table::Loading) {
        return;
    }
    spdlog::info("Reloading table: {}", table.url.toString().toStdString());
    table.status = Table::Loading;
    const QNetworkRequest request(table.url);
    auto* reply = networkManager->get(request);
    connect(reply,
            &QNetworkReply::finished,
            this,
            [this, reply, url = table.url] { handleInitialReply(reply, url); });
    emit dataChanged(createIndex(index, 0), createIndex(index, 0));
}
void
resource_managers::Tables::reorder(int from, int to)
{
    if (from >= tables.size()) {
        return;
    }
    if (from < 0) {
        return;
    }
    if (to >= tables.size()) {
        return;
    }
    if (to < 0) {
        return;
    }
    spdlog::debug("Reordering table entries: {} and {}", from, to);
    beginMoveRows(QModelIndex(), from, from, QModelIndex(), to + (from < to));
    using std::swap;
    swap(tables[from], tables[to]);
    endMoveRows();
    fileOperationThreadPool.start([fromUrl = tables[from].url,
                                   toUrl = tables[to].url,
                                   tableLocation = tableLocation] {
        reorderInFile(tableLocation, fromUrl, toUrl);
    });
}
auto
resource_managers::Tables::getList() -> QVariantList
{
    auto ret = QVariantList{};
    for (const auto& table : tables) {
        ret.push_back(QVariant::fromValue(table));
    }
    return ret;
}