//
// Created by PC on 05/03/2025.
//

#include "TableManager.h"

#include <QJsonDocument>
#include <QJsonValue>
#include <libxml2/libxml/xmlreader.h>
#include <QUrl>
auto
getBmstableLink(const QString& html) -> QString
{
    auto htmlStd = html.toStdString();
    xmlTextReaderPtr reader =
      xmlReaderForMemory(htmlStd.c_str(), html.length(), NULL, NULL, 0);

    if (reader != nullptr) {
        int ret = xmlTextReaderRead(reader);
        while (ret == 1) {
            if (const xmlChar* name = xmlTextReaderConstName(reader);
                name != nullptr &&
                xmlStrcmp(name, reinterpret_cast<const xmlChar*>("meta")) ==
                  0) {
                if (const xmlChar* property = xmlTextReaderGetAttribute(
                      reader, reinterpret_cast<const xmlChar*>("property"));
                    property && xmlStrcmp(property,
                                          reinterpret_cast<const xmlChar*>(
                                            "bmstable")) == 0) {
                    const xmlChar* content = xmlTextReaderGetAttribute(
                      reader, reinterpret_cast<const xmlChar*>("content"));
                    return QString::fromUtf8(content);
                }
            }
            ret = xmlTextReaderRead(reader);
        }
        xmlFreeTextReader(reader);
    }
    return {};
}
auto
resource_managers::Table::fromJson(const QJsonDocument& header, const QJsonDocument& data) -> Table
{
    auto table = Table{};
    table.name = header["name"].toString();
    table.tag = header["tag"].toString();
    table.keymode = header["keymode"];
    for (const auto& level : header["levels"].toArray()) {
        auto levelObj = Level{};
        levelObj.name = level["name"].toString();
        for (const auto& chart : level["charts"].toArray()) {
            levelObj.charts.push_back(chart.toObject().toVariantMap());
        }
        table.levels.push_back(levelObj);
    }
    for (const auto& courseList : data["courses"].toArray()) {
        auto courseListObj = QList<Course>{};
        for (const auto& course : courseList.toArray()) {
            auto courseObj = Course{};
            courseObj.name = course["name"].toString();
            for (const auto& md5 : course["md5"].toArray()) {
                courseObj.md5s.push_back(md5.toString());
            }
            for (const auto& trophy : course["trophy"].toArray()) {
                auto trophyObj = Trophy{};
                trophyObj.name = trophy["name"].toString();
                trophyObj.missRate = trophy["missRate"].toDouble();
                trophyObj.scoreRate = trophy["scoreRate"].toDouble();
                courseObj.trophies.push_back(trophyObj);
            }
            for (const auto& constraint : course["constraints"].toArray()) {
                courseObj.constraints.push_back(constraint.toString());
            }
            courseListObj.push_back(courseObj);
        }
        table.courses.push_back(courseListObj);
    }
    return table;
}
void
resource_managers::TableManager::handleInitialReply(QNetworkReply* reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        if (const auto type = reply->header(QNetworkRequest::ContentTypeHeader);
            reply->url.toString().endsWith(".json") ||
            type.toString().startsWith("application/json")) {
            handleHeaderReply(reply);
            return;
        }
        const QString html = reply->readAll();
        auto bmstableLink = getBmstableLink(html);
        if (!bmstableLink.isEmpty()) {
            auto url = reply->url.resolved(QUrl(bmstableLink));
            const auto request = QNetworkRequest(url);
            const auto* newReply = networkManager->get(request);
            connect(newReply,
                    &QNetworkReply::finished,
                    this,
                    &TableManager::handleHeaderReply);
        } else {
            spdlog::error("No bmstable link found");
        }

    } else {
        spdlog::error("Network error: {}", reply->errorString().toStdString());
    }

    reply->deleteLater();
}
void
resource_managers::TableManager::handleHeaderReply(QNetworkReply* reply, QUrl url)
{
    reply->deleteLater();
    const auto json = QJsonDocument::fromJson(reply->readAll());
    if (json.isNull()) {
        spdlog::error("Failed to parse json");
        return;
    }
    auto dataUrl = json["data_url"].toString();
    if (dataUrl.isEmpty()) {
        spdlog::error("No data url found");
        for (auto& table : tables) {
            if (table.url == url) {
                table.status = TableManager::Error;
            }
        }
    }
}

void
resource_managers::TableManager::handleDataReply(QNetworkReply* reply, const QJsonDocument& header, QUrl url){

}
resource_managers::TableManager::TableManager(
  QNetworkAccessManager* networkManager,
  const QDir& tableLocation,
  QObject* parent)
  : QAbstractListModel(parent)
  , networkManager(networkManager)
  , tableLocation(tableLocation)
{
}
int
resource_managers::TableManager::rowCount(const QModelIndex& parent) const
{
    return tables.size();
}
QVariant
resource_managers::TableManager::data(const QModelIndex& index,
                                      const int role) const
{
    if (!index.isValid()) {
        return {};
    }
    if (index.row() >= tables.size()) {
        return {};
    }
    const auto& table = tables[index.row()];
    switch (role) {
        case UrlRole:
            return table.url;
        case TableRole:
            return QVariant::fromValue(table.table);

    }
    return {};
}
QHash<int, QByteArray>
resource_managers::TableManager::roleNames() const
{
    return {
        { UrlRole, "url" },
        { TableRole, "table" },
    };
}
void
resource_managers::TableManager::removeAt(int index)
{
    if (index >= tables.size()) {
        return;
    }
    if (index < 0) {
        return;
    }
    beginRemoveRows(QModelIndex(), index, index);
    tables.removeAt(index);
    endRemoveRows();
}
void
resource_managers::TableManager::addTable(const QUrl& url)
{
    // check if the table is already in the list
    for (const auto& table : tables) {
        if (table.url == url) {
            return;
        }
    }
    const QNetworkRequest request(url);
    const auto* reply = networkManager->get(request);
    connect(reply,
            &QNetworkReply::finished,
            this,
            &TableManager::handleInitialReply);
    beginInsertRows(QModelIndex(), tables.size(), tables.size());
    tables.push_back({ url });
    endInsertRows();
}