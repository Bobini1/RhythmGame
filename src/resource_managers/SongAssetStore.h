#ifndef RHYTHMGAME_SONGASSETSTORE_H
#define RHYTHMGAME_SONGASSETSTORE_H

#include <QByteArray>
#include <QObject>
#include <QString>
#include <QTemporaryDir>

#include <atomic>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

namespace resource_managers {

/**
 * Resolves song-library virtual paths across ordinary directories and nested
 * archives. A path such as
 * C:/songs/collection.zip/song.zip/chart.bms is kept as the public identity;
 * archive boundaries are discovered and handled inside this module.
 */
class SongAssetStore : public QObject
{
    Q_OBJECT

  public:
    struct ArchiveEntry
    {
        std::filesystem::path virtualPath;
        std::optional<QByteArray> contents;
    };

    using WantsContents =
      std::function<bool(const std::filesystem::path& virtualPath)>;
    using EntryVisitor = std::function<void(ArchiveEntry entry)>;

    explicit SongAssetStore(QObject* parent = nullptr);
    ~SongAssetStore() override;

    [[nodiscard]] auto read(const std::filesystem::path& virtualPath,
                            const std::atomic_bool* stop = nullptr) const
      -> QByteArray;
    [[nodiscard]] auto materialize(const std::filesystem::path& virtualPath,
                                   const std::atomic_bool* stop = nullptr) const
      -> std::filesystem::path;
    [[nodiscard]] auto materializeRelative(
      const std::filesystem::path& virtualDirectory,
      const std::vector<std::filesystem::path>& relativePaths,
      const std::atomic_bool* stop = nullptr) const
      -> std::unordered_map<std::filesystem::path, std::filesystem::path>;
    void walkArchive(const std::filesystem::path& archivePath,
                     const WantsContents& wantsContents,
                     const EntryVisitor& visitor,
                     std::atomic_bool* stop = nullptr) const;

    [[nodiscard]] auto isArchived(
      const std::filesystem::path& virtualPath) const -> bool;

    [[nodiscard]] static auto isArchivePath(const std::filesystem::path& path)
      -> bool;
    [[nodiscard]] static auto isSupportedArchivePath(
      const std::filesystem::path& path) -> bool;
    [[nodiscard]] static auto archiveSupportError(
      const std::filesystem::path& path) -> QString;
    [[nodiscard]] static auto isSplitArchivePath(
      const std::filesystem::path& path) -> bool;
    [[nodiscard]] static auto imageUrl(const std::filesystem::path& virtualPath)
      -> QString;
    [[nodiscard]] static auto imageUrl(const QString& virtualDirectory,
                                       const QString& relativePath) -> QString;
    [[nodiscard]] static auto audioUrl(const std::filesystem::path& virtualPath)
      -> QString;
    [[nodiscard]] static auto isAudioUrl(const QString& source) -> bool;
    [[nodiscard]] static auto pathFromUrl(const QString& source)
      -> std::filesystem::path;

    Q_INVOKABLE [[nodiscard]] QString imageSource(
      const QString& virtualDirectory,
      const QString& relativePath) const;
    Q_INVOKABLE [[nodiscard]] QString localFile(
      const QString& virtualPath) const;
    Q_INVOKABLE [[nodiscard]] QString containingFolder(
      const QString& virtualPath) const;

  private:
    class Impl;

    void materializeRequested(
      const std::vector<std::filesystem::path>& virtualPaths,
      const std::atomic_bool* stop = nullptr) const;

    QTemporaryDir temporaryDirectory;
    std::filesystem::path materializationDirectory;
    std::unique_ptr<Impl> impl;
};

} // namespace resource_managers

#endif // RHYTHMGAME_SONGASSETSTORE_H
