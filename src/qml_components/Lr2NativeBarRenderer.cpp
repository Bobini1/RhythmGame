#include "Lr2NativeBarRenderer.h"

#include "gameplay_logic/lr2_skin/Lr2SkinParser.h"
#include "resource_managers/Lr2FontImageProvider.h"

#include <QCache>
#include <QFont>
#include <QFontMetricsF>
#include <QMetaObject>
#include <QMutex>
#include <QMutexLocker>
#include <QPainter>
#include <QQmlProperty>
#include <QUrl>
#include <algorithm>
#include <climits>
#include <cmath>

namespace qml_components {

namespace {

using gameplay_logic::lr2_skin::Lr2SrcBarNumber;
using gameplay_logic::lr2_skin::Lr2SrcBarText;
using gameplay_logic::lr2_skin::Lr2SrcNumber;

struct DrawState
{
    qreal x = 0.0;
    qreal y = 0.0;
    qreal w = 0.0;
    qreal h = 0.0;
    int a = 255;
    int r = 255;
    int g = 255;
    int b = 255;
    int blend = 1;
    bool valid = false;
};

auto
variantMap(const QVariant& value) -> QVariantMap
{
    if (value.metaType().id() == QMetaType::QVariantMap ||
        value.canConvert<QVariantMap>()) {
        return value.toMap();
    }

    if (auto* object = value.value<QObject*>()) {
        QVariantMap map;
        const auto* meta = object->metaObject();
        for (int i = 0; i < meta->propertyCount(); ++i) {
            const auto prop = meta->property(i);
            map.insert(QString::fromUtf8(prop.name()), prop.read(object));
        }
        return map;
    }

    return {};
}

auto
barTextFromVariant(const QVariant& value) -> Lr2SrcBarText
{
    if (value.canConvert<Lr2SrcBarText>()) {
        return value.value<Lr2SrcBarText>();
    }

    const auto map = variantMap(value);
    Lr2SrcBarText src;
    src.titleType = map.value(QStringLiteral("titleType")).toInt();
    src.font = map.value(QStringLiteral("font")).toInt();
    src.st = map.value(QStringLiteral("st")).toInt();
    src.align = map.value(QStringLiteral("align")).toInt();
    src.edit = map.value(QStringLiteral("edit")).toInt();
    src.panel = map.value(QStringLiteral("panel")).toInt();
    src.fontPath = map.value(QStringLiteral("fontPath")).toString();
    src.fontFamily = map.value(QStringLiteral("fontFamily")).toString();
    src.fontSize = map.value(QStringLiteral("fontSize")).toInt();
    src.fontThickness = map.value(QStringLiteral("fontThickness")).toInt();
    src.fontType = map.value(QStringLiteral("fontType")).toInt();
    src.bitmapFont = map.value(QStringLiteral("bitmapFont")).toBool();
    return src;
}

auto
numberFromVariant(const QVariant& value) -> Lr2SrcNumber
{
    if (value.canConvert<Lr2SrcNumber>()) {
        return value.value<Lr2SrcNumber>();
    }

    const auto map = variantMap(value);
    Lr2SrcNumber src;
    src.gr = map.value(QStringLiteral("gr")).toInt();
    src.x = map.value(QStringLiteral("x")).toInt();
    src.y = map.value(QStringLiteral("y")).toInt();
    src.w = map.value(QStringLiteral("w")).toInt();
    src.h = map.value(QStringLiteral("h")).toInt();
    src.div_x = map.value(QStringLiteral("div_x"), 1).toInt();
    src.div_y = map.value(QStringLiteral("div_y"), 1).toInt();
    src.cycle = map.value(QStringLiteral("cycle")).toInt();
    src.timer = map.value(QStringLiteral("timer")).toInt();
    src.num = map.value(QStringLiteral("num")).toInt();
    src.align = map.value(QStringLiteral("align")).toInt();
    src.keta = map.value(QStringLiteral("keta")).toInt();
    src.source = map.value(QStringLiteral("source")).toString();
    return src;
}

auto
barNumberFromVariant(const QVariant& value) -> Lr2SrcBarNumber
{
    if (value.canConvert<Lr2SrcBarNumber>()) {
        return value.value<Lr2SrcBarNumber>();
    }

    const auto map = variantMap(value);
    Lr2SrcBarNumber src;
    src.kind = map.value(QStringLiteral("kind")).toInt();
    src.variant = map.value(QStringLiteral("variant")).toInt();
    src.source = map.value(QStringLiteral("source"));
    return src;
}

auto
stateFromVariant(const QVariant& value) -> DrawState
{
    const auto map = variantMap(value);
    if (map.isEmpty()) {
        return {};
    }

    DrawState state;
    state.x = map.value(QStringLiteral("x")).toReal();
    state.y = map.value(QStringLiteral("y")).toReal();
    state.w = map.value(QStringLiteral("w")).toReal();
    state.h = map.value(QStringLiteral("h")).toReal();
    state.a = map.value(QStringLiteral("a"), 255).toInt();
    state.r = map.value(QStringLiteral("r"), 255).toInt();
    state.g = map.value(QStringLiteral("g"), 255).toInt();
    state.b = map.value(QStringLiteral("b"), 255).toInt();
    state.blend = map.value(QStringLiteral("blend"), 1).toInt();
    state.valid = true;
    return state;
}

auto
interpolatedRowState(const QVariantList& states, int row, qreal offset)
  -> DrawState
{
    if (row < 0 || row >= states.size()) {
        return {};
    }

    auto from = stateFromVariant(states[row]);
    if (!from.valid || offset <= 0.001 || row <= 0) {
        return from;
    }

    auto to = stateFromVariant(states[row - 1]);
    if (!to.valid) {
        return from;
    }

    const auto t = std::clamp(offset, 0.0, 1.0);
    const auto inv = 1.0 - t;
    from.x = from.x * inv + to.x * t;
    from.y = from.y * inv + to.y * t;
    from.w = from.w * inv + to.w * t;
    from.h = from.h * inv + to.h * t;
    from.a = static_cast<int>(std::round(from.a * inv + to.a * t));
    return from;
}

auto
invoke(QObject* object, const char* method, const QVariant& arg) -> QVariant
{
    if (!object) {
        return {};
    }
    QVariant result;
    QMetaObject::invokeMethod(object,
                              method,
                              Q_RETURN_ARG(QVariant, result),
                              Q_ARG(QVariant, arg));
    return result;
}

auto
invoke(QObject* object,
       const char* method,
       const QVariant& arg1,
       const QVariant& arg2) -> QVariant
{
    if (!object) {
        return {};
    }
    QVariant result;
    QMetaObject::invokeMethod(object,
                              method,
                              Q_RETURN_ARG(QVariant, result),
                              Q_ARG(QVariant, arg1),
                              Q_ARG(QVariant, arg2));
    return result;
}

auto
barEntry(QObject* context, int row, int center) -> QVariant
{
    return invoke(context, "barEntry", row, center);
}

auto
localPathForSource(const QString& source) -> QString
{
    const auto url = QUrl{ source };
    if (url.isLocalFile()) {
        return url.toLocalFile();
    }
    if (url.scheme() == QStringLiteral("qrc")) {
        return QStringLiteral(":") + url.path();
    }
    if (source.startsWith(QStringLiteral("file:///")) ||
        source.startsWith(QStringLiteral("file://"))) {
        return url.toLocalFile();
    }
    return source;
}

auto
imageCache() -> QCache<QString, QImage>&
{
    static QCache<QString, QImage> cache;
    static const bool initialized = [] {
        cache.setMaxCost(96 * 1024 * 1024);
        return true;
    }();
    Q_UNUSED(initialized);
    return cache;
}

auto
imageCacheMutex() -> QMutex&
{
    static QMutex mutex;
    return mutex;
}

auto
cachedImage(const QString& source) -> QImage
{
    if (source.isEmpty()) {
        return {};
    }

    {
        QMutexLocker lock(&imageCacheMutex());
        if (const auto* cached = imageCache().object(source)) {
            return *cached;
        }
    }

    auto image = QImage{ localPathForSource(source) };
    if (image.isNull()) {
        return {};
    }

    const auto cost =
      std::max<qsizetype>(1, std::min<qsizetype>(image.sizeInBytes(), INT_MAX));
    {
        QMutexLocker lock(&imageCacheMutex());
        imageCache().insert(source, new QImage(image), static_cast<int>(cost));
    }
    return image;
}

auto
solidColorForState(const DrawState& state) -> QColor
{
    return QColor{ std::clamp(state.r, 0, 255),
                   std::clamp(state.g, 0, 255),
                   std::clamp(state.b, 0, 255) };
}

void
configurePaintedItem(QQuickPaintedItem& item)
{
    item.setAntialiasing(false);
    item.setMipmap(false);
    item.setOpaquePainting(false);
    item.setFillColor(Qt::transparent);
}

void
drawBitmapFontText(QPainter* painter,
                   const QString& fontPath,
                   const QString& text,
                   int alignment,
                   const QRectF& box)
{
    const auto image =
      resource_managers::Lr2FontImageProvider::textImage(fontPath, text);
    if (image.isNull() || box.width() <= 0.0 || box.height() <= 0.0) {
        return;
    }

    const auto naturalWidth = static_cast<qreal>(image.width());
    const auto naturalHeight = static_cast<qreal>(image.height());
    if (naturalWidth <= 0.0 || naturalHeight <= 0.0) {
        return;
    }

    const auto scaleY = box.height() / naturalHeight;
    const auto fitScaleX =
      naturalWidth > box.width() ? box.width() / naturalWidth : 1.0;
    const auto drawnWidth = naturalWidth * scaleY * fitScaleX;
    qreal alignedX = 0.0;
    if (alignment == 1) {
        alignedX = (box.width() - drawnWidth) * 0.5;
    } else if (alignment == 2) {
        alignedX = box.width() - drawnWidth;
    }

    painter->drawImage(QRectF{ box.x() + alignedX,
                               box.y(),
                               drawnWidth,
                               box.height() },
                       image);
}

void
drawSystemText(QPainter* painter,
               const QString& family,
               const QString& text,
               const QColor& color,
               int alignment,
               int fontSize,
               int fontThickness,
               const QRectF& box)
{
    if (text.isEmpty() || box.width() <= 0.0 || box.height() <= 0.0) {
        return;
    }

    QFont font{ family };
    font.setPixelSize(std::max(1, fontSize > 0
                                    ? fontSize
                                    : static_cast<int>(std::round(box.height()))));
    if (fontThickness >= 6) {
        font.setWeight(QFont::Bold);
    } else if (fontThickness >= 4) {
        font.setWeight(QFont::DemiBold);
    }

    QFontMetricsF metrics{ font };
    const auto naturalWidth = metrics.horizontalAdvance(text);
    if (naturalWidth <= 0.0) {
        return;
    }

    const auto scaleY = box.height() / std::max<qreal>(1.0, font.pixelSize());
    const auto fitScaleX =
      naturalWidth > box.width() ? box.width() / naturalWidth : 1.0;
    const auto scaleX = scaleY * fitScaleX;
    const auto drawnWidth = naturalWidth * scaleX;
    qreal alignedX = 0.0;
    if (alignment == 1) {
        alignedX = (box.width() - drawnWidth) * 0.5;
    } else if (alignment == 2) {
        alignedX = box.width() - drawnWidth;
    }

    painter->save();
    painter->setFont(font);
    painter->setPen(color);
    painter->translate(box.x() + alignedX, box.y());
    painter->scale(scaleX, scaleY);
    painter->drawText(QPointF{ 0.0, metrics.ascent() }, text);
    painter->restore();
}

} // namespace

Lr2NativeBarText::Lr2NativeBarText(QQuickItem* parent)
  : QQuickPaintedItem(parent)
{
    configurePaintedItem(*this);
}

auto
Lr2NativeBarText::dstState() const -> QVariant
{
    return m_dstState;
}

void
Lr2NativeBarText::setDstState(const QVariant& state)
{
    m_dstState = state;
    emit dstStateChanged();
    update();
}

auto
Lr2NativeBarText::srcData() const -> QVariant
{
    return m_srcData;
}

void
Lr2NativeBarText::setSrcData(const QVariant& data)
{
    m_srcData = data;
    emit srcDataChanged();
    update();
}

auto
Lr2NativeBarText::selectContext() const -> QObject*
{
    return m_selectContext;
}

void
Lr2NativeBarText::setSelectContext(QObject* context)
{
    if (m_selectContext == context) {
        return;
    }
    m_selectContext = context;
    emit selectContextChanged();
    update();
}

auto
Lr2NativeBarText::barBaseStates() const -> QVariantList
{
    return m_barBaseStates;
}

void
Lr2NativeBarText::setBarBaseStates(const QVariantList& states)
{
    m_barBaseStates = states;
    emit barBaseStatesChanged();
    update();
}

auto
Lr2NativeBarText::barScrollOffset() const -> qreal
{
    return m_barScrollOffset;
}

void
Lr2NativeBarText::setBarScrollOffset(qreal offset)
{
    if (qFuzzyCompare(m_barScrollOffset, offset)) {
        return;
    }
    m_barScrollOffset = offset;
    emit barScrollOffsetChanged();
    update();
}

auto
Lr2NativeBarText::barCenter() const -> int
{
    return m_barCenter;
}

void
Lr2NativeBarText::setBarCenter(int center)
{
    if (m_barCenter == center) {
        return;
    }
    m_barCenter = center;
    emit barCenterChanged();
    update();
}

auto
Lr2NativeBarText::scaleOverride() const -> qreal
{
    return m_scaleOverride;
}

void
Lr2NativeBarText::setScaleOverride(qreal scale)
{
    if (qFuzzyCompare(m_scaleOverride, scale)) {
        return;
    }
    m_scaleOverride = scale;
    emit scaleOverrideChanged();
    update();
}

auto
Lr2NativeBarText::revision() const -> int
{
    return m_revision;
}

void
Lr2NativeBarText::setRevision(int revision)
{
    if (m_revision == revision) {
        return;
    }
    m_revision = revision;
    emit revisionChanged();
    update();
}

auto
Lr2NativeBarText::visualBaseIndex() const -> int
{
    return m_visualBaseIndex;
}

void
Lr2NativeBarText::setVisualBaseIndex(int index)
{
    if (m_visualBaseIndex == index) {
        return;
    }
    m_visualBaseIndex = index;
    emit visualBaseIndexChanged();
    update();
}

void
Lr2NativeBarText::paint(QPainter* painter)
{
    const auto src = barTextFromVariant(m_srcData);
    const auto dst = stateFromVariant(m_dstState);
    if (!dst.valid || !m_selectContext || m_barBaseStates.isEmpty()) {
        return;
    }

    painter->setRenderHint(QPainter::Antialiasing, false);
    painter->setRenderHint(QPainter::SmoothPixmapTransform, false);

    for (int row = 1; row < m_barBaseStates.size(); ++row) {
        const auto visibleState = stateFromVariant(m_barBaseStates[row]);
        if (!visibleState.valid) {
            continue;
        }

        const auto entry = barEntry(m_selectContext, row, m_barCenter);
        if (!entry.isValid() ||
            invoke(m_selectContext, "entryTitleType", entry).toInt() !=
              src.titleType) {
            continue;
        }

        const auto text =
          invoke(m_selectContext, "entryDisplayName", entry, true).toString();
        if (text.isEmpty()) {
            continue;
        }

        const auto rowState =
          interpolatedRowState(m_barBaseStates, row, m_barScrollOffset);
        if (!rowState.valid) {
            continue;
        }

        const auto renderWidth =
          std::min<qreal>(std::abs(dst.w * m_scaleOverride), 32760.0);
        const auto renderHeight =
          std::min<qreal>(std::abs(dst.h * m_scaleOverride), 32760.0);
        if (renderWidth <= 0.0 || renderHeight <= 0.0) {
            continue;
        }

        qreal anchorOffsetX = 0.0;
        if (src.align == 1) {
            anchorOffsetX = -renderWidth * 0.5;
        } else if (src.align == 2) {
            anchorOffsetX = -renderWidth;
        }

        const QRectF box{ (dst.x + rowState.x) * m_scaleOverride +
                            anchorOffsetX,
                          (dst.y + rowState.y) * m_scaleOverride,
                          renderWidth,
                          renderHeight };

        painter->save();
        painter->setOpacity(std::clamp(dst.a, 0, 255) / 255.0);
        if (src.bitmapFont ||
            src.fontPath.toLower().endsWith(QStringLiteral(".lr2font"))) {
            drawBitmapFontText(painter, src.fontPath, text, src.align, box);
        } else {
            drawSystemText(painter,
                           src.fontFamily.isEmpty() ? src.fontPath
                                                    : src.fontFamily,
                           text,
                           solidColorForState(dst),
                           src.align,
                           src.fontSize,
                           src.fontThickness,
                           box);
        }
        painter->restore();
    }
}

Lr2NativeBarNumber::Lr2NativeBarNumber(QQuickItem* parent)
  : QQuickPaintedItem(parent)
{
    configurePaintedItem(*this);
}

auto
Lr2NativeBarNumber::dstState() const -> QVariant
{
    return m_dstState;
}

void
Lr2NativeBarNumber::setDstState(const QVariant& state)
{
    m_dstState = state;
    emit dstStateChanged();
    update();
}

auto
Lr2NativeBarNumber::srcData() const -> QVariant
{
    return m_srcData;
}

void
Lr2NativeBarNumber::setSrcData(const QVariant& data)
{
    m_srcData = data;
    emit srcDataChanged();
    update();
}

auto
Lr2NativeBarNumber::selectContext() const -> QObject*
{
    return m_selectContext;
}

void
Lr2NativeBarNumber::setSelectContext(QObject* context)
{
    if (m_selectContext == context) {
        return;
    }
    m_selectContext = context;
    emit selectContextChanged();
    update();
}

auto
Lr2NativeBarNumber::barBaseStates() const -> QVariantList
{
    return m_barBaseStates;
}

void
Lr2NativeBarNumber::setBarBaseStates(const QVariantList& states)
{
    m_barBaseStates = states;
    emit barBaseStatesChanged();
    update();
}

auto
Lr2NativeBarNumber::barScrollOffset() const -> qreal
{
    return m_barScrollOffset;
}

void
Lr2NativeBarNumber::setBarScrollOffset(qreal offset)
{
    if (qFuzzyCompare(m_barScrollOffset, offset)) {
        return;
    }
    m_barScrollOffset = offset;
    emit barScrollOffsetChanged();
    update();
}

auto
Lr2NativeBarNumber::barCenter() const -> int
{
    return m_barCenter;
}

void
Lr2NativeBarNumber::setBarCenter(int center)
{
    if (m_barCenter == center) {
        return;
    }
    m_barCenter = center;
    emit barCenterChanged();
    update();
}

auto
Lr2NativeBarNumber::scaleOverride() const -> qreal
{
    return m_scaleOverride;
}

void
Lr2NativeBarNumber::setScaleOverride(qreal scale)
{
    if (qFuzzyCompare(m_scaleOverride, scale)) {
        return;
    }
    m_scaleOverride = scale;
    emit scaleOverrideChanged();
    update();
}

auto
Lr2NativeBarNumber::revision() const -> int
{
    return m_revision;
}

void
Lr2NativeBarNumber::setRevision(int revision)
{
    if (m_revision == revision) {
        return;
    }
    m_revision = revision;
    emit revisionChanged();
    update();
}

auto
Lr2NativeBarNumber::visualBaseIndex() const -> int
{
    return m_visualBaseIndex;
}

void
Lr2NativeBarNumber::setVisualBaseIndex(int index)
{
    if (m_visualBaseIndex == index) {
        return;
    }
    m_visualBaseIndex = index;
    emit visualBaseIndexChanged();
    update();
}

void
Lr2NativeBarNumber::paint(QPainter* painter)
{
    const auto src = barNumberFromVariant(m_srcData);
    const auto numberSrc = numberFromVariant(src.source);
    const auto dst = stateFromVariant(m_dstState);
    if (!dst.valid || !m_selectContext || m_barBaseStates.isEmpty() ||
        numberSrc.source.isEmpty()) {
        return;
    }

    const auto sheet = cachedImage(numberSrc.source);
    if (sheet.isNull()) {
        return;
    }

    painter->setRenderHint(QPainter::Antialiasing, false);
    painter->setRenderHint(QPainter::SmoothPixmapTransform, false);

    const auto divX = std::max(1, numberSrc.div_x);
    const auto divY = std::max(1, numberSrc.div_y);
    const auto cellW =
      std::floor(numberSrc.w / static_cast<qreal>(divX));
    const auto cellH =
      std::floor(numberSrc.h / static_cast<qreal>(divY));
    if (cellW <= 0.0 || cellH <= 0.0 || dst.w <= 0.0 || dst.h <= 0.0) {
        return;
    }

    for (int row = 1; row < m_barBaseStates.size(); ++row) {
        const auto visibleState = stateFromVariant(m_barBaseStates[row]);
        if (!visibleState.valid) {
            continue;
        }

        const auto entry = barEntry(m_selectContext, row, m_barCenter);
        if (!entry.isValid()) {
            continue;
        }

        const auto ranking =
          invoke(m_selectContext, "isRankingEntry", entry).toBool();
        const auto chart = invoke(m_selectContext, "isChart", entry).toBool();
        const auto entryLike =
          invoke(m_selectContext, "isEntry", entry).toBool();
        if (!ranking && !chart && !entryLike) {
            continue;
        }

        const auto playLevel =
          invoke(m_selectContext, "entryPlayLevel", entry).toInt();
        const auto difficulty =
          invoke(m_selectContext, "entryDifficulty", entry).toInt();
        const auto variant =
          ranking ? 6 : (difficulty <= 0 ? 0 : difficulty);
        if (src.variant != variant) {
            continue;
        }

        auto text = QString::number(playLevel);
        if (numberSrc.keta > 0 && numberSrc.align == 2) {
            while (text.size() < numberSrc.keta) {
                text.prepend(QLatin1Char('0'));
            }
        }

        const auto rowState =
          interpolatedRowState(m_barBaseStates, row, m_barScrollOffset);
        if (!rowState.valid) {
            continue;
        }

        const auto digitW = dst.w * m_scaleOverride;
        const auto digitH = dst.h * m_scaleOverride;
        const auto textW = digitW * text.size();
        qreal alignOffset = 0.0;
        if (numberSrc.align == 1) {
            alignOffset = std::max<qreal>(0.0, dst.w * m_scaleOverride - textW);
        } else if (numberSrc.align == 2) {
            alignOffset =
              std::max<qreal>(0.0, (dst.w * m_scaleOverride - textW) * 0.5);
        }

        painter->save();
        painter->setOpacity(std::clamp(dst.a, 0, 255) / 255.0);
        const auto baseX =
          (dst.x + rowState.x) * m_scaleOverride + alignOffset;
        const auto baseY = (dst.y + rowState.y) * m_scaleOverride;

        for (int i = 0; i < text.size(); ++i) {
            bool ok = false;
            auto digit = QStringView{ text }.mid(i, 1).toInt(&ok);
            if (!ok) {
                digit = 0;
            }
            digit = std::clamp(digit, 0, divX * divY - 1);
            const auto col = digit % divX;
            const auto sourceRow = digit / divX;
            const QRectF sourceRect{ numberSrc.x + col * cellW,
                                     numberSrc.y + sourceRow * cellH,
                                     cellW,
                                     cellH };
            const QRectF targetRect{ baseX + i * digitW,
                                     baseY,
                                     digitW,
                                     digitH };
            painter->drawImage(targetRect, sheet, sourceRect);
        }
        painter->restore();
    }
}

} // namespace qml_components
