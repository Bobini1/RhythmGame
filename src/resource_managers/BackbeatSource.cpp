#include "BackbeatSource.h"

#include "db/SqliteCppDb.h"
#include "support/PathToQString.h"
#include "support/QStringToPath.h"

#include <backbeat.h>
#include <QRegularExpression>
#include <QScopeGuard>
#include <mutex>
#include <span>
#include <stdexcept>

namespace resource_managers {
namespace {

void
check(bkb_error_code code)
{
    if (code != BKB_OK) {
        throw std::runtime_error(std::string("Backbeat: ") +
                                 bkb_error_string(code));
    }
}

template<typename T, auto Free, typename Function, typename... Args>
auto
result(Function function, Args... args)
{
    T* value = nullptr;
    check(function(args..., &value));
    return std::unique_ptr<T, decltype(Free)>{ value, Free };
}

auto
string(bkb_str value) -> QString
{
    return QString::fromUtf8(value.ptr, static_cast<qsizetype>(value.len));
}

auto
tags(const bkb_tag* values, size_t size) -> QHash<QString, QString>
{
    QHash<QString, QString> ret;
    for (const auto& tag : std::span(values, size)) {
        ret.insert(string(tag.key), string(tag.value));
    }
    return ret;
}

constexpr const char* gamemodes[] = { "bms-5k",
                                      "bms-7k",
                                      "bms-10k",
                                      "bms-14k" };

auto
makeEntry(const QString& id,
          const QString& description,
          const QString& bundleId,
          const QHash<QString, BackbeatSource::IndexedBundle>& indexed) -> Entry
{
    Entry entry;
    entry.title = description;
    entry.comment = QObject::tr("Managed by Backbeat");
    if (id.startsWith("md5/")) {
        entry.md5 = id.mid(4).toUpper();
    } else if (id.startsWith("sha256/")) {
        entry.sha256 = id.mid(7).toUpper();
    }
    if (const auto found = indexed.constFind(bundleId);
        found != indexed.cend()) {
        entry.path = found->path;
        entry.md5 = found->md5;
        entry.sha256 = found->sha256;
    }
    return entry;
}

void
addEntry(Level& level, Entry entry)
{
    entry.level = level.name;
    if (!entry.md5.isEmpty()) {
        level.md5s.insert(entry.md5, entry);
    }
    level.entries.append(std::move(entry));
}

} // namespace

class BackbeatSource::Impl
{
    std::once_flag opened;
    std::unique_ptr<bkb_store, decltype(&bkb_store_free)> store{
        nullptr,
        bkb_store_free
    };

  public:
    auto get() -> bkb_store*
    {
        std::call_once(opened, [this] {
            store = result<bkb_store, bkb_store_free>(bkb_store_open);
        });
        return store.get();
    }
};

BackbeatSource::BackbeatSource()
  : impl(std::make_unique<Impl>())
{
}
BackbeatSource::~BackbeatSource() = default;

auto
BackbeatSource::rootPath() -> QString
{
    return QStringLiteral("backbeat:/Backbeat/");
}

auto
BackbeatSource::isPath(const std::filesystem::path& path) -> bool
{
    return support::pathToQString(path).startsWith(rootPath());
}

auto
BackbeatSource::chartPath(const QString& id, const QString& filename)
  -> std::filesystem::path
{
    static const QRegularExpression bundleId{ QStringLiteral(
      "^[0-9a-f]{64}$") };
    if (!bundleId.match(id).hasMatch() || filename.isEmpty() ||
        filename == "." || filename == ".." || filename.contains('/') ||
        filename.contains('\\') || filename.contains(':')) {
        throw std::runtime_error("Invalid Backbeat bundle path");
    }
    return support::qStringToPath(rootPath() + id + '/' + filename);
}

auto
BackbeatSource::revision() const -> qint64
{
    bool changed = false;
    int64_t revision = 0;
    check(bkb_store_should_refresh(impl->get(), -1, &changed, &revision));
    return revision;
}

auto
BackbeatSource::bundles(const std::atomic_bool& cancelled) const -> QStringList
{
    constexpr const char* extensions[] = { "bms", "bme", "bml", "bmson" };
    QStringList ids;
    uint64_t offset = 0;
    while (!cancelled) {
        const auto page =
          result<bkb_bundle_search_result, bkb_bundle_search_result_free>(
            bkb_store_search_bundles,
            impl->get(),
            nullptr,
            offset,
            uint32_t{ 256 },
            extensions,
            std::size(extensions));
        for (const auto& chart : std::span(page->charts, page->charts_len)) {
            ids.append(string(chart.bundle_id));
        }
        if (!page->has_more) {
            break;
        }
        if (page->charts_len == 0) {
            throw std::runtime_error(
              "Backbeat returned an empty page with more results");
        }
        offset += page->charts_len;
    }
    return ids;
}

auto
BackbeatSource::bundle(const QString& id) const -> Bundle
{
    const auto bb = result<bkb_bb, bkb_bb_free>(
      bkb_store_get_bundle, impl->get(), id.toUtf8().constData());
    Bundle bundle{ string(bb->filename),
                   QByteArray(reinterpret_cast<const char*>(bb->chart),
                              static_cast<qsizetype>(bb->chart_len)),
                   {} };
    for (const auto& asset : std::span(bb->assets, bb->assets_len)) {
        bundle.assets.append(string(asset.path));
    }
    return bundle;
}

auto
BackbeatSource::resolve(const std::filesystem::path& virtualPath) const
  -> std::optional<Asset>
{
    const auto path = support::pathToQString(virtualPath);
    if (!isPath(virtualPath)) {
        throw std::runtime_error("Not a Backbeat path");
    }
    const auto relative = path.mid(rootPath().size());
    const auto separator = relative.indexOf('/');
    if (separator < 0) {
        throw std::runtime_error("Incomplete Backbeat path");
    }
    const auto id = relative.left(separator);
    const auto requested =
      relative.mid(separator + 1).trimmed().replace('\\', '/');
    if (requested.isEmpty() || requested.startsWith('/') ||
        requested.contains(':') || requested.contains(QChar::Null)) {
        throw std::runtime_error("Invalid Backbeat asset path");
    }
    // Relative segments belong to this bundle's asset-map key. The SDK
    // resolves them inside the bundle, without traversing the filesystem.
    // A chart itself is stored as chart data, not in its asset map.
    const auto extension =
      requested.mid(requested.lastIndexOf('.') + 1).toLower();
    if (extension == "bms" || extension == "bme" || extension == "bml" ||
        extension == "bmson") {
        const auto bb = bundle(id);
        if (bb.filename == requested) {
            return bb.chart;
        }
    }
    bkb_asset_data data{};
    const auto code = bkb_store_resolve_path(impl->get(),
                                             id.toUtf8().constData(),
                                             requested.toUtf8().constData(),
                                             &data);
    if (code == BKB_ERR_NOT_FOUND) {
        return std::nullopt;
    }
    check(code);
    const auto owned = qScopeGuard([&] { bkb_asset_data_free(data); });
    if (data.kind == BKB_ASSET_DATA_FILE) {
        return support::qStringToPath(QString::fromUtf8(
          data.value.file.ptr, static_cast<qsizetype>(data.value.file.len)));
    }
    if (data.kind == BKB_ASSET_DATA_BYTES) {
        return QByteArray(reinterpret_cast<const char*>(data.value.bytes.ptr),
                          static_cast<qsizetype>(data.value.bytes.len));
    }
    throw std::runtime_error("Backbeat returned an unknown asset kind");
}

auto
BackbeatSource::collections(db::SqliteCppDb* db,
                            const QHash<QString, IndexedBundle>& indexed) const
  -> QList<Table>
{
    QList<Table> tables;
    QHash<QString, QString> localMd5s;
    const auto entryFor = [&](const QString& id,
                              const QString& description,
                              const QString& bundleId) {
        auto entry = makeEntry(id, description, bundleId, indexed);
        if (entry.md5.isEmpty() && !entry.sha256.isEmpty()) {
            if (!localMd5s.contains(entry.sha256)) {
                auto query =
                  db->createStatement("SELECT md5 FROM charts WHERE sha256 = ? "
                                      "ORDER BY id LIMIT 1");
                query.bind(1, entry.sha256.toStdString());
                localMd5s.insert(
                  entry.sha256,
                  QString::fromStdString(
                    query.executeAndGet<std::string>().value_or("")));
            }
            entry.md5 = localMd5s.value(entry.sha256);
        }
        return entry;
    };
    const auto base = [](const bkb_collection_metadata& meta,
                         const QString& kind) {
        Table table;
        table.name =
          QObject::tr("Backbeat %1: %2").arg(kind, string(meta.name));
        table.url = QUrl(string(meta.url));
        table.status = Table::Loaded;
        table.managedExternally = true;
        table.keymode = string(meta.gamemode).mid(4).chopped(1).toInt();
        return table;
    };
    const auto installedTables =
      result<bkb_collection_metadata_list, bkb_collection_metadata_list_free>(
        bkb_store_list_tables, impl->get(), gamemodes, std::size(gamemodes));
    for (const auto& meta :
         std::span(installedTables->items, installedTables->items_len)) {
        auto table = base(meta, QObject::tr("table"));
        const auto contents = result<bkb_table, bkb_table_free>(
          bkb_store_get_table,
          impl->get(),
          string(meta.url).toUtf8().constData());
        table.symbol = string(contents->symbol);
        const auto appendLevel = [&](const auto& src, const QString& name) {
            Level level{ db, name };
            for (const auto& chart : std::span(src.charts, src.charts_len)) {
                addEntry(level,
                         entryFor(string(chart.id),
                                  string(chart.desc),
                                  string(chart.bundle_id)));
            }
            table.levels.append(std::move(level));
        };
        for (const auto& level :
             std::span(contents->levels, contents->levels_len)) {
            appendLevel(level, string(level.level));
        }
        for (const auto& folder :
             std::span(contents->folders, contents->folders_len)) {
            appendLevel(folder, string(folder.name));
        }
        tables.append(std::move(table));
    }
    const auto installedPacks =
      result<bkb_collection_metadata_list, bkb_collection_metadata_list_free>(
        bkb_store_list_packs, impl->get(), gamemodes, std::size(gamemodes));
    for (const auto& meta :
         std::span(installedPacks->items, installedPacks->items_len)) {
        auto table = base(meta, QObject::tr("pack"));
        const auto contents = result<bkb_pack, bkb_pack_free>(
          bkb_store_get_pack,
          impl->get(),
          string(meta.url).toUtf8().constData());
        Level level{ db, QObject::tr("Charts") };
        for (const auto& bundle :
             std::span(contents->bundles, contents->bundles_len)) {
            auto entry = entryFor({}, string(bundle.desc), string(bundle.id));
            // Packs specify an exact bundle, including its assets. A matching
            // chart from another bundle is not a substitute for a missing one.
            entry.exactPath = true;
            addEntry(level, std::move(entry));
        }
        table.levels.append(std::move(level));
        tables.append(std::move(table));
    }
    const auto installedCourses =
      result<bkb_collection_metadata_list, bkb_collection_metadata_list_free>(
        bkb_store_list_courses, impl->get(), gamemodes, std::size(gamemodes));
    for (const auto& meta :
         std::span(installedCourses->items, installedCourses->items_len)) {
        auto table = base(meta, QObject::tr("course"));
        const auto contents = result<bkb_course, bkb_course_free>(
          bkb_store_get_course,
          impl->get(),
          string(meta.url).toUtf8().constData());
        const auto rules = tags(contents->tags, contents->tags_len);
        const auto gauge = rules.value("bms/gauge", "lr2");
        const auto lanes = rules.value("bms/allowed-lane-mods", "none");
        Course course{ db };
        course.name = string(contents->name);
        course.originalUrl = string(meta.url);
        course.constraints = { "gauge_lr2",
                               lanes == "mirror" ? "grade_mirror" : "grade" };
        if (gauge != "lr2" || (lanes != "none" && lanes != "mirror")) {
            course.unavailableReason = QObject::tr(
              "This course uses rules RhythmGame does not support.");
        }
        for (const auto& chart :
             std::span(contents->charts, contents->charts_len)) {
            const auto id = string(chart.id);
            const auto entry =
              entryFor(id, string(chart.desc), string(chart.bundle_id));
            course.md5s.append(entry.md5);
            // Preserve the published ID for stable course scores, even before
            // a SHA-256-identified chart has been installed.
            course.sha256s.append(id.startsWith("sha256/") ? id.mid(7).toUpper()
                                                           : QString{});
            course.paths.append(entry.path);
            if (!id.startsWith("md5/") && !id.startsWith("sha256/")) {
                course.unavailableReason = QObject::tr(
                  "This course uses an unsupported chart identifier.");
            }
        }
        if (course.md5s.isEmpty()) {
            continue;
        }
        table.courses.append(QList<Course>{ std::move(course) });
        tables.append(std::move(table));
    }
    return tables;
}

} // namespace resource_managers
