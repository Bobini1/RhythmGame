//
// Created by PC on 05/03/2025.
//

#ifndef TABLEMANAGER_H
#define TABLEMANAGER_H

#include <QAbstractListModel>
#include <QDir>
#include <QNetworkReply>
#include <qthreadpool.h>
namespace db {
class SqliteCppDb;
}
namespace resource_managers {

struct Level
{
    Q_GADGET
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(QList<QVariantMap> charts MEMBER charts)
  public:
    db::SqliteCppDb* db;
    QString name;
    QList<QVariantMap> charts;
    Q_INVOKABLE QVariantList loadCharts() const;
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

  public:
    enum Status
    {
        Loading,
        Loaded,
        Error
    };
    Q_ENUM(Status)
  private:
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(QString tag MEMBER tag)
    Q_PROPERTY(QVariant keymode MEMBER keymode)
    Q_PROPERTY(QVariantList levels READ getLevels CONSTANT)
    Q_PROPERTY(QVariantList courses READ getCourses CONSTANT)
    Q_PROPERTY(QUrl url MEMBER url)
    Q_PROPERTY(Status status MEMBER status)

  public:
    QString name;
    QString tag;
    QVariant keymode;
    QList<Level> levels;
    QList<QList<Course>> courses;
    QUrl url;
    Status status{ Loading };
    auto getLevels() const -> QVariantList;
    auto getCourses() const -> QVariantList;
};

class Tables final : public QAbstractListModel
{
    Q_OBJECT
    QNetworkAccessManager* networkManager;
    QDir tableLocation;
    db::SqliteCppDb* db;
    QList<Table> tables;
    QThreadPool fileOperationThreadPool;

    void handleInitialReply(QNetworkReply* reply, const QUrl& url);
    void setErrorFlag(const QUrl& url);
    void handleHeader(const QUrl& url, const QJsonObject& header);
    void handleData(const QUrl& url, const QJsonArray& data);
    void handleHeaderReply(const QUrl& url, const QByteArray& reply);

  public:
    explicit Tables(QNetworkAccessManager* networkManager,
                    const QDir& tableLocation,
                    db::SqliteCppDb* db,
                    QObject* parent = nullptr);
    auto rowCount(const QModelIndex& parent) const -> int override;
    auto data(const QModelIndex& index, int role) const -> QVariant override;
    Q_INVOKABLE void removeAt(int index);
    Q_INVOKABLE void add(const QUrl& url);
    Q_INVOKABLE void reload(int index);
    Q_INVOKABLE void reorder(int from, int to);
    Q_INVOKABLE QList<QVariant> getList();
};
} // namespace resource_managers

#endif // TABLEMANAGER_H
