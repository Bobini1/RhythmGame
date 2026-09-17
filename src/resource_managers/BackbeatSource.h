#ifndef RHYTHMGAME_BACKBEATSOURCE_H
#define RHYTHMGAME_BACKBEATSOURCE_H

#include "Tables.h"

#include <QByteArray>
#include <atomic>
#include <filesystem>
#include <memory>
#include <optional>
#include <variant>

namespace resource_managers {

// Owns the SDK store and converts its allocations into ordinary C++ values.
// The store is opened lazily, on the catalog worker, and shared by asset
// readers.
class BackbeatSource
{
  public:
    struct Bundle
    {
        QString filename;
        QByteArray chart;
        QStringList assets;
    };
    struct IndexedBundle
    {
        QString path;
        QString md5;
        QString sha256;
    };
    using Asset = std::variant<std::filesystem::path, QByteArray>;

    BackbeatSource();
    virtual ~BackbeatSource();
    BackbeatSource(const BackbeatSource&) = delete;
    auto operator=(const BackbeatSource&) -> BackbeatSource& = delete;

    virtual auto revision() const -> qint64;
    virtual auto bundles(const std::atomic_bool& cancelled) const
      -> QStringList;
    virtual auto bundle(const QString& id) const -> Bundle;
    virtual auto resolve(const std::filesystem::path& virtualPath) const
      -> std::optional<Asset>;
    virtual auto collections(db::SqliteCppDb* db,
                             const QHash<QString, IndexedBundle>& indexed) const
      -> QList<Table>;

    static auto chartPath(const QString& id, const QString& filename)
      -> std::filesystem::path;
    static auto isPath(const std::filesystem::path& path) -> bool;
    static auto rootPath() -> QString;

  private:
    class Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace resource_managers

#endif
