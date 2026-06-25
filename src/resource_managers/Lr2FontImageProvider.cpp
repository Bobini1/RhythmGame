#include "Lr2FontImageProvider.h"

#include "Lr2FontCache.h"

#include <QPainter>
#include <QUrl>
#include <QUrlQuery>
#include <algorithm>
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
glyphAdvance(const Lr2FontDict& dict, const Lr2FontGlyph& glyph) -> qreal
{
    return glyph.rect.width() > 0 ? glyph.rect.width()
                                  : fallbackAdvance(dict.height);
}

auto
providerQueryValue(const QUrlQuery& query, const QString& key) -> QString
{
    // QML constructs the provider URL with encodeURIComponent(), and Qt Quick
    // preserves that escaped query through the image-provider request id.
    // Decode the query item explicitly so visible text does not leak
    // "%5B"/"%5D".
    return QUrl::fromPercentEncoding(
      query.queryItemValue(key, QUrl::FullyDecoded).toUtf8());
}

auto
composeTextImage(const QString& fontPath, const QString& text)
  -> Lr2RenderedFontText
{
    const auto* dict = Lr2FontCache::instance().load(fontPath);
    if (!dict || dict->height <= 0 || text.isEmpty()) {
        return {};
    }

    QList<GlyphDraw> glyphs;
    glyphs.reserve(text.size());

    qreal sourceTotalWidth = 0.0;
    int textureHeight = dict->height;
    bool hasPreviousCharacter = false;
    for (const auto codepoint : text.toUcs4()) {
        if (hasPreviousCharacter) {
            sourceTotalWidth += dict->kerning;
        }
        hasPreviousCharacter = true;

        const auto it = dict->glyphs.find(static_cast<char32_t>(codepoint));
        if (it == dict->glyphs.end()) {
            sourceTotalWidth += fallbackAdvance(dict->height);
            continue;
        }

        const auto& glyph = it.value();
        const qreal advance = glyphAdvance(*dict, glyph);

        if (validGlyph(*dict, glyph)) {
            glyphs.append(
              { &dict->textures[glyph.imgIdx], glyph.rect, sourceTotalWidth });
            textureHeight = std::max(textureHeight, glyph.rect.height());
        }

        sourceTotalWidth += advance;
    }

    if (sourceTotalWidth <= 0.0) {
        return {};
    }

    QImage image(std::max(1, static_cast<int>(std::ceil(sourceTotalWidth))),
                 textureHeight,
                 QImage::Format_ARGB32);
    image.fill(Qt::transparent);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
    for (const auto& glyph : glyphs) {
        const QRectF target(glyph.sourceX,
                            0.0,
                            glyph.sourceRect.width(),
                            glyph.sourceRect.height());
        painter.drawImage(target, *glyph.texture, glyph.sourceRect);
    }
    painter.end();

    Lr2RenderedFontText rendered;
    rendered.image = std::move(image);
    rendered.naturalSize = QSizeF(sourceTotalWidth, dict->height);
    return rendered;
}

auto
composeScaledTextImage(const QString& fontPath,
                       const QString& text,
                       const QSize& targetSize,
                       const bool smooth) -> QImage
{
    const auto* dict = Lr2FontCache::instance().load(fontPath);
    if (!dict || dict->height <= 0 || text.isEmpty() || targetSize.isEmpty()) {
        return {};
    }

    QList<GlyphDraw> glyphs;
    glyphs.reserve(text.size());

    qreal sourceTotalWidth = 0.0;
    int textureHeight = dict->height;
    bool hasPreviousCharacter = false;
    for (const auto codepoint : text.toUcs4()) {
        if (hasPreviousCharacter) {
            sourceTotalWidth += dict->kerning;
        }
        hasPreviousCharacter = true;

        const auto it = dict->glyphs.find(static_cast<char32_t>(codepoint));
        if (it == dict->glyphs.end()) {
            sourceTotalWidth += fallbackAdvance(dict->height);
            continue;
        }

        const auto& glyph = it.value();
        const qreal advance = glyphAdvance(*dict, glyph);

        if (validGlyph(*dict, glyph)) {
            glyphs.append(
              { &dict->textures[glyph.imgIdx], glyph.rect, sourceTotalWidth });
            textureHeight = std::max(textureHeight, glyph.rect.height());
        }

        sourceTotalWidth += advance;
    }

    if (sourceTotalWidth <= 0.0 || textureHeight <= 0) {
        return {};
    }

    QImage image(targetSize, QImage::Format_ARGB32);
    image.fill(Qt::transparent);

    const qreal scaleX =
      static_cast<qreal>(targetSize.width()) / sourceTotalWidth;
    const qreal scaleY =
      static_cast<qreal>(targetSize.height()) / textureHeight;

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, smooth);
    for (const auto& glyph : glyphs) {
        const QRectF target(glyph.sourceX * scaleX,
                            0.0,
                            glyph.sourceRect.width() * scaleX,
                            glyph.sourceRect.height() * scaleY);
        painter.drawImage(target, *glyph.texture, glyph.sourceRect);
    }
    painter.end();

    return image;
}

} // namespace

Lr2FontImageProvider::Lr2FontImageProvider()
  : QQuickImageProvider(QQuickImageProvider::Image)
{
}

QImage
Lr2FontImageProvider::textImage(const QString& fontPath, const QString& text)
{
    return renderedText(fontPath, text).image;
}

QImage
Lr2FontImageProvider::scaledTextImage(const QString& fontPath,
                                      const QString& text,
                                      const QSize& targetSize,
                                      const bool smooth)
{
    return composeScaledTextImage(fontPath, text, targetSize, smooth);
}

Lr2RenderedFontText
Lr2FontImageProvider::renderedText(const QString& fontPath, const QString& text)
{
    return composeTextImage(fontPath, text);
}

QImage
Lr2FontImageProvider::requestImage(const QString& id,
                                   QSize* size,
                                   const QSize& /*requestedSize*/)
{
    const auto [path, query] = splitProviderId(id);
    const auto image = textImage(path, providerQueryValue(query, "text"));
    if (size) {
        *size = image.size();
    }
    return image;
}

} // namespace resource_managers
