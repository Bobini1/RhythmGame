//
// Created by bobini on 30.09.23.
//

#ifndef RHYTHMGAME_BMSREPLAYDATA_H
#define RHYTHMGAME_BMSREPLAYDATA_H

#include "HitEvent.h"
#include "db/SqliteCppDb.h"

#include <QList>
#include <QObject>
#include <QString>
#include <QJsonArray>
namespace gameplay_logic {

class BmsReplayData final : public QObject
{
    Q_OBJECT

    /**
     * @brief The list of hit events that occurred during the score.
     */
    Q_PROPERTY(QList<HitEvent> hitEvents READ getHitEvents CONSTANT)
    /**
     * @brief The identifier of the score this replay data belongs to.
     */
    Q_PROPERTY(QString guid READ getGuid CONSTANT)

    QList<HitEvent> hitEvents;
    QString guid;

  public:
    explicit BmsReplayData(QList<HitEvent> hitEvents,
                           QString guid,
                           QObject* parent = nullptr);
    auto getHitEvents() const -> const QList<HitEvent>&;
    auto getGuid() const -> QString;

    struct DTO
    {
        int64_t id;
        std::string guid;
        std::string hitEvents;
    };

    void save(db::SqliteCppDb& db) const;
    static auto load(const DTO& dto) -> std::unique_ptr<BmsReplayData>;
    static void migrateStoredReplayData(db::SqliteCppDb& db);
    auto toJsonArray() const -> QJsonArray;
    static auto fromJsonArray(const QJsonArray& array) -> QList<HitEvent>;
};

} // namespace gameplay_logic

#endif // RHYTHMGAME_BMSREPLAYDATA_H
