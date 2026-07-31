#include "ArenaAvailabilityIndex.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <optional>
#include <utility>

namespace arena {
namespace {

auto
recordCount(const QByteArray& packed) -> qsizetype
{
    return packed.size() / ArenaSha256Bytes;
}

auto
recordAt(const QByteArray& packed, qsizetype index) -> const char*
{
    return packed.constData() + index * ArenaSha256Bytes;
}

auto
compareRecords(const char* lhs, const char* rhs) -> int
{
    return std::memcmp(lhs, rhs, ArenaSha256Bytes);
}

auto
validPackedVector(const QByteArray& packed) -> bool
{
    if (packed.size() % ArenaSha256Bytes != 0 ||
        packed.size() > ArenaMaxInventoryBytes) {
        return false;
    }
    const auto count = recordCount(packed);
    for (qsizetype i = 1; i < count; ++i) {
        if (compareRecords(recordAt(packed, i - 1), recordAt(packed, i)) >= 0) {
            return false;
        }
    }
    return true;
}

auto
hexNibble(QChar value) -> int
{
    const auto code = value.unicode();
    if (code >= '0' && code <= '9') {
        return code - '0';
    }
    if (code >= 'a' && code <= 'f') {
        return code - 'a' + 10;
    }
    if (code >= 'A' && code <= 'F') {
        return code - 'A' + 10;
    }
    return -1;
}

auto
decodeHexHash(QStringView hex) -> std::optional<std::array<char, 32>>
{
    if (hex.size() != ArenaSha256Bytes * 2) {
        return std::nullopt;
    }
    std::array<char, 32> result{};
    for (qsizetype i = 0; i < ArenaSha256Bytes; ++i) {
        const auto high = hexNibble(hex[i * 2]);
        const auto low = hexNibble(hex[i * 2 + 1]);
        if (high < 0 || low < 0) {
            return std::nullopt;
        }
        result[static_cast<size_t>(i)] = static_cast<char>((high << 4) | low);
    }
    return result;
}

auto
containsRecord(const QByteArray& packed, const char* target) -> bool
{
    qsizetype first = 0;
    auto last = recordCount(packed);
    while (first < last) {
        const auto middle = first + (last - first) / 2;
        const auto comparison =
          compareRecords(recordAt(packed, middle), target);
        if (comparison < 0) {
            first = middle + 1;
        } else {
            last = middle;
        }
    }
    return first < recordCount(packed) &&
           compareRecords(recordAt(packed, first), target) == 0;
}

auto
vectorsDisjoint(const QByteArray& left, const QByteArray& right) -> bool
{
    qsizetype leftIndex = 0;
    qsizetype rightIndex = 0;
    while (leftIndex < recordCount(left) && rightIndex < recordCount(right)) {
        const auto comparison = compareRecords(recordAt(left, leftIndex),
                                               recordAt(right, rightIndex));
        if (comparison == 0) {
            return false;
        }
        if (comparison < 0) {
            ++leftIndex;
        } else {
            ++rightIndex;
        }
    }
    return true;
}

auto
removeRecords(const QByteArray& current,
              const QByteArray& removed,
              QByteArray& retained) -> bool
{
    retained.clear();
    retained.reserve(std::max<qsizetype>(0, current.size() - removed.size()));
    qsizetype currentIndex = 0;
    qsizetype removedIndex = 0;
    while (currentIndex < recordCount(current)) {
        if (removedIndex >= recordCount(removed)) {
            retained.append(recordAt(current, currentIndex), ArenaSha256Bytes);
            ++currentIndex;
            continue;
        }
        const auto comparison = compareRecords(recordAt(current, currentIndex),
                                               recordAt(removed, removedIndex));
        if (comparison < 0) {
            retained.append(recordAt(current, currentIndex), ArenaSha256Bytes);
            ++currentIndex;
        } else if (comparison == 0) {
            ++currentIndex;
            ++removedIndex;
        } else {
            return false;
        }
    }
    return removedIndex == recordCount(removed);
}

auto
mergeAdded(const QByteArray& retained,
           const QByteArray& added,
           QByteArray& result) -> bool
{
    if (recordCount(retained) + recordCount(added) > ArenaMaxInventoryHashes) {
        return false;
    }
    result.clear();
    result.reserve(retained.size() + added.size());
    qsizetype retainedIndex = 0;
    qsizetype addedIndex = 0;
    while (retainedIndex < recordCount(retained) &&
           addedIndex < recordCount(added)) {
        const auto comparison = compareRecords(
          recordAt(retained, retainedIndex), recordAt(added, addedIndex));
        if (comparison == 0) {
            return false;
        }
        if (comparison < 0) {
            result.append(recordAt(retained, retainedIndex), ArenaSha256Bytes);
            ++retainedIndex;
        } else {
            result.append(recordAt(added, addedIndex), ArenaSha256Bytes);
            ++addedIndex;
        }
    }
    if (retainedIndex < recordCount(retained)) {
        result.append(recordAt(retained, retainedIndex),
                      (recordCount(retained) - retainedIndex) *
                        ArenaSha256Bytes);
    }
    if (addedIndex < recordCount(added)) {
        result.append(recordAt(added, addedIndex),
                      (recordCount(added) - addedIndex) * ArenaSha256Bytes);
    }
    return true;
}

} // namespace

ArenaAvailabilityIndex::ArenaAvailabilityIndex(QObject* parent)
  : QObject(parent)
{
}

auto
ArenaAvailabilityIndex::state() const -> State
{
    return m_state;
}

auto
ArenaAvailabilityIndex::revision() const -> qint64
{
    return m_revision;
}

auto
ArenaAvailabilityIndex::availability(QStringView sha256Hex) const
  -> Availability
{
    if (m_state == State::NotApplicable) {
        return Availability::NotApplicable;
    }
    if (m_state == State::Syncing) {
        return Availability::Syncing;
    }
    const auto hash = decodeHexHash(sha256Hex);
    return hash && containsRecord(m_packedHashes, hash->data())
             ? Availability::AvailableToAll
             : Availability::UnavailableToSome;
}

auto
ArenaAvailabilityIndex::availabilityFor(const QString& sha256Hex) const
  -> Availability
{
    return availability(sha256Hex);
}

auto
ArenaAvailabilityIndex::applyReset(qint64 targetRevision, QByteArray packed)
  -> bool
{
    if (targetRevision <= 0 || targetRevision < m_revision ||
        !validPackedVector(packed)) {
        return false;
    }
    m_packedHashes = std::move(packed);
    m_revision = targetRevision;
    m_state = State::Ready;
    emit changed();
    return true;
}

auto
ArenaAvailabilityIndex::applyDelta(qint64 baseRevision,
                                   qint64 targetRevision,
                                   QByteArray added,
                                   QByteArray removed) -> bool
{
    if (m_state != State::Ready || baseRevision != m_revision ||
        targetRevision <= baseRevision || !validPackedVector(added) ||
        !validPackedVector(removed) ||
        recordCount(added) + recordCount(removed) > ArenaMaxInventoryHashes ||
        !vectorsDisjoint(added, removed)) {
        return false;
    }

    QByteArray retained;
    if (!removeRecords(m_packedHashes, removed, retained)) {
        return false;
    }
    QByteArray merged;
    if (!mergeAdded(retained, added, merged)) {
        return false;
    }

    m_packedHashes = std::move(merged);
    m_revision = targetRevision;
    emit changed();
    return true;
}

void
ArenaAvailabilityIndex::setSyncing()
{
    if (m_state == State::Syncing) {
        return;
    }
    m_state = State::Syncing;
    emit changed();
}

void
ArenaAvailabilityIndex::clear()
{
    if (m_state == State::NotApplicable && m_revision == 0 &&
        m_packedHashes.isEmpty()) {
        return;
    }
    m_state = State::NotApplicable;
    m_revision = 0;
    m_packedHashes.clear();
    emit changed();
}

} // namespace arena
