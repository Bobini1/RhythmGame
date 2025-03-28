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
    Q_PROPERTY(QVariantList trophies READ getTrophies CONSTANT)
    Q_PROPERTY(QStringList constraints MEMBER constraints)
  public:
    QString name;
    QString originalUrl;
    QStringList md5s;
    QList<Trophy> trophies;
    QStringList constraints;
    auto getTrophies() const -> QVariantList;
};

struct Table
{
    Q_GADGET
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(QString tag MEMBER tag)
    Q_PROPERTY(QVariant keymode MEMBER keymode)
    Q_PROPERTY(QVariantList levels READ getLevels CONSTANT)
    Q_PROPERTY(QVariantList courses READ getCourses CONSTANT)
  public:
    QString name;
    QString tag;
    QVariant keymode;
    QList<Level> levels;
    QList<QList<Course>> courses;
    auto getLevels() const -> QVariantList;
    auto getCourses() const -> QVariantList;
};

class Tables final : public QAbstractListModel
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
        Table table;
        Status status{ Loading };
    };
    QList<TableData> tables;
    QThreadPool fileOperationThreadPool;

    void handleInitialReply(QNetworkReply* reply, const QUrl& url);
    void setErrorFlag(const QUrl& url);
    void handleHeader(const QUrl& url, const QJsonObject& header);
    void handleData(const QUrl& url, const QJsonArray& data);
    void handleHeaderReply(const QUrl& url, const QByteArray& reply);

  public:
    enum Roles
    {
        UrlRole = Qt::UserRole + 1,
        TableRole,
        StatusRole
    };
    explicit Tables(QNetworkAccessManager* networkManager,
                    const QDir& tableLocation,
                    QObject* parent = nullptr);
    auto rowCount(const QModelIndex& parent) const -> int override;
    auto data(const QModelIndex& index, int role) const -> QVariant override;
    auto roleNames() const -> QHash<int, QByteArray> override;
    Q_INVOKABLE void removeAt(int index);
    Q_INVOKABLE void add(const QUrl& url);
    Q_INVOKABLE void reload(int index);
    Q_INVOKABLE void reorder(int from, int to);
};
} // namespace resource_managers

#endif // TABLEMANAGER_H
