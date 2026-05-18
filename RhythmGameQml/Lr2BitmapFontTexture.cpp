#include "Lr2BitmapFontTexture.h"

#include "resource_managers/Lr2FontImageProvider.h"

#include <QQuickWindow>
#include <QSGSimpleTextureNode>
#include <QSGTexture>

#include <algorithm>
#include <cmath>

namespace {

QSGTexture::Filtering filteringForFilter(int filter)
{
    return filter == 0 ? QSGTexture::Nearest : QSGTexture::Linear;
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
    rebuildImage();
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
    update();
}

qreal Lr2BitmapFontTexture::naturalWidth() const {
    return m_naturalSize.width();
}

qreal Lr2BitmapFontTexture::naturalHeight() const {
    return m_naturalSize.height();
}

QSGNode* Lr2BitmapFontTexture::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*) {
    if (!window() || m_image.isNull() || width() <= 0.0 || height() <= 0.0) {
        delete oldNode;
        m_textureDirty = false;
        return nullptr;
    }

    auto* node = static_cast<QSGSimpleTextureNode*>(oldNode);
    if (!node || m_textureDirty) {
        delete node;
        node = new QSGSimpleTextureNode;
        auto* texture = window()->createTextureFromImage(m_image);
        if (texture) {
            texture->setHorizontalWrapMode(QSGTexture::ClampToEdge);
            texture->setVerticalWrapMode(QSGTexture::ClampToEdge);
        }
        node->setTexture(texture);
        node->setOwnsTexture(true);
        m_textureDirty = false;
    }

    const auto filtering = filteringForFilter(m_textureFilter);
    if (auto* texture = node->texture()) {
        texture->setFiltering(filtering);
    }
    node->setFiltering(filtering);
    node->setRect(boundingRect());
    return node;
}

void Lr2BitmapFontTexture::geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) {
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    if (newGeometry.size() != oldGeometry.size()) {
        update();
    }
}

void Lr2BitmapFontTexture::rebuildImage() {
    const QSize oldSize = m_naturalSize;

    if (m_fontPath.isEmpty() || m_text.isEmpty()) {
        m_baseImage = {};
        m_image = {};
    } else {
        m_baseImage = resource_managers::Lr2FontImageProvider::textImage(m_fontPath, m_text);
        m_image = colorNeedsTint(m_textColor) ? tintedImage(m_baseImage, m_textColor) : m_baseImage;
    }

    m_naturalSize = m_baseImage.size();
    m_textureDirty = true;

    if (m_naturalSize != oldSize) {
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
