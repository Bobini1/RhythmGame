#pragma once

#include <QByteArray>
#include <QObject>
#include <QtTypes>

namespace arena {

struct ArenaInventorySnapshot
{
    qint64 libraryGeneration{};
    QByteArray packedSha256;
    bool operator==(const ArenaInventorySnapshot&) const = default;
};

enum class ArenaInventoryFailure
{
    DatabaseError,
    InvalidHash,
    TooManyCharts,
    Cancelled,
};

class ArenaInventorySource : public QObject
{
    Q_OBJECT

  public:
    using QObject::QObject;
    ~ArenaInventorySource() override = default;

    [[nodiscard]] virtual auto generation() const -> qint64 = 0;
    virtual void requestSnapshot(quint64 requestId) = 0;
    virtual void cancel(quint64 requestId) = 0;

  signals:
    void generationChanged(qint64 generation);
    void snapshotReady(quint64 requestId,
                       arena::ArenaInventorySnapshot snapshot);
    void snapshotFailed(quint64 requestId,
                        arena::ArenaInventoryFailure failure);
};

} // namespace arena

Q_DECLARE_METATYPE(arena::ArenaInventorySnapshot)
Q_DECLARE_METATYPE(arena::ArenaInventoryFailure)
