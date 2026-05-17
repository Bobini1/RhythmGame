#include "Lr2BarTextItem.h"

#include "Lr2SelectBarCell.h"
#include "resource_managers/Lr2FontImageProvider.h"

#include <QFont>
#include <QFontMetricsF>
#include <QJSValue>
#include <QMetaProperty>
#include <QPainter>
#include <QQuickWindow>
#include <QSGNode>
#include <QSGOpacityNode>
#include <QSGSimpleTextureNode>
#include <QSGTexture>
#include <QVariantMap>
#include <algorithm>
#include <cmath>

namespace {

constexpr qreal maxLayerSize = 32760.0;

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
};

qreal toReal(const QVariant& value, qreal fallback = 0.0) {
    if (!value.isValid() || value.isNull()) {
        return fallback;
    }
    bool ok = false;
    const qreal result = value.toReal(&ok);
    return ok ? result : fallback;
}

int toInt(const QVariant& value, int fallback = 0) {
    if (!value.isValid() || value.isNull()) {
        return fallback;
    }
    bool ok = false;
    const int result = value.toInt(&ok);
    return ok ? result : fallback;
}

QVariant gadgetProperty(const QVariant& source, const char* name) {
    const QMetaObject* metaObject = source.metaType().metaObject();
    if (!metaObject) {
        return {};
    }

    const int propertyIndex = metaObject->indexOfProperty(name);
    return propertyIndex < 0
        ? QVariant {}
        : metaObject->property(propertyIndex).readOnGadget(source.constData());
}

QVariant valueProperty(const QVariant& source, const char* name) {
    if (!source.isValid() || source.isNull()) {
        return {};
    }
    if (source.canConvert<QVariantMap>()) {
        const QVariantMap map = source.toMap();
        const auto it = map.constFind(QString::fromLatin1(name));
        return it == map.constEnd() ? QVariant() : *it;
    }
    if (source.canConvert<QVariantHash>()) {
        const QVariantHash hash = source.toHash();
        const auto it = hash.constFind(QString::fromLatin1(name));
        return it == hash.constEnd() ? QVariant() : *it;
    }
    if (source.canConvert<QJSValue>()) {
        const QJSValue value = source.value<QJSValue>();
        if (value.isObject()) {
            const QJSValue property = value.property(QString::fromLatin1(name));
            return property.isUndefined() || property.isNull() ? QVariant() : property.toVariant();
        }
    }
    if (source.canConvert<QObject*>()) {
        if (QObject* object = source.value<QObject*>()) {
            return object->property(name);
        }
    }
    return gadgetProperty(source, name);
}

QVariantMap dstAsMap(const QVariant& dst) {
    static const char* names[] = {
        "time", "x", "y", "w", "h", "acc", "a", "r", "g", "b",
        "blend", "filter", "angle", "center", "sortId", "loop",
        "timer", "op1", "op2", "op3", "op4", "offsets",
    };

    QVariantMap result;
    for (const char* name : names) {
        const QVariant value = valueProperty(dst, name);
        if (value.isValid()) {
            result.insert(QString::fromLatin1(name), value);
        }
    }
    return result;
}

QVariant firstDstWithClearedTitleOps(const QVariant& dst) {
    QVariantMap map = dstAsMap(dst);
    map.insert(QStringLiteral("op1"), 0);
    map.insert(QStringLiteral("op2"), 0);
    map.insert(QStringLiteral("op3"), 0);
    return map;
}

bool containsUnsupportedBlend(const QVariantList& dsts) {
    for (const QVariant& dst : dsts) {
        const int blend = toInt(valueProperty(dst, "blend"));
        if (blend == 5 || blend == 6) {
            return true;
        }
    }
    return false;
}

TextState stateFromVariant(const QVariant& value) {
    TextState state;
    if (!value.isValid() || value.isNull()) {
        return state;
    }

    state.valid = true;
    state.x = toReal(valueProperty(value, "x"));
    state.y = toReal(valueProperty(value, "y"));
    state.w = toReal(valueProperty(value, "w"));
    state.h = toReal(valueProperty(value, "h"));
    state.a = toReal(valueProperty(value, "a"), 255.0);
    state.r = toReal(valueProperty(value, "r"), 255.0);
    state.g = toReal(valueProperty(value, "g"), 255.0);
    state.b = toReal(valueProperty(value, "b"), 255.0);
    state.blend = toInt(valueProperty(value, "blend"));
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

Lr2BarPositionCache* Lr2BarTextItem::barPositionCache() const {
    return m_barPositionCache;
}

void Lr2BarTextItem::setBarPositionCache(Lr2BarPositionCache* value) {
    if (m_barPositionCache == value) {
        return;
    }
    m_barPositionCache = value;
    reconnectPositionCache();
    emit barPositionCacheChanged();
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
    source.titleType = toInt(valueProperty(m_srcData, "titleType"), -1);
    source.align = toInt(valueProperty(m_srcData, "align"));
    source.fontSize = toInt(valueProperty(m_srcData, "fontSize"));
    source.fontThickness = toInt(valueProperty(m_srcData, "fontThickness"));
    source.fontType = toInt(valueProperty(m_srcData, "fontType"));
    source.bitmapFont = valueProperty(m_srcData, "bitmapFont").toBool();
    source.fontPath = valueProperty(m_srcData, "fontPath").toString();
    source.fontFamily = valueProperty(m_srcData, "fontFamily").toString();
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
            &Lr2SelectBarCell::revisionChanged,
            this,
            [this]() {
                if (m_textImageCache.size() > 512) {
                    m_textImageCache.clear();
                }
                requestSceneUpdate();
            }));
    }
}

void Lr2BarTextItem::reconnectPositionCache() {
    if (m_positionRevisionConnection) {
        disconnect(m_positionRevisionConnection);
    }
    if (m_positionSlotOffsetConnection) {
        disconnect(m_positionSlotOffsetConnection);
    }
    if (m_positionSlotCountConnection) {
        disconnect(m_positionSlotCountConnection);
    }

    if (!m_barPositionCache) {
        m_positionRevisionConnection = {};
        m_positionSlotOffsetConnection = {};
        m_positionSlotCountConnection = {};
        return;
    }

    m_positionRevisionConnection = connect(
        m_barPositionCache,
        &Lr2BarPositionCache::revisionChanged,
        this,
        &Lr2BarTextItem::requestSceneUpdate);
    m_positionSlotOffsetConnection = connect(
        m_barPositionCache,
        &Lr2BarPositionCache::slotOffsetChanged,
        this,
        &Lr2BarTextItem::requestSceneUpdate);
    m_positionSlotCountConnection = connect(
        m_barPositionCache,
        &Lr2BarPositionCache::slotCountChanged,
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
        QImage image = resource_managers::Lr2FontImageProvider::textImage(m_source.fontPath, text);
        if (!image.isNull()
                && (std::abs(color.redF() - 1.0) > 0.001
                    || std::abs(color.greenF() - 1.0) > 0.001
                    || std::abs(color.blueF() - 1.0) > 0.001)) {
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
        }
        result.image = image;
        result.naturalSize = image.size();
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

QSGNode* Lr2BarTextItem::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*) {
    if (!window() || !m_supported || !m_barPositionCache || m_barCells.isEmpty()) {
        delete oldNode;
        return nullptr;
    }

    const TextState state = stateFromVariant(m_timeline.staticState());
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

    for (int slot = 0; slot < m_barCells.size(); ++slot) {
        const auto* cell = m_barCells.at(slot).data();
        if (!cell) {
            continue;
        }

        const int row = m_barPositionCache->rowForSlot(slot);
        if (row <= 0 || row >= m_barPositionCache->count()) {
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
        const qreal drawnH = m_source.isLr2Font ? boxH : rendered.naturalSize.height() * scaleY;
        const qreal alignedX = m_source.align == 1
            ? (boxW - drawnW) * 0.5
            : (m_source.align == 2 ? boxW - drawnW : 0.0);

        const qreal x = (m_barPositionCache->xAt(row) + state.x) * m_scaleOverride
            + anchorOffsetX + alignedX;
        const qreal y = (m_barPositionCache->yAt(row) + state.y) * m_scaleOverride;
        if (drawnW <= 0.0 || drawnH <= 0.0) {
            continue;
        }

        auto* texture = window()->createTextureFromImage(rendered.image);
        if (!texture) {
            continue;
        }
        texture->setFiltering(QSGTexture::Linear);
        texture->setHorizontalWrapMode(QSGTexture::ClampToEdge);
        texture->setVerticalWrapMode(QSGTexture::ClampToEdge);

        auto* textureNode = new QSGSimpleTextureNode;
        textureNode->setTexture(texture);
        textureNode->setOwnsTexture(true);
        textureNode->setFiltering(QSGTexture::Linear);
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
