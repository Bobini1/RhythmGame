#include "WebPlaytestSnapshot.h"

#include <QByteArray>
#include <QVariant>

#include <algorithm>
#include <limits>

namespace web_playtest {

void
SnapshotMailbox::reserveVisibleNotes(std::size_t capacity)
{
    for (auto& slot : snapshotSlots) {
        slot.payload.gameplay.visibleNotes.reserve(capacity);
    }
}

auto
SnapshotMailbox::tryBeginWrite(std::size_t& slot) noexcept
  -> WebPlaytestSnapshotPayload*
{
    for (auto index = std::size_t{}; index < snapshotSlots.size(); ++index) {
        auto expected = SnapshotSlotState::Free;
        if (snapshotSlots[index].state.compare_exchange_strong(
              expected,
              SnapshotSlotState::Writing,
              std::memory_order_acquire,
              std::memory_order_relaxed)) {
            slot = index;
            return &snapshotSlots[index].payload;
        }
    }
    dropped.fetch_add(1, std::memory_order_relaxed);
    slot = invalidSlot;
    return nullptr;
}

void
SnapshotMailbox::publishWrite(std::size_t slot,
                              std::uint64_t publicationSequence) noexcept
{
    if (slot >= snapshotSlots.size()) {
        return;
    }
    snapshotSlots[slot].payload.publicationSequence = publicationSequence;
    auto expected = SnapshotSlotState::Writing;
    (void)snapshotSlots[slot].state.compare_exchange_strong(
      expected,
      SnapshotSlotState::Published,
      std::memory_order_release,
      std::memory_order_relaxed);
}

void
SnapshotMailbox::cancelWrite(std::size_t slot) noexcept
{
    if (slot >= snapshotSlots.size()) {
        return;
    }
    auto expected = SnapshotSlotState::Writing;
    (void)snapshotSlots[slot].state.compare_exchange_strong(
      expected,
      SnapshotSlotState::Free,
      std::memory_order_release,
      std::memory_order_relaxed);
}

auto
SnapshotMailbox::tryAcquireLatest() noexcept
  -> const WebPlaytestSnapshotPayload*
{
    auto newestSlot = invalidSlot;
    auto newestGeneration = std::uint64_t{};
    for (auto index = std::size_t{}; index < snapshotSlots.size(); ++index) {
        if (snapshotSlots[index].state.load(std::memory_order_acquire) !=
            SnapshotSlotState::Published) {
            continue;
        }
        const auto publicationSequence =
          snapshotSlots[index].payload.publicationSequence;
        if (newestSlot == invalidSlot ||
            publicationSequence > newestGeneration) {
            newestSlot = index;
            newestGeneration = publicationSequence;
        }
    }
    if (newestSlot == invalidSlot) {
        return nullptr;
    }

    auto expected = SnapshotSlotState::Published;
    if (!snapshotSlots[newestSlot].state.compare_exchange_strong(
          expected,
          SnapshotSlotState::Reading,
          std::memory_order_acquire,
          std::memory_order_relaxed)) {
        return nullptr;
    }
    releaseReading();
    readingSlot = newestSlot;

    for (auto index = std::size_t{}; index < snapshotSlots.size(); ++index) {
        if (index == newestSlot) {
            continue;
        }
        if (snapshotSlots[index].state.load(std::memory_order_acquire) !=
            SnapshotSlotState::Published) {
            continue;
        }
        expected = SnapshotSlotState::Published;
        if (snapshotSlots[index].payload.publicationSequence <=
            newestGeneration) {
            (void)snapshotSlots[index].state.compare_exchange_strong(
              expected,
              SnapshotSlotState::Free,
              std::memory_order_release,
              std::memory_order_relaxed);
        }
    }
    return &snapshotSlots[newestSlot].payload;
}

void
SnapshotMailbox::releaseReading() noexcept
{
    if (readingSlot == invalidSlot) {
        return;
    }
    auto expected = SnapshotSlotState::Reading;
    (void)snapshotSlots[readingSlot].state.compare_exchange_strong(
      expected,
      SnapshotSlotState::Free,
      std::memory_order_release,
      std::memory_order_relaxed);
    readingSlot = invalidSlot;
}

auto
SnapshotMailbox::droppedSnapshots() const noexcept -> std::uint64_t
{
    return dropped.load(std::memory_order_acquire);
}

auto
SnapshotMailbox::stateForTesting(std::size_t slot) const noexcept
  -> SnapshotSlotState
{
    return slot < snapshotSlots.size()
             ? snapshotSlots[slot].state.load(std::memory_order_acquire)
             : SnapshotSlotState::Free;
}

WebPlaytestNoteModel::WebPlaytestNoteModel(QObject* parent)
  : QAbstractListModel(parent)
{
}

auto
WebPlaytestNoteModel::rowCount(const QModelIndex& parent) const -> int
{
    if (parent.isValid() ||
        notes.size() >
          static_cast<std::size_t>((std::numeric_limits<int>::max)())) {
        return 0;
    }
    return static_cast<int>(notes.size());
}

auto
WebPlaytestNoteModel::data(const QModelIndex& index, int role) const -> QVariant
{
    if (!index.isValid() || index.row() < 0 ||
        static_cast<std::size_t>(index.row()) >= notes.size()) {
        return {};
    }
    const auto& note = notes[static_cast<std::size_t>(index.row())];
    switch (role) {
        case StableIdRole:
            return note.stableId;
        case DisplayColumnRole:
            return note.displayColumn;
        case NoteTypeRole:
            return static_cast<int>(note.type);
        case ChartTimeNsRole:
            return note.chartTimeNs;
        case ScrollPositionRole:
            return note.scrollPosition;
        case PairedScrollPositionRole:
            return note.pairedScrollPosition;
        case HasPairedScrollPositionRole:
            return note.hasPairedScrollPosition;
        case HoldingRole:
            return note.holding;
        default:
            return {};
    }
}

auto
WebPlaytestNoteModel::roleNames() const -> QHash<int, QByteArray>
{
    return {
        { StableIdRole, "stableId" },
        { DisplayColumnRole, "displayColumn" },
        { NoteTypeRole, "noteType" },
        { ChartTimeNsRole, "chartTimeNs" },
        { ScrollPositionRole, "scrollPosition" },
        { PairedScrollPositionRole, "pairedScrollPosition" },
        { HasPairedScrollPositionRole, "hasPairedScrollPosition" },
        { HoldingRole, "holding" },
    };
}

void
WebPlaytestNoteModel::reserve(std::size_t capacity)
{
    notes.reserve(capacity);
    desired.reserve(capacity);
}

void
WebPlaytestNoteModel::apply(const gameplay_logic::GameplaySnapshot& snapshot)
{
    desired.clear();
    for (const auto& visible : snapshot.visibleNotes) {
        if (shouldRender(visible)) {
            desired.push_back(makeNote(visible));
        }
    }
    std::ranges::sort(desired, {}, &Note::stableId);

    auto row = std::size_t{};
    while (row < notes.size() || row < desired.size()) {
        if (row == notes.size()) {
            beginInsertRows(
              {}, static_cast<int>(row), static_cast<int>(desired.size() - 1));
            notes.insert(notes.end(),
                         desired.begin() + static_cast<std::ptrdiff_t>(row),
                         desired.end());
            endInsertRows();
            break;
        }
        if (row == desired.size()) {
            beginRemoveRows(
              {}, static_cast<int>(row), static_cast<int>(notes.size() - 1));
            notes.erase(notes.begin() + static_cast<std::ptrdiff_t>(row),
                        notes.end());
            endRemoveRows();
            break;
        }
        if (notes[row].stableId < desired[row].stableId) {
            beginRemoveRows({}, static_cast<int>(row), static_cast<int>(row));
            notes.erase(notes.begin() + static_cast<std::ptrdiff_t>(row));
            endRemoveRows();
            continue;
        }
        if (desired[row].stableId < notes[row].stableId) {
            beginInsertRows({}, static_cast<int>(row), static_cast<int>(row));
            notes.insert(notes.begin() + static_cast<std::ptrdiff_t>(row),
                         desired[row]);
            endInsertRows();
            ++row;
            continue;
        }
        if (notes[row] != desired[row]) {
            notes[row] = desired[row];
            const auto modelIndex = index(static_cast<int>(row));
            emit dataChanged(modelIndex, modelIndex);
        }
        ++row;
    }
}

auto
WebPlaytestNoteModel::makeNote(
  const gameplay_logic::GameplaySnapshot::VisibleNote& visible) -> Note
{
    return {
        .stableId = visible.stableId,
        .displayColumn = static_cast<std::uint8_t>(
          visible.column == 7 ? 0 : visible.column + 1),
        .type = visible.type,
        .chartTimeNs = visible.chartTimeNs,
        .scrollPosition = visible.scrollPosition,
        .pairedScrollPosition =
          visible.pairedScrollPosition.value_or(visible.scrollPosition),
        .hasPairedScrollPosition = visible.pairedScrollPosition.has_value(),
        .holding = visible.holding,
    };
}

auto
WebPlaytestNoteModel::shouldRender(
  const gameplay_logic::GameplaySnapshot::VisibleNote& visible) -> bool
{
    return visible.column <= 7 && (!visible.removed || visible.holding) &&
           visible.type != charts::BmsNotesData::NoteType::Invisible &&
           visible.type != charts::BmsNotesData::NoteType::LongNoteEnd;
}

} // namespace web_playtest
