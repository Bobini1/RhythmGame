#ifndef RHYTHMGAME_ARENAROUNDLOADER_H
#define RHYTHMGAME_ARENAROUNDLOADER_H

#include "ArenaTypes.h"

#include <QByteArray>
#include <QList>
#include <QObject>

#include <cstdint>
#include <expected>

namespace gameplay_logic {
class ChartData;
class ChartRunner;
} // namespace gameplay_logic

namespace arena {

enum class ArenaSelectionBuildFailure
{
    InvalidChart,
    InvalidSha256,
    InvalidRandomSequence,
    UnsupportedConfig,
};

using ArenaSelectionBuildResult =
  std::expected<SelectionSnapshot, ArenaSelectionBuildFailure>;

enum class ArenaProbeFailure
{
    None,
    MissingFile,
    HashMismatch,
    ReadFailed,
    Cancelled,
};

struct ArenaProbeResult
{
    ArenaProbeFailure failure{ ArenaProbeFailure::None };
    /** Raw digest for local diagnostics; wire failures must omit it. */
    QByteArray observedSha256;
    bool operator==(const ArenaProbeResult&) const = default;
};

enum class ArenaLoadFailure
{
    MissingFile,
    HashMismatch,
    ParseFailed,
    UnsupportedConfig,
    ResourceFailed,
    Cancelled,
};

struct ArenaRoundPlayConfig
{
    QList<qint64> randomSequence;
    NoteOrder noteOrderP1{ NoteOrder::NormalOrMirror };
    NoteOrder noteOrderP2{ NoteOrder::NormalOrMirror };
    DpMode dpMode{ DpMode::Off };
    std::uint64_t laneSeed{};
    int randomizationVersion{ 1 };
    bool operator==(const ArenaRoundPlayConfig&) const = default;
};

struct ArenaRoundLoadRequest
{
    /** Raw 32-byte digest or its 64-character ASCII hex representation. */
    QByteArray sha256;
    ArenaRoundPlayConfig playConfig;
    bool operator==(const ArenaRoundLoadRequest&) const = default;
};

class ArenaRoundLoader : public QObject
{
    Q_OBJECT

  public:
    using QObject::QObject;
    ~ArenaRoundLoader() override = default;

    virtual auto buildSelection(gameplay_logic::ChartData* chart)
      -> ArenaSelectionBuildResult = 0;
    /** Accepts a raw 32-byte digest or 64-character ASCII hex. */
    virtual void probe(quint64 requestId, const QByteArray& sha256) = 0;
    virtual void load(quint64 requestId,
                      const ArenaRoundLoadRequest& request) = 0;
    virtual void cancel(quint64 requestId) = 0;

  signals:
    void probeFinished(quint64 requestId, arena::ArenaProbeResult result);
    void loadFinished(quint64 requestId, gameplay_logic::ChartRunner* runner);
    void loadFailed(quint64 requestId, arena::ArenaLoadFailure failure);
};

} // namespace arena

Q_DECLARE_METATYPE(arena::ArenaProbeResult)
Q_DECLARE_METATYPE(arena::ArenaLoadFailure)

#endif // RHYTHMGAME_ARENAROUNDLOADER_H
