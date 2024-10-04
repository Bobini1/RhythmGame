//
// Created by bobini on 30.09.23.
//

#ifndef RHYTHMGAME_BMSSCOREAFTERMATH_H
#define RHYTHMGAME_BMSSCOREAFTERMATH_H

#include <QObject>
#include "BmsGaugeHistory.h"
#include "BmsResult.h"
#include "BmsReplayData.h"
#include "resource_managers/Profile.h"

#include <qpointer.h>
namespace gameplay_logic {
class BmsScoreAftermath : public QObject
{
    Q_OBJECT

    Q_PROPERTY(Profile* profile READ getProfile CONSTANT)
    Q_PROPERTY(BmsResult* result READ getResult CONSTANT)
    Q_PROPERTY(BmsReplayData* replayData READ getReplayData CONSTANT)
    Q_PROPERTY(BmsGaugeHistory* gaugeHistory READ getGaugeHistory CONSTANT)

    QPointer<resource_managers::Profile> profile;
    BmsResult* result;
    BmsReplayData* replayData;
    BmsGaugeHistory* gaugeHistory;

  public:
    explicit BmsScoreAftermath(resource_managers::Profile* profile,
                               std::unique_ptr<BmsResult> result,
                               std::unique_ptr<BmsReplayData> replayData,
                               std::unique_ptr<BmsGaugeHistory> gaugeHistory,
                               QObject* parent = nullptr);

    auto getProfile() const -> resource_managers::Profile*;
    auto getResult() const -> BmsResult*;
    auto getReplayData() const -> BmsReplayData*;
    auto getGaugeHistory() const -> BmsGaugeHistory*;
};
} // namespace gameplay_logic

#endif // RHYTHMGAME_BMSSCOREAFTERMATH_H
