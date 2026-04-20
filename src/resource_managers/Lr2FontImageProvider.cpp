#include "Lr2FontImageProvider.h"

#include "Lr2FontCache.h"

#include <QCache>
#include <QMutex>
#include <QMutexLocker>
#include <QPainter>
#include <QUrl>
#include <QUrlQuery>
#include <algorithm>
#include <climits>
#include <cmath>
#include <utility>

namespace resource_managers {
namespace {

struct GlyphDraw
{
    const QImage* texture = nullptr;
    QRect sourceRect;
    qreal sourceX = 0.0;
};

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

    return { path, QUrlQuery(rawQuery) };
}

auto
fallbackAdvance(int lineHeight) -> qreal
{
    return lineHeight > 0 ? lineHeight / 2.0 : 0.0;
}

auto
validGlyph(const Lr2FontDict& dict, const Lr2FontGlyph& glyph) -> bool
{
    return glyph.imgIdx >= 0 && glyph.imgIdx < dict.textures.size() &&
           !dict.textures[glyph.imgIdx].isNull() && glyph.rect.width() > 0 &&
           glyph.rect.height() > 0 &&
           dict.textures[glyph.imgIdx].rect().contains(glyph.rect);
}

auto
textImageCache() -> QCache<QString, QImage>&
{
    static QCache<QString, QImage> cache;
    static const bool initialized = [] {
        cache.setMaxCost(64 * 1024 * 1024);
        return true;
    }();
    Q_UNUSED(initialized);
    return cache;
}

auto
textImageCacheMutex() -> QMutex&
{
    static QMutex mutex;
    return mutex;
}

auto
cacheKey(const QString& fontPath, const QString& text) -> QString
{
    auto key = fontPath;
    key.append(QChar(0x1f));
    key.append(text);
    return key;
}

auto
providerQueryValue(const QUrlQuery& query, const QString& key) -> QString
{
    // QML constructs the provider URL with encodeURIComponent(), and Qt Quick
    // preserves that escaped query through the image-provider request id. Decode
    // the query item explicitly so visible text does not leak "%5B"/"%5D".
    return QUrl::fromPercentEncoding(
      query.queryItemValue(key, QUrl::FullyDecoded).toUtf8());
}

auto
cachedTextImage(const QString& fontPath, const QString& text) -> QImage
{
    const auto key = cacheKey(fontPath, text);
    {
        QMutexLocker lock(&textImageCacheMutex());
        if (const auto* cached = textImageCache().object(key)) {
            return *cached;
        }
    }

    const auto* dict = Lr2FontCache::instance().load(fontPath);
    if (!dict || dict->height <= 0 || text.isEmpty()) {
        return {};
    }

    QList<GlyphDraw> glyphs;
    glyphs.reserve(text.size());

    qreal sourceTotalWidth = 0.0;
    for (const auto codepoint : text.toUcs4()) {
        const auto it = dict->glyphs.find(static_cast<char32_t>(codepoint));
        if (it == dict->glyphs.end()) {
            sourceTotalWidth += fallbackAdvance(dict->height);
            continue;
        }

        const auto& glyph = it.value();
        const qreal advance = glyph.rect.width() > 0
                                ? glyph.rect.width()
                                : fallbackAdvance(dict->height);

        if (validGlyph(*dict, glyph)) {
            glyphs.append({ &dict->textures[glyph.imgIdx],
                            glyph.rect,
                            sourceTotalWidth });
        }

        sourceTotalWidth += advance;
    }

    if (sourceTotalWidth <= 0.0) {
        return {};
    }

    QImage image(std::max(1, static_cast<int>(std::ceil(sourceTotalWidth))),
                 dict->height,
                 QImage::Format_ARGB32);
    image.fill(Qt::transparent);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    for (const auto& glyph : glyphs) {
        const QRectF target(glyph.sourceX,
                            0.0,
                            glyph.sourceRect.width(),
                            dict->height);
        painter.drawImage(target, *glyph.texture, glyph.sourceRect);
    }
    painter.end();

    const auto imageCost = std::max<qsizetype>(1, image.sizeInBytes());
    {
        QMutexLocker lock(&textImageCacheMutex());
        textImageCache().insert(
          key,
          new QImage(image),
          static_cast<int>(std::min<qsizetype>(imageCost, INT_MAX)));
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
    const auto image = cachedTextImage(path, providerQueryValue(query, "text"));
    if (size) {
        *size = image.size();
    }
    return image;
}

} // namespace resource_managers
