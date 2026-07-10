#ifndef RHYTHMGAME_QTARENAROUNDLOADER_H
#define RHYTHMGAME_QTARENAROUNDLOADER_H

#include "ArenaRoundLoader.h"

#include <QHash>
#include <QPointer>

#include <atomic>
#include <functional>
#include <memory>
#include <optional>

namespace db {
class SqliteCppDb;
}

namespace qml_components {
class ChartLoader;
class ProfileList;
} // namespace qml_components

template<typename T>
class QFutureWatcher;

namespace arena {

class QtArenaRoundLoader final : public ArenaRoundLoader
{
    Q_OBJECT

  public:
    using PlayConfigProvider =
      std::function<std::optional<resource_managers::ChartPlayConfig>()>;
    using PathResolver =
      std::function<std::optional<QString>(QByteArrayView sha256)>;
    using RunnerLoader = std::function<gameplay_logic::ChartRunner*(
      const QString& path,
      const resource_managers::ChartPlayConfig& config)>;
    using SeedGenerator = std::function<quint64()>;

    QtArenaRoundLoader(PlayConfigProvider playConfigProvider,
                       PathResolver pathResolver,
                       RunnerLoader runnerLoader,
                       SeedGenerator seedGenerator = {},
                       QObject* parent = nullptr);
    QtArenaRoundLoader(qml_components::ProfileList* profileList,
                       db::SqliteCppDb* songDb,
                       qml_components::ChartLoader* chartLoader,
                       QObject* parent = nullptr);
    ~QtArenaRoundLoader() override;

    auto buildSelection(gameplay_logic::ChartData* chart)
      -> ArenaSelectionBuildResult override;
    void probe(quint64 requestId, const QByteArray& sha256) override;
    void load(quint64 requestId, const ArenaRoundLoadRequest& request) override;
    void cancel(quint64 requestId) override;

  private:
    enum class OperationKind
    {
        Probe,
        Load,
    };

    enum class FileCheckFailure
    {
        None,
        MissingFile,
        HashMismatch,
        ReadFailed,
        Cancelled,
    };

    struct FileCheckResult
    {
        FileCheckFailure failure{ FileCheckFailure::None };
        QByteArray observedSha256;
    };

    struct Operation
    {
        OperationKind kind{ OperationKind::Probe };
        quint64 serial{};
        std::shared_ptr<std::atomic_bool> cancelled;
        QPointer<QFutureWatcher<FileCheckResult>> watcher;
        QPointer<gameplay_logic::ChartRunner> runner;
        QString path;
        std::optional<ArenaRoundLoadRequest> loadRequest;
        bool completionReported{};
    };

    PlayConfigProvider m_playConfigProvider;
    PathResolver m_pathResolver;
    RunnerLoader m_runnerLoader;
    SeedGenerator m_seedGenerator;
    QHash<quint64, Operation> m_operations;
    quint64 m_nextSerial{ 1 };

    void discard(quint64 requestId);
    void startFileCheck(quint64 requestId,
                        OperationKind kind,
                        const QByteArray& expectedSha256,
                        QString path,
                        std::optional<ArenaRoundLoadRequest> loadRequest);
    void finishFileCheck(quint64 requestId, quint64 serial);
    void prepareRunner(quint64 requestId, quint64 serial);
    void observeRunner(quint64 requestId, quint64 serial);
    void failLoad(quint64 requestId, quint64 serial, ArenaLoadFailure failure);
};

} // namespace arena

#endif // RHYTHMGAME_QTARENAROUNDLOADER_H
