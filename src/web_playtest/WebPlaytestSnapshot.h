#pragma once

#include "InputEvent.h"
#include "gameplay_logic/SinglePlayerGameplayCore.h"

#include <QAbstractListModel>

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace web_playtest {

struct WorkerTelemetry
{
    std::uint64_t sessionGeneration = {};
    std::uint64_t lateInputClampNs = {};
    std::uint64_t lateByFrames = {};
    std::uint64_t droppedInputCommands = {};
    std::uint64_t firstNonZeroInputLatencyNs = {};
    std::uint32_t activeVoices = {};
    bool firstNonZeroInputLatencyAvailable = {};
};

struct WebPlaytestSnapshotPayload
{
    gameplay_logic::GameplaySnapshot gameplay;
    WorkerTelemetry telemetry;
    RuntimePhase phase = RuntimePhase::InstallingChart;
    double countdownSeconds = {};
    std::uint64_t publicationSequence = {};
};

enum class SnapshotSlotState : std::uint8_t
{
    Free,
    Writing,
    Published,
    Reading
};

class SnapshotMailbox final
{
  public:
    static constexpr auto slotCount = std::size_t{ 3 };
    static constexpr auto invalidSlot = slotCount;

    void reserveVisibleNotes(std::size_t capacity);
    [[nodiscard]] auto tryBeginWrite(std::size_t& slot) noexcept
      -> WebPlaytestSnapshotPayload*;
    void publishWrite(std::size_t slot,
                      std::uint64_t publicationSequence) noexcept;
    void cancelWrite(std::size_t slot) noexcept;
    [[nodiscard]] auto tryAcquireLatest() noexcept
      -> const WebPlaytestSnapshotPayload*;
    void releaseReading() noexcept;
    [[nodiscard]] auto droppedSnapshots() const noexcept -> std::uint64_t;
    [[nodiscard]] auto stateForTesting(std::size_t slot) const noexcept
      -> SnapshotSlotState;

  private:
    struct Slot
    {
        WebPlaytestSnapshotPayload payload;
        std::atomic<SnapshotSlotState> state{ SnapshotSlotState::Free };
    };

    std::array<Slot, slotCount> snapshotSlots;
    std::size_t readingSlot = invalidSlot;
    std::atomic<std::uint64_t> dropped = {};
};

class WebPlaytestNoteModel final : public QAbstractListModel
{
    Q_OBJECT

  public:
    enum Role
    {
        StableIdRole = Qt::UserRole + 1,
        DisplayColumnRole,
        NoteTypeRole,
        ChartTimeNsRole,
        ScrollPositionRole,
        PairedScrollPositionRole,
        HasPairedScrollPositionRole,
        HoldingRole
    };
    Q_ENUM(Role)

    explicit WebPlaytestNoteModel(QObject* parent = nullptr);
    [[nodiscard]] auto rowCount(const QModelIndex& parent = QModelIndex{}) const
      -> int override;
    [[nodiscard]] auto data(const QModelIndex& index,
                            int role = Qt::DisplayRole) const
      -> QVariant override;
    [[nodiscard]] auto roleNames() const -> QHash<int, QByteArray> override;

    void reserve(std::size_t capacity);
    void apply(const gameplay_logic::GameplaySnapshot& snapshot);

  private:
    struct Note
    {
        std::uint32_t stableId = {};
        std::uint8_t displayColumn = {};
        charts::BmsNotesData::NoteType type =
          charts::BmsNotesData::NoteType::Normal;
        std::int64_t chartTimeNs = {};
        double scrollPosition = {};
        double pairedScrollPosition = {};
        bool hasPairedScrollPosition = {};
        bool holding = {};

        bool operator==(const Note&) const = default;
    };

    [[nodiscard]] static auto makeNote(
      const gameplay_logic::GameplaySnapshot::VisibleNote& visible) -> Note;
    static auto shouldRender(
      const gameplay_logic::GameplaySnapshot::VisibleNote& visible) -> bool;

    std::vector<Note> notes;
    std::vector<Note> desired;
};

} // namespace web_playtest
