#include "gameplay_logic/BmsReplayData.h"

#include "db/SqliteCppDb.h"

#include <spdlog/spdlog.h>
#include <tuple>

void
gameplay_logic::BmsReplayData::save(db::SqliteCppDb& db) const
{
    if (guid.isEmpty()) {
        return;
    }
    auto statement =
      db.createStatement("INSERT OR REPLACE INTO replay_data (score_guid, "
                         "replay_data) VALUES (?, ?)");
    const auto serialized = serializeReplayData(hitEvents);
    statement.bind(1, guid.toStdString());
    statement.bind(2, serialized.data(), serialized.size());
    statement.execute();
}

void
gameplay_logic::BmsReplayData::migrateStoredReplayData(db::SqliteCppDb& db)
{
    auto statement = db.createStatement(
      "SELECT id, score_guid, replay_data FROM replay_data;");
    const auto rows =
      statement
        .executeAndGetAll<std::tuple<int64_t, std::string, std::string>>();
    auto update = db.createStatement(
      "UPDATE replay_data SET replay_data = ? WHERE id = ?;");
    for (const auto& [id, guid, replayData] : rows) {
        try {
            const auto hitEvents =
              deserializeReplayData(QByteArray::fromStdString(replayData));
            const auto serialized = serializeReplayData(hitEvents);
            update.reset();
            update.bind(1, serialized.data(), serialized.size());
            update.bind(2, id);
            update.execute();
        } catch (const std::exception& e) {
            spdlog::warn(
              "Failed to migrate replay data {}: {}", guid, e.what());
        }
    }
}
