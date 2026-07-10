#pragma once

#include "ArenaInventorySource.h"

#include <QFutureWatcher>

#include <filesystem>
#include <optional>

namespace arena {

class SqliteArenaInventorySource final : public ArenaInventorySource
{
    Q_OBJECT

  public:
    explicit SqliteArenaInventorySource(std::filesystem::path songDbPath,
                                        QObject* parent = nullptr);
    ~SqliteArenaInventorySource() override;

    [[nodiscard]] auto generation() const -> qint64 override;
    void requestSnapshot(quint64 requestId) override;
    void cancel(quint64 requestId) override;

  public slots:
    void commitLibraryMutation();

  private:
    struct BuildRequest
    {
        quint64 requestId{};
        qint64 generation{};
        bool cancelled{};
        bool superseded{};
    };

    struct BuildResult
    {
        qint64 generation{};
        QByteArray packedSha256;
        std::optional<ArenaInventoryFailure> failure;
    };

    static auto buildSnapshot(const std::filesystem::path& songDbPath,
                              qint64 generation) -> BuildResult;
    void startBuild(BuildRequest request);
    void handleBuildFinished();
    void setPending(BuildRequest request);

    std::filesystem::path m_songDbPath;
    qint64 m_generation{ 1 };
    QFutureWatcher<BuildResult> m_watcher;
    std::optional<BuildRequest> m_active;
    std::optional<BuildRequest> m_pending;
};

} // namespace arena
