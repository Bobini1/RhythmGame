#pragma once

#include <QHash>
#include <QImage>
#include <QList>
#include <QRect>
#include <QString>
#include <mutex>

namespace resource_managers {

struct Lr2FontGlyph
{
    int imgIdx = -1;
    QRect rect;
};

struct Lr2FontDict
{
    QList<QImage> textures;
    QHash<char32_t, Lr2FontGlyph> glyphs;
    QHash<int, int> imgMap; // file-declared ID -> index into textures
    int height = 0;
};

// Process-wide cache of parsed .lr2font files. Shared by
// Lr2FontImageProvider (which serves atlas textures) and Lr2FontCatalog
// (which exposes glyph metrics to QML).
class Lr2FontCache
{
  public:
    static Lr2FontCache& instance();

    // Returns nullptr on load failure. Pointer is stable for the lifetime
    // of the process.
    const Lr2FontDict* load(const QString& path);

    Lr2FontCache(const Lr2FontCache&) = delete;
    Lr2FontCache& operator=(const Lr2FontCache&) = delete;

  private:
    Lr2FontCache() = default;

    std::mutex m_mutex;
    QHash<QString, Lr2FontDict> m_cache;
};

} // namespace resource_managers
