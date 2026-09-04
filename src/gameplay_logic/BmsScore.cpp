//
// Created by bobini on 30.09.23.
//

#include "BmsScore.h"
gameplay_logic::BmsScore::BmsScore(
  std::unique_ptr<BmsResult> result,
  std::unique_ptr<BmsReplayData> replayData,
  std::unique_ptr<BmsGaugeHistory> gaugeHistory,
  QObject* parent)
  : BmsScore(std::move(result),
             std::move(replayData),
             std::move(gaugeHistory),
             parent,
             Source::RhythmGame,
             LongNoteMode::Ln)
{
}
gameplay_logic::BmsScore::BmsScore(
  std::unique_ptr<BmsResult> result,
  std::unique_ptr<BmsReplayData> replayData,
  std::unique_ptr<BmsGaugeHistory> gaugeHistory,
  QObject* parent,
  Source source,
  LongNoteMode longNoteMode)
  : QObject(parent)
  , result(result.release())
  , replayData(replayData.release())
  , gaugeHistory(gaugeHistory.release())
  , source(source)
  , longNoteMode(longNoteMode)
{
    this->result->setParent(this);
    if (this->replayData != nullptr) {
        this->replayData->setParent(this);
    }
    if (this->gaugeHistory != nullptr) {
        this->gaugeHistory->setParent(this);
    }
}
auto
gameplay_logic::BmsScore::fromImportedResult(std::unique_ptr<BmsResult> result,
                                             Source source,
                                             LongNoteMode longNoteMode,
                                             QObject* parent)
  -> std::unique_ptr<BmsScore>
{
    Q_ASSERT(source != Source::RhythmGame);
    return std::make_unique<BmsScore>(
      std::move(result), nullptr, nullptr, parent, source, longNoteMode);
}
auto
gameplay_logic::BmsScore::getResult() const -> BmsResult*
{
    return result;
}
auto
gameplay_logic::BmsScore::getReplayData() const -> BmsReplayData*
{
    return replayData;
}
auto
gameplay_logic::BmsScore::getGaugeHistory() const -> BmsGaugeHistory*
{
    return gaugeHistory;
}
auto
gameplay_logic::BmsScore::getSource() const -> Source
{
    return source;
}
auto
gameplay_logic::BmsScore::getLongNoteMode() const -> LongNoteMode
{
    return longNoteMode;
}
auto
gameplay_logic::BmsScore::getSourceName() const -> QString
{
    switch (source) {
        case Source::Lr2:
            return QStringLiteral("LR2");
        case Source::Beatoraja:
            return QStringLiteral("beatoraja");
        case Source::Bokutachi:
            return QStringLiteral("Bokutachi");
        case Source::RhythmGame:
            return QStringLiteral("RhythmGame");
    }
    Q_UNREACHABLE_RETURN(QString{});
}
auto
gameplay_logic::BmsScore::isImported() const -> bool
{
    return source != Source::RhythmGame;
}
void
gameplay_logic::BmsScore::setSubmissionState(SubmissionState newState)
{
    if (submissionState != newState) {
        submissionState = newState;
        emit submissionStateChanged();
    }
}
auto
gameplay_logic::BmsScore::getSubmissionState() const -> SubmissionState
{
    return submissionState;
}
void
gameplay_logic::BmsScore::save(db::SqliteCppDb& db) const
{
    result->save(db, static_cast<int>(source), static_cast<int>(longNoteMode));
    if (replayData != nullptr) {
        replayData->save(db);
    }
    if (gaugeHistory != nullptr) {
        gaugeHistory->save(db);
    }
}
