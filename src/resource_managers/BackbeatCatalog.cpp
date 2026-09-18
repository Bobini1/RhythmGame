#include "BackbeatCatalog.h"

#include "ChartDataFactory.h"
#include "db/SqliteCppDb.h"
#include "support/PathToQString.h"
#include "support/QStringToPath.h"

#include <QFileInfo>
#include <QSet>
#include <QtConcurrent>
#include <chrono>
#include <spdlog/spdlog.h>

namespace resource_managers {

BackbeatCatalog::BackbeatCatalog(std::shared_ptr<BackbeatSource> source,
                                 const std::filesystem::path& databasePath,
                                 db::SqliteCppDb* modelDatabase,
                                 QObject* parent)
  : QObject(parent)
  , source(std::move(source))
  , databasePath(databasePath)
  , modelDatabase(modelDatabase)
{
    refreshTimer.setInterval(std::chrono::seconds(5));
    connect(&refreshTimer, &QTimer::timeout, this, [this] { refresh(); });
    refreshTimer.start();
    connect(&watcher, &QFutureWatcher<Update>::finished, this, [this] {
        active = false;
        const auto update = watcher.result();
        if (update.revision) {
            revision = update.revision;
            emit updated(update.collections);
            if (update.added != 0 || update.removed != 0) {
                emit chartSetChanged();
            }
        }
        if (update.revision || !update.error.isEmpty()) {
            emit errorChanged(update.error);
        }
        emit busyChanged(false);
        if (pendingRefresh) {
            pendingRefresh = false;
            refresh(true);
        }
    });
}

BackbeatCatalog::~BackbeatCatalog()
{
    cancelled = true;
    watcher.waitForFinished();
}

void
BackbeatCatalog::refresh(bool force)
{
    if (active) {
        pendingRefresh = pendingRefresh || force;
        return;
    }
    active = true;
    watcher.setFuture(
      QtConcurrent::run([this, previous = force ? std::nullopt : revision] {
          try {
              return synchronize(previous);
          } catch (const std::exception& error) {
              spdlog::warn("Backbeat library refresh failed: {}", error.what());
              return Update{ .error = QString::fromUtf8(error.what()) };
          }
      }));
}

auto
BackbeatCatalog::synchronize(std::optional<qint64> previousRevision) -> Update
{
    const auto currentRevision = source->revision();
    if (previousRevision == currentRevision || cancelled) {
        return {};
    }
    emit busyChanged(true);
    auto ids = source->bundles(cancelled);
    if (cancelled) {
        return {};
    }
    const QSet<QString> installed(ids.cbegin(), ids.cend());
    db::SqliteCppDb database(databasePath, std::chrono::seconds(5));
    database.execute("CREATE TABLE IF NOT EXISTS backbeat_bundles ("
                     "bundle_id TEXT PRIMARY KEY, path TEXT NOT NULL UNIQUE "
                     "REFERENCES charts(path) ON DELETE CASCADE)");
    auto cachedQuery = database.createStatement(
      "SELECT b.bundle_id, c.path, c.md5, c.sha256 FROM backbeat_bundles b "
      "JOIN charts c ON c.path = b.path");
    struct Cached
    {
        std::string id;
        std::string path;
        std::string md5;
        std::string sha256;
    };
    QHash<QString, BackbeatSource::IndexedBundle> indexed;
    for (const auto& row : cachedQuery.executeAndGetAll<Cached>()) {
        indexed.insert(QString::fromStdString(row.id),
                       { QString::fromStdString(row.path),
                         QString::fromStdString(row.md5),
                         QString::fromStdString(row.sha256) });
    }
    struct Parsed
    {
        QString id;
        std::unique_ptr<gameplay_logic::ChartData> chart;
        QString preview;
        QString readme;
    };
    std::vector<Parsed> parsed;
    QStringList errors;
    for (const auto& id : ids) {
        if (cancelled) {
            return {};
        }
        if (indexed.contains(id)) {
            continue;
        }
        try {
            const auto bundle = source->bundle(id);
            const auto path = BackbeatSource::chartPath(id, bundle.filename);
            const auto contents =
              std::string_view(bundle.chart.constData(),
                               static_cast<size_t>(bundle.chart.size()));
            const ChartDataFactory factory;
            auto components =
              bundle.filename.endsWith(".bmson", Qt::CaseInsensitive)
                ? factory.loadBmsonChartData(contents, path, -1)
                : factory.loadChartData(
                    contents, path, [](auto) { return 1; }, -1);
            Parsed item{ id, std::move(components.chartData), {}, {} };
            for (const auto& asset : bundle.assets) {
                const auto name = QFileInfo(asset).fileName().toLower();
                const auto suffix = QFileInfo(name).suffix();
                const auto assetPath = support::pathToQString(
                  path.parent_path() / support::qStringToPath(asset));
                if (item.readme.isEmpty() && suffix == "txt" &&
                    !name.startsWith('.')) {
                    item.readme = assetPath;
                }
                if (item.preview.isEmpty() && name.startsWith("preview") &&
                    (suffix == "ogg" || suffix == "wav" || suffix == "flac" ||
                     suffix == "mp3")) {
                    item.preview = assetPath;
                }
            }
            indexed.insert(id,
                           { item.chart->getPath(),
                             item.chart->getMd5(),
                             item.chart->getSha256() });
            parsed.push_back(std::move(item));
        } catch (const std::exception& error) {
            spdlog::warn("Could not index Backbeat bundle {}: {}",
                         id.toStdString(),
                         error.what());
            errors.append(QString::fromUtf8(error.what()));
        }
    }
    auto collections = source->collections(modelDatabase, indexed);
    // Pagination and parsing span multiple SDK calls. Only publish a complete,
    // stable snapshot; a concurrent import will be picked up on the next check.
    if (cancelled || source->revision() != currentRevision) {
        return {};
    }
    Update update{ .collections = std::move(collections),
                   .revision = currentRevision };
    {
        auto transaction = database.transaction();
        qint64 directory = -1;
        if (!parsed.empty()) {
            auto insert =
              database.createStatement("INSERT OR IGNORE INTO parent_dir "
                                       "(parent_dir, dir) VALUES (NULL, ?)");
            insert.bind(1, BackbeatSource::rootPath().toStdString());
            insert.execute();
            auto query = database.createStatement(
              "SELECT id FROM parent_dir WHERE dir = ?");
            query.bind(1, BackbeatSource::rootPath().toStdString());
            directory = query.executeAndGet<qint64>().value();
        }
        for (auto it = indexed.cbegin(); it != indexed.cend(); ++it) {
            if (installed.contains(it.key())) {
                continue;
            }
            auto remove =
              database.createStatement("DELETE FROM charts WHERE path = ?");
            remove.bind(1, it->path.toStdString());
            remove.execute();
            ++update.removed;
        }
        for (const auto& item : parsed) {
            item.chart->save(database, directory);
            auto insert = database.createStatement(
              "INSERT INTO backbeat_bundles (bundle_id, path) VALUES (?, ?)");
            insert.bind(1, item.id.toStdString());
            insert.bind(2, item.chart->getPath().toStdString());
            insert.execute();
            const auto saveFile = [&](const char* table, const QString& path) {
                if (path.isEmpty()) {
                    return;
                }
                auto query = database.createStatement(
                  std::string("INSERT OR REPLACE INTO ") + table +
                  " (directory, path) VALUES (?, ?)");
                query.bind(1, item.chart->getChartDirectory().toStdString());
                query.bind(2, path.toStdString());
                query.execute();
            };
            saveFile("preview_files", item.preview);
            saveFile("readme_files", item.readme);
            ++update.added;
        }
        database.execute("DELETE FROM histogram_data WHERE chart_id NOT IN "
                         "(SELECT id FROM charts)");
        database.execute(
          "DELETE FROM preview_files WHERE path GLOB 'backbeat:/*' AND "
          "directory NOT IN (SELECT chart_directory FROM charts)");
        database.execute(
          "DELETE FROM readme_files WHERE path GLOB 'backbeat:/*' AND "
          "directory NOT IN (SELECT chart_directory FROM charts)");
        database.execute(
          "DELETE FROM parent_dir WHERE dir GLOB 'backbeat:/*' AND "
          "id NOT IN (SELECT directory FROM charts WHERE directory IS NOT "
          "NULL)");
        transaction.commit();
    }
    if (!errors.isEmpty()) {
        update.error = tr("Backbeat: %1 chart(s) could not be indexed. %2")
                         .arg(errors.size())
                         .arg(errors.first());
    }
    spdlog::info("Backbeat library: {} new, {} removed, {} collection(s)",
                 update.added,
                 update.removed,
                 update.collections.size());
    return update;
}

} // namespace resource_managers
