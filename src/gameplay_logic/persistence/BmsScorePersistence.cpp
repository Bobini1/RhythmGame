#include "gameplay_logic/BmsScore.h"

#include "db/SqliteCppDb.h"

void
gameplay_logic::BmsScore::save(db::SqliteCppDb& db) const
{
    result->save(db);
    replayData->save(db);
    gaugeHistory->save(db);
}
