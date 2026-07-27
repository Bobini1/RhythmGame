#include "gameplay_logic/BmsGaugeHistory.h"

#include "db/SqliteCppDb.h"
#include "support/Compress.h"

void
gameplay_logic::BmsGaugeHistory::save(db::SqliteCppDb& db) const
{
    if (guid.isEmpty()) {
        return;
    }
    auto statement = db.createStatement(
      "INSERT OR IGNORE INTO gauge_history (score_guid, gauge_info) "
      "VALUES (?, ?)");
    auto compressedInfo = support::compress(gaugeInfo);
    statement.bind(1, guid.toStdString());
    statement.bind(2, compressedInfo.data(), compressedInfo.size());
    statement.execute();
}
