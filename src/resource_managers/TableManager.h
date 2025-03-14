//
// Created by PC on 05/03/2025.
//

#ifndef TABLEMANAGER_H
#define TABLEMANAGER_H
#include "gameplay_logic/ChartData.h"

#include <QAbstractListModel>
#include <qnetworkreply.h>
#include <spdlog/spdlog.h>
#include <QDir>
#include <QFutureWatcher>

namespace resource_managers {

struct Level
{
    Q_GADGET
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(QList<QVariantMap> charts MEMBER charts)
public:
    QString name;
    QList<QVariantMap> charts;
};

struct Trophy
{
    Q_GADGET
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(double missRate MEMBER missRate)
    Q_PROPERTY(double scoreRate MEMBER scoreRate)
public:
    QString name;
    double missRate{};
    double scoreRate{};
};

struct Course
{
    Q_GADGET
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(QStringList md5s MEMBER md5s)
    Q_PROPERTY(QList<Trophy> trophies MEMBER trophies)
    Q_PROPERTY(QStringList constraints MEMBER constraints)
public:
    QString name;
    QString originalUrl;
    QStringList md5s;
    QList<Trophy> trophies;
    QStringList constraints;
};

class Table final : public QObject
{
    Q_GADGET
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(QString tag MEMBER tag)
    Q_PROPERTY(QVariant keymode MEMBER keymode)
    Q_PROPERTY(QList<Level> levels MEMBER levels)
    Q_PROPERTY(QList<QList<Course>> courses MEMBER courses)
public:
    QString name;
    QString tag;
    QVariant keymode;
    QList<Level> levels;
    QList<QList<Course>> courses;
    static auto fromJson(const QJsonDocument& header, const QJsonDocument& data) -> Table;
};

class TableManager final : public QAbstractListModel
{
    Q_OBJECT
    QNetworkAccessManager* networkManager;
    QDir tableLocation;
public:
    enum Status
    {
        Loading,
        Loaded,
        Error
    };
    Q_ENUM(Status)
private:
    struct TableData
    {
        QUrl url;
        std::optional<Table> table;
        Status status{Loading};
    };
    QList<TableData> tables;

    void handleInitialReply(QNetworkReply* reply);
    void handleHeaderReply(QNetworkReply* reply, QUrl url);
    void handleDataReply(QNetworkReply* reply, const QJsonDocument& header, QUrl url);

  public:
    enum Roles
    {
        UrlRole = Qt::UserRole + 1,
        TableRole,
        StatusRole
    };
    explicit TableManager(QNetworkAccessManager* networkManager,
                          const QDir& tableLocation,
                          QObject* parent = nullptr);
    auto rowCount(const QModelIndex& parent) const -> int override;
    auto data(const QModelIndex& index, int role) const -> QVariant override;
    auto roleNames() const -> QHash<int, QByteArray> override;
    void removeAt(int index);
    void addTable(const QUrl& url);

};
} // namespace resource_managers

#endif // TABLEMANAGER_H
