#include "Lr2FontImageProvider.h"

#include "Lr2FontCache.h"

#include <QPainter>
#include <QUrl>
#include <QUrlQuery>
#include <utility>

namespace resource_managers {
namespace {

auto
splitProviderId(const QString& id) -> std::pair<QString, QUrlQuery>
{
    const auto queryStart = id.indexOf('?');
    auto rawPath = queryStart >= 0 ? id.left(queryStart) : id;
    const auto rawQuery = queryStart >= 0 ? id.mid(queryStart + 1) : QString{};

    auto path = QUrl::fromPercentEncoding(rawPath.toUtf8());
    if (path.startsWith('/') && path.size() > 2 && path[2] == ':') {
        path.remove(0, 1);
    }

    return {path, QUrlQuery(rawQuery)};
}

auto
transparentImage(const QSize& imageSize) -> QImage
{
    QImage image(qMax(1, imageSize.width()),
                 qMax(1, imageSize.height()),
                 QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    return image;
}

auto
transparentGlyph(QSize* size, const QSize& imageSize) -> QImage
{
    auto image = transparentImage(imageSize);
    if (size) {
        *size = image.size();
    }
    return image;
}

} // namespace

Lr2FontImageProvider::Lr2FontImageProvider()
  : QQuickImageProvider(QQuickImageProvider::Image)
{
}

QImage
Lr2FontImageProvider::requestImage(const QString& id,
                                   QSize* size,
                                   const QSize& /*requestedSize*/)
{
    const auto [path, query] = splitProviderId(id);

    const auto* dict = Lr2FontCache::instance().load(path);
    if (!dict) {
        if (size) {
            *size = QSize(0, 0);
        }
        return {};
    }

    // Atlas form: return texture N directly. Qt caches the resulting QImage
    // by URL, so all glyphs clipping from the same atlas share one texture.
    if (query.hasQueryItem("atlas")) {
        bool atlasOk = false;
        const int atlasIdx = query.queryItemValue("atlas").toInt(&atlasOk);
        if (atlasIdx < 0 || atlasIdx >= dict->textures.size()) {
            if (size) {
                *size = QSize(0, 0);
            }
            return {};
        }
        const auto& atlas = dict->textures[atlasIdx];
        if (!atlasOk || atlas.isNull() || !query.hasQueryItem("x") ||
            !query.hasQueryItem("y") || !query.hasQueryItem("w") ||
            !query.hasQueryItem("h")) {
            return transparentGlyph(size, QSize(1, 1));
        }

        bool xOk = false;
        bool yOk = false;
        bool wOk = false;
        bool hOk = false;
        const QRect glyphRect(query.queryItemValue("x").toInt(&xOk),
                              query.queryItemValue("y").toInt(&yOk),
                              query.queryItemValue("w").toInt(&wOk),
                              query.queryItemValue("h").toInt(&hOk));
        const QSize requestedGlyphSize(qMax(1, glyphRect.width()),
                                       qMax(1, glyphRect.height()));
        if (!xOk || !yOk || !wOk || !hOk || glyphRect.width() <= 0 ||
            glyphRect.height() <= 0 || !atlas.rect().contains(glyphRect)) {
            return transparentGlyph(size, requestedGlyphSize);
        }

        const auto glyph = atlas.copy(glyphRect);
        if (size) {
            *size = glyph.size();
        }
        return glyph;
    }

    // Legacy form: compose the full string into one image.
    const auto text = query.queryItemValue("text");
    int totalW = 0;
    int maxH = dict->height;

    struct DrawCmd
    {
        const QImage* src;
        QRect rect;
        int dx;
    };
    QList<DrawCmd> cmds;
    cmds.reserve(text.size());

    for (const auto codepoint : text.toUcs4()) {
        const char32_t c32 = static_cast<char32_t>(codepoint);
        if (const auto it = dict->glyphs.find(c32); it != dict->glyphs.end()) {
            const auto& g = it.value();
            if (g.imgIdx >= 0 && g.imgIdx < dict->textures.size() &&
                !dict->textures[g.imgIdx].isNull() && g.rect.width() > 0 &&
                g.rect.height() > 0 &&
                dict->textures[g.imgIdx].rect().contains(g.rect)) {
                cmds.append({&dict->textures[g.imgIdx], g.rect, totalW});
                totalW += g.rect.width();
                maxH = qMax(maxH, g.rect.height());
            } else {
                totalW += g.rect.width() > 0 ? g.rect.width() : maxH / 2;
            }
        } else if (c32 == U' ') {
            totalW += maxH / 2;
        }
    }

    if (totalW <= 0) {
        totalW = 1;
    }
    if (maxH <= 0) {
        maxH = 1;
    }

    QImage result(totalW, maxH, QImage::Format_ARGB32);
    result.fill(Qt::transparent);
    QPainter painter(&result);
    for (const auto& cmd : cmds) {
        painter.drawImage(cmd.dx,
                          0,
                          *cmd.src,
                          cmd.rect.x(),
                          cmd.rect.y(),
                          cmd.rect.width(),
                          cmd.rect.height());
    }

    if (size) {
        *size = result.size();
    }
    return result;
}

} // namespace resource_managers
