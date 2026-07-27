#ifndef RHYTHMGAME_GAMEPLAYTRACE_H
#define RHYTHMGAME_GAMEPLAYTRACE_H

#include "HitEvent.h"
#include "resource_managers/ChartPlayOptions.h"

#include <QByteArray>
#include <QList>
#include <QString>

#include <cstdint>
#include <optional>
#include <vector>

namespace gameplay_logic {

struct GameplayTraceInput
{
    std::int64_t chartTimeNs;
    int key;
    HitEvent::Action action;
};

struct GameplayTraceJudgement
{
    std::int64_t chartTimeNs;
    std::int64_t hitOffsetNs;
    int column;
    int key;
    int noteIndex;
    HitEvent::Action action;
    bool noteRemoved;
    std::optional<Judgement> judgement;
    std::optional<std::int64_t> deviationNs;
    std::optional<double> value;
};

struct GameplayTraceGaugeSample
{
    std::int64_t chartTimeNs;
    double value;
};

struct GameplayTraceResult
{
    double points{};
    double maxPoints{};
    double maxPointsNow{};
    double gauge{};
    int combo{};
    int maxCombo{};
    int mineHits{};
    QString clearType;
    QList<int> judgementCounts;
    std::int64_t savedTimestampSeconds{};
    QString scoreGuid;
    std::int64_t chartLengthNs{};
    int keymode{};
    int dpMode{};
};

struct GameplayTrace
{
    QString sha256;
    QString md5;
    QList<qint64> randomSequence;
    QList<int> permutation;
    std::uint64_t laneSeed{};
    resource_managers::NoteOrderAlgorithm noteOrderP1{
        resource_managers::NoteOrderAlgorithm::Normal
    };
    resource_managers::NoteOrderAlgorithm noteOrderP2{
        resource_managers::NoteOrderAlgorithm::Normal
    };
    resource_managers::DpOptions dpMode{ resource_managers::DpOptions::Off };
    std::vector<GameplayTraceInput> inputs;
    std::vector<GameplayTraceJudgement> judgements;
    std::vector<GameplayTraceGaugeSample> gaugeSamples;
    GameplayTraceResult result;

    [[nodiscard]] auto toCanonicalJson() const -> QByteArray;
};

} // namespace gameplay_logic

#endif // RHYTHMGAME_GAMEPLAYTRACE_H
