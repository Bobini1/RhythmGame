#include "Lr2FontCatalog.h"

#include "Lr2FontCache.h"

namespace resource_managers {

Lr2FontCatalog::Lr2FontCatalog(QObject* parent) : QObject(parent) {}

int
Lr2FontCatalog::lineHeight(const QString& fontPath)
{
    const auto* dict = Lr2FontCache::instance().load(fontPath);
    return dict ? dict->height : 0;
}

QVariantMap
Lr2FontCatalog::glyph(const QString& fontPath, int codepoint)
{
    QVariantMap out;
    const auto* dict = Lr2FontCache::instance().load(fontPath);
    if (!dict) {
        out["found"] = false;
        out["advance"] = 0;
        return out;
    }

    const auto code = static_cast<char32_t>(codepoint);
    const auto it = dict->glyphs.find(code);
    if (it == dict->glyphs.end() || it.value().imgIdx < 0) {
        out["found"] = false;
        // Treat unknown chars as half-em spaces so strings don't collapse.
        out["advance"] = dict->height > 0 ? dict->height / 2 : 0;
        return out;
    }

    const auto& g = it.value();
    out["found"] = true;
    out["atlas"] = g.imgIdx;
    out["x"] = g.rect.x();
    out["y"] = g.rect.y();
    out["w"] = g.rect.width();
    out["h"] = g.rect.height();
    out["advance"] = g.rect.width();
    return out;
}

QVariantList
Lr2FontCatalog::layout(const QString& fontPath, const QString& text)
{
    QVariantList out;
    const auto* dict = Lr2FontCache::instance().load(fontPath);
    if (!dict) {
        return out;
    }
    out.reserve(text.size());

    // QString iterates UTF-16 code units; surrogate pairs would need extra
    // handling, but LR2 fonts are SJIS-derived and their BMP coverage is
    // the entire use case. One iteration per QChar is fine.
    for (const QChar c : text) {
        out.append(glyph(fontPath, c.unicode()));
    }
    return out;
}

} // namespace resource_managers
