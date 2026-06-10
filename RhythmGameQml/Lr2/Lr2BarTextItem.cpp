#include "Lr2BarTextItem.h"

#include "Lr2SelectBarCell.h"
#include "Lr2SkinRuntimeTypes.h"
#include "gameplay_logic/lr2_skin/Lr2SkinParser.h"
#include "resource_managers/Lr2FontImageProvider.h"

#include <QFont>
#include <QFontMetricsF>
#include <QPainter>
#include <QQuickWindow>
#include <QPointF>
#include <QSGNode>
#include <QSGOpacityNode>
#include <QSGSimpleTextureNode>
#include <QSGTexture>
#include <algorithm>
#include <cmath>

namespace {

using gameplay_logic::lr2_skin::Lr2Dst;
namespace rt = lr2skin::runtime;

constexpr qreal maxLayerSize = 16384.0;

struct TextState {
    bool valid = false;
    qreal x = 0.0;
    qreal y = 0.0;
    qreal w = 0.0;
    qreal h = 0.0;
    qreal a = 255.0;
    qreal r = 255.0;
    qreal g = 255.0;
    qreal b = 255.0;
    int blend = 0;
    int filter = 0;
};

QVariantList offsetsAsVariantList(const QVector<int>& offsets) {
    QVariantList result;
    result.reserve(offsets.size());
    for (int offset : offsets) {
        result.append(offset);
    }
    return result;
}

Lr2Dst lr2DstFromRuntimeDst(const rt::Dst& dst) {
    Lr2Dst result;
    result.time = dst.time;
    result.x = dst.x;
    result.y = dst.y;
    result.w = dst.w;
    result.h = dst.h;
    result.acc = dst.acc;
    result.a = dst.a;
    result.r = dst.r;
    result.g = dst.g;
    result.b = dst.b;
    result.blend = dst.blend;
    result.filter = dst.filter;
    result.angle = dst.angle;
    result.center = dst.center;
    result.sortId = dst.sortId;
    result.loop = dst.loop;
    result.timer = dst.timer;
    result.op1 = dst.op1;
    result.op2 = dst.op2;
    result.op3 = dst.op3;
    result.op4 = dst.op4;
    result.offsets = offsetsAsVariantList(dst.offsets);
    return result;
}

QVariant firstDstWithClearedTitleOps(const QVariant& dst) {
    rt::Dst parsed;
    if (!rt::readDst(dst, parsed)) {
        return dst;
    }

    Lr2Dst result = lr2DstFromRuntimeDst(parsed);
    result.op1 = 0;
    result.op2 = 0;
    result.op3 = 0;
    return QVariant::fromValue(result);
}

bool containsUnsupportedBlend(const QVariantList& dsts) {
    for (const QVariant& dst : dsts) {
        rt::Dst parsed;
        if (rt::readDst(dst, parsed) && (parsed.blend == 5 || parsed.blend == 6)) {
            return true;
        }
    }
    return false;
}

TextState stateFromTimelineState(const Lr2TimelineStateValue& value) {
    if (!value.valid) {
        return {};
    }

    TextState state;
    state.valid = true;
    state.x = value.x;
    state.y = value.y;
    state.w = value.w;
    state.h = value.h;
    state.a = value.a;
    state.r = value.r;
    state.g = value.g;
    state.b = value.b;
    state.blend = value.blend;
    state.filter = value.filter;
    return state;
}

QColor colorFromState(const TextState& state) {
    return QColor::fromRgbF(
        std::clamp(state.r, 0.0, 255.0) / 255.0,
        std::clamp(state.g, 0.0, 255.0) / 255.0,
        std::clamp(state.b, 0.0, 255.0) / 255.0,
        1.0);
}

QFont::Weight fontWeightForThickness(int thickness) {
    if (thickness >= 6) {
        return QFont::Bold;
    }
    if (thickness >= 4) {
        return QFont::DemiBold;
    }
    return QFont::Normal;
}

QString colorKey(const QColor& color) {
    return QString::number(color.rgba64().red(), 16)
        + u',' + QString::number(color.rgba64().green(), 16)
        + u',' + QString::number(color.rgba64().blue(), 16)
        + u',' + QString::number(color.rgba64().alpha(), 16);
}

QSizeF sceneScaleForItem(const QQuickItem& item)
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
    return QSizeF(scaleX, scaleY);
}

bool colorNeedsTint(const QColor& color)
{
    return std::abs(color.redF() - 1.0) > 0.001
        || std::abs(color.greenF() - 1.0) > 0.001
        || std::abs(color.blueF() - 1.0) > 0.001;
}

QImage tintedImage(QImage image, const QColor& color)
{
    if (image.isNull() || !colorNeedsTint(color)) {
        return image;
    }

    image = image.convertToFormat(QImage::Format_ARGB32);
    const double redScale = std::clamp(static_cast<double>(color.redF()), 0.0, 1.0);
    const double greenScale = std::clamp(static_cast<double>(color.greenF()), 0.0, 1.0);
    const double blueScale = std::clamp(static_cast<double>(color.blueF()), 0.0, 1.0);
    for (int y = 0; y < image.height(); ++y) {
        auto* line = reinterpret_cast<QRgb*>(image.scanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            const QRgb pixel = line[x];
            const int alpha = qAlpha(pixel);
            if (alpha == 0) {
                continue;
            }
            line[x] = qRgba(
                std::clamp(static_cast<int>(std::lround(qRed(pixel) * redScale)), 0, 255),
                std::clamp(static_cast<int>(std::lround(qGreen(pixel) * greenScale)), 0, 255),
                std::clamp(static_cast<int>(std::lround(qBlue(pixel) * blueScale)), 0, 255),
                alpha);
        }
    }
    return image;
}

void clearChildren(QSGNode* node) {
    while (QSGNode* child = node->firstChild()) {
        node->removeChildNode(child);
        delete child;
    }
}

} // namespace

Lr2BarTextItem::Lr2BarTextItem(QQuickItem* parent)
    : QQuickItem(parent),
      m_timeline(this) {
    setFlag(ItemHasContents, true);
    connect(&m_timeline, &Lr2TimelineState::analysisChanged, this, &Lr2BarTextItem::updateSupported);
}

Lr2BarTextItem::~Lr2BarTextItem() = default;

QVariantList Lr2BarTextItem::dsts() const {
    return m_dsts;
}

void Lr2BarTextItem::setDsts(const QVariantList& value) {
    if (m_dsts == value) {
        return;
    }
    m_dsts = value;
    m_hasUnsupportedBlend = containsUnsupportedBlend(m_dsts);
    updateTimelineDsts();
    emit dstsChanged();
    updateSupported();
    requestSceneUpdate();
}

QVariant Lr2BarTextItem::srcData() const {
    return m_srcData;
}

void Lr2BarTextItem::setSrcData(const QVariant& value) {
    if (m_srcData == value) {
        return;
    }
    m_srcData = value;
    parseSource();
    m_textImageCache.clear();
    emit srcDataChanged();
    updateSupported();
    requestSceneUpdate();
}

QVariantList Lr2BarTextItem::barCells() const {
    return m_barCellValues;
}

void Lr2BarTextItem::setBarCells(const QVariantList& value) {
    if (m_barCellValues == value) {
        return;
    }
    m_barCellValues = value;
    reconnectCells();
    emit barCellsChanged();
    requestSceneUpdate();
}

Lr2BarPositionMap* Lr2BarTextItem::barPositionMap() const {
    return m_barPositionMap;
}

void Lr2BarTextItem::setBarPositionMap(Lr2BarPositionMap* value) {
    if (m_barPositionMap == value) {
        return;
    }
    m_barPositionMap = value;
    reconnectPositionMap();
    emit barPositionMapChanged();
    requestSceneUpdate();
}

qreal Lr2BarTextItem::scaleOverride() const {
    return m_scaleOverride;
}

void Lr2BarTextItem::setScaleOverride(qreal value) {
    if (std::abs(m_scaleOverride - value) <= 0.000001) {
        return;
    }
    m_scaleOverride = value;
    emit scaleOverrideChanged();
    requestSceneUpdate();
}

bool Lr2BarTextItem::isSupported() const {
    return m_supported;
}

void Lr2BarTextItem::parseSource() {
    Source source;
    rt::Source parsed;
    if (rt::readSource(m_srcData, parsed)) {
        source.titleType = parsed.titleType;
        source.align = parsed.align;
        source.fontSize = parsed.fontSize;
        source.fontThickness = parsed.fontThickness;
        source.fontType = parsed.fontType;
        source.bitmapFont = parsed.bitmapFont;
        source.fontPath = parsed.fontPath;
        source.fontFamily = parsed.fontFamily;
    }
    source.isLr2Font = source.bitmapFont || source.fontPath.endsWith(QStringLiteral(".lr2font"), Qt::CaseInsensitive);
    m_source = source;
}

void Lr2BarTextItem::updateTimelineDsts() {
    m_timelineDsts = m_dsts;
    if (!m_timelineDsts.isEmpty()) {
        m_timelineDsts[0] = firstDstWithClearedTitleOps(m_timelineDsts.front());
    }
    m_timeline.setDsts(m_timelineDsts);
}

void Lr2BarTextItem::reconnectCells() {
    for (const QMetaObject::Connection& connection : m_cellConnections) {
        disconnect(connection);
    }
    m_cellConnections.clear();
    m_barCells.clear();
    m_barCells.reserve(m_barCellValues.size());

    for (const QVariant& value : m_barCellValues) {
        auto* cell = qobject_cast<Lr2SelectBarCell*>(value.value<QObject*>());
        m_barCells.append(cell);
        if (!cell) {
            continue;
        }
        m_cellConnections.append(connect(
            cell,
            &Lr2SelectBarCell::coreChanged,
            this,
            [this]() {
                if (m_textImageCache.size() > 512) {
                    m_textImageCache.clear();
                }
                requestSceneUpdate();
            }));
    }
}

void Lr2BarTextItem::reconnectPositionMap() {
    if (m_positionCoordinatesConnection) {
        disconnect(m_positionCoordinatesConnection);
    }
    if (m_positionSlotOffsetConnection) {
        disconnect(m_positionSlotOffsetConnection);
    }
    if (m_positionSlotCountConnection) {
        disconnect(m_positionSlotCountConnection);
    }

    if (!m_barPositionMap) {
        m_positionCoordinatesConnection = {};
        m_positionSlotOffsetConnection = {};
        m_positionSlotCountConnection = {};
        return;
    }

    m_positionCoordinatesConnection = connect(
        m_barPositionMap,
        &Lr2BarPositionMap::coordinatesChanged,
        this,
        &Lr2BarTextItem::requestSceneUpdate);
    m_positionSlotOffsetConnection = connect(
        m_barPositionMap,
        &Lr2BarPositionMap::slotOffsetChanged,
        this,
        &Lr2BarTextItem::requestSceneUpdate);
    m_positionSlotCountConnection = connect(
        m_barPositionMap,
        &Lr2BarPositionMap::slotCountChanged,
        this,
        &Lr2BarTextItem::requestSceneUpdate);
}

void Lr2BarTextItem::updateSupported() {
    const bool next = m_source.titleType >= 0
        && !m_dsts.isEmpty()
        && !m_hasUnsupportedBlend
        && m_timeline.canUseStaticState();
    if (m_supported == next) {
        return;
    }
    m_supported = next;
    emit supportedChanged();
}

void Lr2BarTextItem::requestSceneUpdate() {
    update();
}

Lr2BarTextItem::TextImage Lr2BarTextItem::textImageFor(const QString& text,
                                                       const QColor& color,
                                                       qreal boxHeight,
                                                       bool hasEdge) {
    if (text.isEmpty() || boxHeight <= 0.0) {
        return {};
    }

    const int basePixelSize = std::max(1, static_cast<int>(std::lround(
        m_source.fontSize > 0 ? m_source.fontSize : boxHeight)));
    const QString key = QString::number(m_source.isLr2Font)
        + u'\x1f' + m_source.fontPath
        + u'\x1f' + m_source.fontFamily
        + u'\x1f' + QString::number(m_source.fontSize)
        + u'\x1f' + QString::number(m_source.fontThickness)
        + u'\x1f' + QString::number(m_source.fontType)
        + u'\x1f' + QString::number(basePixelSize)
        + u'\x1f' + colorKey(color)
        + u'\x1f' + text;

    if (const auto it = m_textImageCache.constFind(key); it != m_textImageCache.constEnd()) {
        return *it;
    }

    TextImage result;
    if (m_source.isLr2Font) {
        const auto rendered =
          resource_managers::Lr2FontImageProvider::renderedText(m_source.fontPath, text);
        result.image = tintedImage(rendered.image, color);
        result.naturalSize = rendered.naturalSize;
    } else {
        QFont font(m_source.fontFamily.isEmpty() ? m_source.fontPath : m_source.fontFamily);
        font.setPixelSize(basePixelSize);
        font.setWeight(fontWeightForThickness(m_source.fontThickness));
        font.setHintingPreference(QFont::PreferFullHinting);

        const QFontMetricsF metrics(font);
        const int edge = hasEdge ? 1 : 0;
        const qreal advance = std::max<qreal>(1.0, metrics.horizontalAdvance(text));
        const qreal height = std::max<qreal>(1.0, metrics.height());
        const QSize imageSize(
            std::max(1, static_cast<int>(std::ceil(advance + edge * 2))),
            std::max(1, static_cast<int>(std::ceil(height + edge * 2))));
        QImage image(imageSize, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);

        QPainter painter(&image);
        painter.setRenderHint(QPainter::TextAntialiasing, true);
        painter.setFont(font);
        const QPointF baseline(edge, edge + metrics.ascent());
        if (hasEdge) {
            painter.setPen(QColor::fromRgbF(0.03f, 0.02f, 0.05f, 1.0f));
            const int r = edge;
            const int rr = r * r;
            for (int y = -r; y <= r; ++y) {
                for (int x = -r; x <= r; ++x) {
                    if ((x != 0 || y != 0) && x * x + y * y <= rr) {
                        painter.drawText(baseline + QPointF(x, y), text);
                    }
                }
            }
        }
        painter.setPen(color);
        painter.drawText(baseline, text);
        painter.end();

        result.image = image;
        result.naturalSize = QSizeF(advance, height);
    }

    if (m_textImageCache.size() > 512) {
        m_textImageCache.clear();
    }
    m_textImageCache.insert(key, result);
    return result;
}

QImage Lr2BarTextItem::scaledLr2TextImageFor(const QString& text,
                                             const QColor& color,
                                             const QSize& targetSize)
{
    if (!m_source.isLr2Font || text.isEmpty() || targetSize.isEmpty()) {
        return {};
    }

    const QString key = m_source.fontPath
        + u'\x1f' + QString::number(targetSize.width())
        + u'x' + QString::number(targetSize.height())
        + u'\x1f' + colorKey(color)
        + u'\x1f' + text;
    if (const auto it = m_scaledTextImageCache.constFind(key); it != m_scaledTextImageCache.constEnd()) {
        return *it;
    }

    QImage image =
      resource_managers::Lr2FontImageProvider::scaledTextImage(
          m_source.fontPath,
          text,
          targetSize,
          true);
    image = tintedImage(std::move(image), color);

    if (m_scaledTextImageCache.size() > 512) {
        m_scaledTextImageCache.clear();
    }
    m_scaledTextImageCache.insert(key, image);
    return image;
}

QSGNode* Lr2BarTextItem::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*) {
    if (!window() || !m_supported || !m_barPositionMap || m_barCells.isEmpty()) {
        delete oldNode;
        return nullptr;
    }

    const TextState state = stateFromTimelineState(m_timeline.staticState());
    const qreal boxW = std::min(std::abs(state.w * m_scaleOverride), maxLayerSize);
    const qreal boxH = std::min(std::abs(state.h * m_scaleOverride), maxLayerSize);
    if (!state.valid || state.a <= 0.0 || boxW <= 0.0 || boxH <= 0.0) {
        delete oldNode;
        return nullptr;
    }

    auto* root = oldNode ? oldNode : new QSGNode;
    clearChildren(root);

    const QColor textColor = colorFromState(state);
    const qreal opacity = std::clamp(state.a / 255.0, 0.0, 1.0);
    const bool hasEdge = (m_source.fontType & 1) != 0;
    const qreal anchorOffsetX = m_source.align == 1
        ? -boxW * 0.5
        : (m_source.align == 2 ? -boxW : 0.0);
    const QSizeF sceneScale = sceneScaleForItem(*this);

    for (int slot = 0; slot < m_barCells.size(); ++slot) {
        const auto* cell = m_barCells.at(slot).data();
        if (!cell) {
            continue;
        }

        const int row = m_barPositionMap->rowForSlot(slot);
        if (row <= 0 || row >= m_barPositionMap->count()
            || !m_barPositionMap->validAt(row)) {
            continue;
        }

        const QString text = cell->textForTitleType(m_source.titleType);
        if (text.isEmpty()) {
            continue;
        }

        const TextImage rendered = textImageFor(text, textColor, boxH, hasEdge);
        if (rendered.image.isNull() || rendered.naturalSize.width() <= 0.0 || rendered.naturalSize.height() <= 0.0) {
            continue;
        }

        const qreal scaleY = m_source.isLr2Font
            ? boxH / rendered.naturalSize.height()
            : boxH / std::max<qreal>(1.0, m_source.fontSize > 0 ? m_source.fontSize : boxH);
        const qreal fitScaleX = rendered.naturalSize.width() > boxW && rendered.naturalSize.width() > 0.0
            ? boxW / rendered.naturalSize.width()
            : 1.0;
        const qreal scaleX = scaleY * fitScaleX;
        const qreal drawnW = rendered.naturalSize.width() * scaleX;
        const qreal drawnH = m_source.isLr2Font
            ? rendered.image.height() * scaleY
            : rendered.naturalSize.height() * scaleY;
        const qreal alignedX = m_source.align == 1
            ? (boxW - drawnW) * 0.5
            : (m_source.align == 2 ? boxW - drawnW : 0.0);

        const qreal x = (m_barPositionMap->xAt(row) + state.x) * m_scaleOverride
            + anchorOffsetX + alignedX;
        const qreal y = (m_barPositionMap->yAt(row) + state.y) * m_scaleOverride;
        if (drawnW <= 0.0 || drawnH <= 0.0) {
            continue;
        }

        QImage textureImage = rendered.image;
        if (m_source.isLr2Font) {
            const QSize targetSize(
                std::max(1, static_cast<int>(std::lround(drawnW * sceneScale.width()))),
                std::max(1, static_cast<int>(std::lround(drawnH * sceneScale.height()))));
            const QImage scaledImage = scaledLr2TextImageFor(text, textColor, targetSize);
            if (!scaledImage.isNull()) {
                textureImage = scaledImage;
            }
        }

        auto* texture = window()->createTextureFromImage(textureImage);
        if (!texture) {
            continue;
        }
        const auto filtering = QSGTexture::Linear;
        texture->setFiltering(filtering);
        texture->setHorizontalWrapMode(QSGTexture::ClampToEdge);
        texture->setVerticalWrapMode(QSGTexture::ClampToEdge);

        auto* textureNode = new QSGSimpleTextureNode;
        textureNode->setTexture(texture);
        textureNode->setOwnsTexture(true);
        textureNode->setFiltering(filtering);
        textureNode->setRect(QRectF(x, y, drawnW, drawnH));

        if (opacity < 0.999) {
            auto* opacityNode = new QSGOpacityNode;
            opacityNode->setOpacity(opacity);
            opacityNode->appendChildNode(textureNode);
            root->appendChildNode(opacityNode);
        } else {
            root->appendChildNode(textureNode);
        }
    }

    if (!root->firstChild()) {
        delete root;
        return nullptr;
    }

    return root;
}
