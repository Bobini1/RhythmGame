#ifndef RHYTHMGAME_SINGLEPLAYERGAMEPLAYCORE_H
#define RHYTHMGAME_SINGLEPLAYERGAMEPLAYCORE_H

#include "GameplayTrace.h"
#include "Judgement.h"
#include "charts/BmsNotesData.h"
#include "input/BmsKeys.h"
#include "resource_managers/ChartPlayConfig.h"
#include "sounds/Sound.h"

#include <QString>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace gameplay_logic {

enum class GameplayKeyAction
{
    Press,
    Release
};

struct GameplayCoreConfig
{
    resource_managers::ChartPlayConfig play;
    std::int64_t savedTimestampSeconds;
    QString scoreGuid;
    double maxHitValue;
};

struct GameplaySnapshot
{
    struct VisibleNote
    {
        std::uint32_t stableId;
        std::uint8_t column;
        charts::BmsNotesData::NoteType type;
        std::int64_t chartTimeNs;
        double beatPosition;
        double scrollPosition;
        // Present only for a safely matched long-note endpoint.
        std::optional<double> pairedScrollPosition;
        bool removed;
        bool holding;

        bool operator==(const VisibleNote&) const = default;
    };

    std::int64_t chartTimeNs;
    double beatPosition;
    double scrollPosition;
    double points;
    double maxPointsNow;
    double gauge;
    int combo;
    int maxCombo;
    int mineHits;
    std::optional<Judgement> latestJudgement;
    std::optional<std::int64_t> latestDeviationNs;
    std::array<bool, charts::BmsNotesData::columnNumber> pressedColumns;
    std::vector<VisibleNote> visibleNotes;
    bool finished;
};

class SinglePlayerGameplayCore
{
    class Impl;
    std::unique_ptr<Impl> impl;

    explicit SinglePlayerGameplayCore(std::unique_ptr<Impl> impl);

  public:
    ~SinglePlayerGameplayCore();
    SinglePlayerGameplayCore(const SinglePlayerGameplayCore&) = delete;
    auto operator=(const SinglePlayerGameplayCore&)
      -> SinglePlayerGameplayCore& = delete;
    SinglePlayerGameplayCore(SinglePlayerGameplayCore&&) noexcept;
    auto operator=(SinglePlayerGameplayCore&&) noexcept
      -> SinglePlayerGameplayCore&;

    static auto create(
      std::string_view chartBytes,
      const std::filesystem::path& logicalChartPath,
      GameplayCoreConfig config,
      std::unordered_map<std::uint64_t, std::shared_ptr<sounds::Sound>> sounds)
      -> std::unique_ptr<SinglePlayerGameplayCore>;

    void advanceTo(std::chrono::nanoseconds chartTime);
    void passKey(input::BmsKey key,
                 GameplayKeyAction action,
                 std::chrono::nanoseconds chartTime);
    void preScheduleBgm();
    /**
     * Upper bound for visible-note storage required by fillSnapshot().
     */
    [[nodiscard]] auto snapshotVisibleNoteCapacity() const noexcept
      -> std::size_t;
    /**
     * Reserves enough visible-note storage for allocation-free fillSnapshot()
     * calls. Intended for long-lived snapshot buffers.
     */
    void reserveSnapshot(GameplaySnapshot& snapshot) const;
    /**
     * Replaces snapshot contents, reusing its visible-note storage. This call
     * does not reallocate after reserveSnapshot() has been called.
     */
    void fillSnapshot(GameplaySnapshot& snapshot) const;
    [[nodiscard]] auto snapshot() const -> GameplaySnapshot;
    [[nodiscard]] auto finishTrace() const -> QByteArray;
};

} // namespace gameplay_logic

#endif // RHYTHMGAME_SINGLEPLAYERGAMEPLAYCORE_H
