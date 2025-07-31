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
} // namespace db
namespace resource_managers {

struct Entry
{
    Q_GADGET
    Q_PROPERTY(QString title MEMBER title CONSTANT)
    Q_PROPERTY(QString artist MEMBER artist CONSTANT)
    Q_PROPERTY(QString subtitle MEMBER subtitle CONSTANT)
    Q_PROPERTY(QString subartist MEMBER subartist CONSTANT)
    Q_PROPERTY(QString md5 MEMBER md5 CONSTANT)
    Q_PROPERTY(QString sha256 MEMBER sha256 CONSTANT)
    Q_PROPERTY(QString url MEMBER url CONSTANT)
    Q_PROPERTY(QString urlDiff MEMBER urlDiff CONSTANT)
    Q_PROPERTY(QString level MEMBER level CONSTANT)
    Q_PROPERTY(QString comment MEMBER comment CONSTANT)
    public:
    QString title;
    QString artist;
    QString subtitle;
    QString subartist;
    QString md5;
    QString sha256;
    QString url;
    QString urlDiff;
    QString level;
    QString comment;
};

struct Level
{
    Q_GADGET
    Q_PROPERTY(QString name MEMBER name CONSTANT)
    Q_PROPERTY(QVariantList entries READ getEntries CONSTANT)
  public:
    db::SqliteCppDb* db;
    QString name;
    QList<Entry> entries;
    // for searching
    QHash<QString, Entry> md5s;
    auto getEntries() const -> QVariantList;
    Q_INVOKABLE QVariantList loadCharts() const;
};

struct Trophy
{
    Q_GADGET
    Q_PROPERTY(QString name MEMBER name CONSTANT)
    Q_PROPERTY(double missRate MEMBER missRate CONSTANT)
    Q_PROPERTY(double scoreRate MEMBER scoreRate CONSTANT)
  public:
    QString name;
    double missRate{};
    double scoreRate{};

    static auto fromJson(const QJsonObject& json) -> Trophy;
    auto toJson() const -> QJsonObject;
};

struct Course
{
    Q_GADGET
    Q_PROPERTY(QString name MEMBER name CONSTANT)
    Q_PROPERTY(QStringList md5s MEMBER md5s CONSTANT)
    Q_PROPERTY(QVariantList trophies READ getTrophies CONSTANT)
    Q_PROPERTY(QStringList constraints MEMBER constraints CONSTANT)
    Q_PROPERTY(QString identifier READ getIdentifier STORED false CONSTANT)
  public:
    db::SqliteCppDb* db;
    QString name;
    QString originalUrl;
    QStringList md5s;
    QList<Trophy> trophies;
    QStringList constraints;
    auto getTrophies() const -> QVariantList;
    auto getIdentifier() const -> QString;
    Q_INVOKABLE QVariantList loadCharts() const;
};

struct TableInfo
{
    Q_GADGET
    Q_PROPERTY(QString tableName MEMBER tableName CONSTANT)
    Q_PROPERTY(QString levelName MEMBER levelName CONSTANT)
    Q_PROPERTY(QString symbol MEMBER symbol CONSTANT)
    Q_PROPERTY(QUrl tableUrl MEMBER tableUrl CONSTANT)
    Q_PROPERTY(Entry entry MEMBER entry CONSTANT)
  public:
    QString tableName;
    QString levelName;
    QString symbol;
    QUrl tableUrl;
    Entry entry;
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
    Q_PROPERTY(QString symbol MEMBER symbol)

  public:
    QString name;
    QString tag;
    QVariant keymode;
    QString symbol;
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
    Q_INVOKABLE QVariantList getList();
    Q_INVOKABLE QList<TableInfo> search(const QString& md5);
};
} // namespace resource_managers

#endif // TABLEMANAGER_H
