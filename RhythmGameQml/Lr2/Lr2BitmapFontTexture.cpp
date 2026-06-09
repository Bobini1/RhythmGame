#include "Lr2BitmapFontTexture.h"

#include "resource_managers/Lr2FontImageProvider.h"

#include <QQuickWindow>
#include <QPointF>
#include <QSGSimpleTextureNode>
#include <QSGTexture>

#include <algorithm>
#include <cmath>

namespace {

QSize sceneTextureSizeForItemSize(const QQuickItem& item, const QSizeF& itemSize)
{
    qreal dpr = 1.0;
    if (const auto* window = item.window()) {
        dpr = std::max<qreal>(1.0, window->devicePixelRatio());
    }

    const QPointF origin = item.mapToScene(QPointF(0.0, 0.0));
    const QPointF xUnit = item.mapToScene(QPointF(1.0, 0.0));
    const QPointF yUnit = item.mapToScene(QPointF(0.0, 1.0));
    qreal scaleX = std::hypot(xUnit.x() - origin.x(), xUnit.y() - origin.y()) * dpr;
    qreal scaleY = std::hypot(yUnit.x() - origin.x(), yUnit.y() - origin.y()) * dpr;
    if (!std::isfinite(scaleX) || scaleX <= 0.0) {
        scaleX = 1.0;
    }
    if (!std::isfinite(scaleY) || scaleY <= 0.0) {
        scaleY = 1.0;
    }

    return QSize(std::max(1, static_cast<int>(std::lround(itemSize.width() * scaleX))),
                 std::max(1, static_cast<int>(std::lround(itemSize.height() * scaleY))));
}

QImage scaledNearestEndpoint(const QImage& image, const QSize& targetSize)
{
    if (image.isNull() || targetSize.isEmpty()) {
        return {};
    }
    if (image.size() == targetSize) {
        return image;
    }

    const QImage source = image.convertToFormat(QImage::Format_ARGB32);
    QImage scaled(targetSize, QImage::Format_ARGB32);

    const int sourceW = source.width();
    const int sourceH = source.height();
    const int targetW = targetSize.width();
    const int targetH = targetSize.height();

    for (int y = 0; y < targetH; ++y) {
        const int sourceY = targetH <= 1
            ? 0
            : static_cast<int>(std::lround(
                static_cast<double>(y) * (sourceH - 1) / (targetH - 1)));
        const auto* sourceLine = reinterpret_cast<const QRgb*>(source.constScanLine(sourceY));
        auto* targetLine = reinterpret_cast<QRgb*>(scaled.scanLine(y));
        for (int x = 0; x < targetW; ++x) {
            const int sourceX = targetW <= 1
                ? 0
                : static_cast<int>(std::lround(
                    static_cast<double>(x) * (sourceW - 1) / (targetW - 1)));
            targetLine[x] = sourceLine[sourceX];
        }
    }

    return scaled;
}

} // namespace

Lr2BitmapFontTexture::Lr2BitmapFontTexture(QQuickItem* parent) : QQuickItem(parent) {
    setFlag(ItemHasContents, true);
}

QString Lr2BitmapFontTexture::fontPath() const {
    return m_fontPath;
}

void Lr2BitmapFontTexture::setFontPath(const QString& value) {
    if (m_fontPath == value) {
        return;
    }

    m_fontPath = value;
    emit fontPathChanged();
    rebuildImage();
}

QString Lr2BitmapFontTexture::text() const {
    return m_text;
}

void Lr2BitmapFontTexture::setText(const QString& value) {
    if (m_text == value) {
        return;
    }

    m_text = value;
    emit textChanged();
    if (updateRenderedText()) {
        rebuildImage();
    }
}

QColor Lr2BitmapFontTexture::textColor() const {
    return m_textColor;
}

void Lr2BitmapFontTexture::setTextColor(const QColor& value) {
    if (m_textColor == value) {
        return;
    }

    m_textColor = value;
    emit textColorChanged();
    m_image = colorNeedsTint(m_textColor) ? tintedImage(m_baseImage, m_textColor) : m_baseImage;
    m_textureDirty = true;
    update();
}

int Lr2BitmapFontTexture::textureFilter() const {
    return m_textureFilter;
}

void Lr2BitmapFontTexture::setTextureFilter(int value) {
    if (m_textureFilter == value) {
        return;
    }

    m_textureFilter = value;
    emit textureFilterChanged();
    m_textureDirty = true;
    update();
}

int Lr2BitmapFontTexture::alignment() const {
    return m_alignment;
}

void Lr2BitmapFontTexture::setAlignment(int value) {
    if (m_alignment == value) {
        return;
    }

    m_alignment = value;
    emit alignmentChanged();
    update();
}

bool Lr2BitmapFontTexture::uppercase() const {
    return m_uppercase;
}

void Lr2BitmapFontTexture::setUppercase(bool value) {
    if (m_uppercase == value) {
        return;
    }

    m_uppercase = value;
    emit uppercaseChanged();
    if (updateRenderedText()) {
        rebuildImage();
    }
}

bool Lr2BitmapFontTexture::isActive() const {
    return m_active;
}

void Lr2BitmapFontTexture::setActive(bool value) {
    if (m_active == value) {
        return;
    }

    m_active = value;
    emit activeChanged();
    if (updateRenderedText()) {
        rebuildImage();
    }
}

qreal Lr2BitmapFontTexture::naturalWidth() const {
    return m_naturalSize.width();
}

qreal Lr2BitmapFontTexture::naturalHeight() const {
    return m_naturalSize.height();
}

qreal Lr2BitmapFontTexture::textureHeight() const {
    return m_baseImage.height();
}

QSGNode* Lr2BitmapFontTexture::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*) {
    const QRectF targetRect = renderRect();
    if (!window() || m_image.isNull() || targetRect.isEmpty()) {
        delete oldNode;
        m_textureDirty = false;
        return nullptr;
    }

    auto* node = static_cast<QSGSimpleTextureNode*>(oldNode);
    if (!node || m_textureDirty) {
        delete node;
        node = new QSGSimpleTextureNode;
        const QSize targetSize = sceneTextureSizeForItemSize(*this, targetRect.size());
        QImage textureImage =
          resource_managers::Lr2FontImageProvider::scaledTextImage(
              m_fontPath,
              m_renderedText,
              targetSize,
              m_textureFilter != 0);
        if (textureImage.isNull()) {
            textureImage = m_textureFilter == 0
                ? scaledNearestEndpoint(m_image, targetSize)
                : m_image;
        } else if (colorNeedsTint(m_textColor)) {
            textureImage = tintedImage(textureImage, m_textColor);
        }
        auto* texture = window()->createTextureFromImage(textureImage);
        if (texture) {
            texture->setHorizontalWrapMode(QSGTexture::ClampToEdge);
            texture->setVerticalWrapMode(QSGTexture::ClampToEdge);
        }
        node->setTexture(texture);
        node->setOwnsTexture(true);
        m_textureDirty = false;
    }

    const auto filtering = QSGTexture::Nearest;
    if (auto* texture = node->texture()) {
        texture->setFiltering(filtering);
    }
    node->setFiltering(filtering);
    node->setRect(targetRect);
    return node;
}

void Lr2BitmapFontTexture::geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) {
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    if (newGeometry.size() != oldGeometry.size()) {
        m_textureDirty = true;
        update();
    }
}

QRectF Lr2BitmapFontTexture::renderRect() const {
    if (m_fontPath.isEmpty()
            || m_renderedText.isEmpty()
            || width() <= 0.0
            || height() <= 0.0
            || m_naturalSize.width() <= 0.0
            || m_naturalSize.height() <= 0.0
            || m_baseImage.height() <= 0) {
        return {};
    }

    const qreal scaleY = height() / m_naturalSize.height();
    const qreal fitScaleX = m_naturalSize.width() > width()
        ? width() / m_naturalSize.width()
        : 1.0;
    const qreal drawnWidth = m_naturalSize.width() * scaleY * fitScaleX;
    const qreal drawnHeight = m_baseImage.height() * scaleY;
    qreal x = 0.0;
    if (m_alignment == 1) {
        x = (width() - drawnWidth) / 2.0;
    } else if (m_alignment == 2) {
        x = width() - drawnWidth;
    }
    return QRectF(x, 0.0, drawnWidth, drawnHeight);
}

bool Lr2BitmapFontTexture::updateRenderedText() {
    const QString nextText = !m_active ? QString() : (m_uppercase ? m_text.toUpper() : m_text);
    if (m_renderedText == nextText) {
        return false;
    }

    m_renderedText = nextText;
    return true;
}

void Lr2BitmapFontTexture::rebuildImage() {
    const QSizeF oldNaturalSize = m_naturalSize;
    const int oldTextureHeight = m_baseImage.height();

    if (m_fontPath.isEmpty() || m_renderedText.isEmpty()) {
        m_baseImage = {};
        m_image = {};
        m_naturalSize = {};
    } else {
        const auto rendered =
          resource_managers::Lr2FontImageProvider::renderedText(m_fontPath, m_renderedText);
        m_baseImage = rendered.image;
        m_naturalSize = rendered.naturalSize;
        m_image = colorNeedsTint(m_textColor) ? tintedImage(m_baseImage, m_textColor) : m_baseImage;
    }

    m_textureDirty = true;

    if (m_naturalSize != oldNaturalSize || m_baseImage.height() != oldTextureHeight) {
        emit naturalSizeChanged();
    }

    update();
}

bool Lr2BitmapFontTexture::colorNeedsTint(const QColor& color) {
    return std::abs(color.redF() - 1.0) > 0.001 || std::abs(color.greenF() - 1.0) > 0.001 ||
           std::abs(color.blueF() - 1.0) > 0.001;
}

QImage Lr2BitmapFontTexture::tintedImage(const QImage& image, const QColor& color) {
    if (image.isNull()) {
        return {};
    }

    QImage tinted = image.convertToFormat(QImage::Format_ARGB32);
    const double redScale = std::clamp(static_cast<double>(color.redF()), 0.0, 1.0);
    const double greenScale = std::clamp(static_cast<double>(color.greenF()), 0.0, 1.0);
    const double blueScale = std::clamp(static_cast<double>(color.blueF()), 0.0, 1.0);

    for (int y = 0; y < tinted.height(); ++y) {
        auto* line = reinterpret_cast<QRgb*>(tinted.scanLine(y));
        for (int x = 0; x < tinted.width(); ++x) {
            const QRgb pixel = line[x];
            const int alpha = qAlpha(pixel);
            if (alpha == 0) {
                continue;
            }

            line[x] = qRgba(
                std::clamp(static_cast<int>(std::lround(qRed(pixel) * redScale)), 0, 255),
                std::clamp(static_cast<int>(std::lround(qGreen(pixel) * greenScale)), 0, 255),
                std::clamp(static_cast<int>(std::lround(qBlue(pixel) * blueScale)), 0, 255), alpha);
        }
    }

    return tinted;
}
