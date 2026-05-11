#include "Lr2BarDistributionGraphItem.h"

#include "Lr2SelectBarCell.h"

#include <QFileInfo>
#include <QImageReader>
#include <QJSValue>
#include <QMatrix4x4>
#include <QQuickWindow>
#include <QSGGeometryNode>
#include <QSGMaterial>
#include <QSGTexture>
#include <QtMath>
#include <QUrl>
#include <QVariantMap>
#include <algorithm>
#include <cstring>
#include <cmath>
#include <vector>

namespace {

constexpr int uniformSize = 112;

struct GraphVertex {
    float x;
    float y;
    float u;
    float v;
    float localU;
    float localV;
    float r;
    float g;
    float b;
    float a;
    float blend;
    float colorKey;
    float nearest;
    float flagsPad;
    float sourceX;
    float sourceY;
    float sourceW;
    float sourceH;
};

struct GraphState {
    bool valid = false;
    qreal x = 0.0;
    qreal y = 0.0;
    qreal w = 0.0;
    qreal h = 0.0;
    qreal a = 255.0;
    qreal r = 255.0;
    qreal g = 255.0;
    qreal b = 255.0;
    qreal angle = 0.0;
    int center = 0;
    int blend = 0;
    int filter = 0;
};

const QSGGeometry::AttributeSet& graphAttributes() {
    static const QSGGeometry::Attribute attributes[] = {
        QSGGeometry::Attribute::create(0, 2, QSGGeometry::FloatType, true),
        QSGGeometry::Attribute::create(1, 2, QSGGeometry::FloatType),
        QSGGeometry::Attribute::create(2, 2, QSGGeometry::FloatType),
        QSGGeometry::Attribute::create(3, 4, QSGGeometry::FloatType),
        QSGGeometry::Attribute::create(4, 4, QSGGeometry::FloatType),
        QSGGeometry::Attribute::create(5, 4, QSGGeometry::FloatType),
    };
    static const QSGGeometry::AttributeSet set = {
        6,
        sizeof(GraphVertex),
        attributes,
    };
    return set;
}

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
    return {};
}

QString resolvedImageSource(const QVariant& srcData) {
    const QString rawSource = valueProperty(srcData, "source").toString();
    if (rawSource.isEmpty()) {
        return {};
    }
    QString path = rawSource;
    path.replace(u'\\', u'/');
    if (path.size() >= 3 && path[1] == u':' && path[2] == u'/' && path[0].isLetter()) {
        return QStringLiteral("file:///") + path;
    }
    if (path.startsWith(u'/')) {
        return QStringLiteral("file://") + path;
    }
    return path;
}

QString localPathForSource(const QString& source) {
    if (source.isEmpty()) {
        return {};
    }
    const QUrl url(source);
    if (url.isLocalFile()) {
        return url.toLocalFile();
    }
    if (url.scheme() == QStringLiteral("qrc")) {
        return QStringLiteral(":") + url.path();
    }
    if (source.startsWith(QStringLiteral(":/"))) {
        return source;
    }
    return source;
}

GraphState stateFromVariant(const QVariant& value) {
    GraphState state;
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
    state.angle = toReal(valueProperty(value, "angle"));
    state.center = toInt(valueProperty(value, "center"));
    state.blend = toInt(valueProperty(value, "blend"));
    state.filter = toInt(valueProperty(value, "filter"));
    return state;
}

QPointF centerAnchor(int center) {
    switch (center) {
    case 1:
        return {0.0, 1.0};
    case 2:
        return {0.5, 1.0};
    case 3:
        return {1.0, 1.0};
    case 4:
        return {0.0, 0.5};
    case 6:
        return {1.0, 0.5};
    case 7:
        return {0.0, 0.0};
    case 8:
        return {0.5, 0.0};
    case 9:
        return {1.0, 0.0};
    default:
        return {0.5, 0.5};
    }
}

qreal graphValueAt(const Lr2SelectBarCell* cell, int graphType, int segment) {
    return cell ? cell->graphValueForType(graphType, segment) : 0.0;
}

int normalizedBlendMode(int rawBlend, bool colorKeyEnabled) {
    if (rawBlend == 0 && !colorKeyEnabled) {
        return 1;
    }
    if (rawBlend == 5 || rawBlend == 6) {
        return 2;
    }
    if (rawBlend == 3 || rawBlend == 4 || rawBlend == 9 || rawBlend == 11) {
        return 1;
    }
    return rawBlend;
}

float normalizedColor(qreal value) {
    return static_cast<float>(std::clamp(value, 0.0, 255.0) / 255.0);
}

QPointF rotatedPoint(const QPointF& point, const QPointF& origin, qreal degrees) {
    if (std::abs(degrees) <= 0.0001) {
        return point;
    }
    const qreal radians = qDegreesToRadians(degrees);
    const qreal s = std::sin(radians);
    const qreal c = std::cos(radians);
    const qreal dx = point.x() - origin.x();
    const qreal dy = point.y() - origin.y();
    return {
        origin.x() + dx * c - dy * s,
        origin.y() + dx * s + dy * c,
    };
}

class DistributionGraphMaterial;

class DistributionGraphShader : public QSGMaterialShader {
public:
    DistributionGraphShader() {
        setShaderFileName(VertexStage, QStringLiteral(":/Lr2BarDistributionGraph.vert.qsb"));
        setShaderFileName(FragmentStage, QStringLiteral(":/Lr2BarDistributionGraph.frag.qsb"));
    }

    bool updateUniformData(RenderState& state,
                           QSGMaterial* newMaterial,
                           QSGMaterial* oldMaterial) override;
    void updateSampledImage(RenderState& state,
                            int binding,
                            QSGTexture** texture,
                            QSGMaterial* newMaterial,
                            QSGMaterial* oldMaterial) override;
};

class DistributionGraphMaterial : public QSGMaterial {
public:
    DistributionGraphMaterial() {
        setFlag(QSGMaterial::Blending, true);
    }

    ~DistributionGraphMaterial() override {
        delete m_texture;
    }

    QSGMaterialType* type() const override {
        static QSGMaterialType type;
        return &type;
    }

    QSGMaterialShader* createShader(QSGRendererInterface::RenderMode) const override {
        return new DistributionGraphShader;
    }

    int compare(const QSGMaterial* other) const override {
        const auto* material = static_cast<const DistributionGraphMaterial*>(other);
        if (m_texture == material->m_texture) {
            return 0;
        }
        return m_texture < material->m_texture ? -1 : 1;
    }

    void setTexture(QSGTexture* texture) {
        if (m_texture == texture) {
            return;
        }
        delete m_texture;
        m_texture = texture;
    }

    QSGTexture* texture() const {
        return m_texture;
    }

    QColor transColor = Qt::black;
    QSize textureSize;
    float tolerance = 0.03125f;

private:
    QSGTexture* m_texture = nullptr;
};

bool DistributionGraphShader::updateUniformData(RenderState& state,
                                                QSGMaterial* newMaterial,
                                                QSGMaterial*) {
    const auto* material = static_cast<DistributionGraphMaterial*>(newMaterial);
    QByteArray* buffer = state.uniformData();
    if (buffer->size() != uniformSize) {
        buffer->resize(uniformSize);
    }
    std::fill(buffer->begin(), buffer->end(), char(0));

    const QMatrix4x4 matrix = state.combinedMatrix();
    std::memcpy(buffer->data(), matrix.constData(), 64);

    const float opacity = state.opacity();
    std::memcpy(buffer->data() + 64, &opacity, sizeof(float));

    const float transColor[4] = {
        static_cast<float>(material->transColor.redF()),
        static_cast<float>(material->transColor.greenF()),
        static_cast<float>(material->transColor.blueF()),
        static_cast<float>(material->transColor.alphaF()),
    };
    std::memcpy(buffer->data() + 80, transColor, sizeof(transColor));

    const float textureSize[2] = {
        static_cast<float>(std::max(1, material->textureSize.width())),
        static_cast<float>(std::max(1, material->textureSize.height())),
    };
    std::memcpy(buffer->data() + 96, textureSize, sizeof(textureSize));
    std::memcpy(buffer->data() + 104, &material->tolerance, sizeof(float));
    return true;
}

void DistributionGraphShader::updateSampledImage(RenderState&,
                                                 int binding,
                                                 QSGTexture** texture,
                                                 QSGMaterial* newMaterial,
                                                 QSGMaterial*) {
    if (binding != 1) {
        return;
    }
    auto* material = static_cast<DistributionGraphMaterial*>(newMaterial);
    *texture = material->texture();
}

} // namespace

Lr2BarDistributionGraphItem::Lr2BarDistributionGraphItem(QQuickItem* parent)
    : QQuickItem(parent) {
    setFlag(ItemHasContents, true);
}

Lr2BarDistributionGraphItem::~Lr2BarDistributionGraphItem() = default;

QVariant Lr2BarDistributionGraphItem::srcData() const {
    return m_srcData;
}

void Lr2BarDistributionGraphItem::setSrcData(const QVariant& value) {
    if (m_srcData == value) {
        return;
    }
    m_srcData = value;
    parseSource();
    emit srcDataChanged();
    requestSceneUpdate();
}

QVariant Lr2BarDistributionGraphItem::stateData() const {
    return m_stateData;
}

void Lr2BarDistributionGraphItem::setStateData(const QVariant& value) {
    if (m_stateData == value) {
        return;
    }
    m_stateData = value;
    emit stateDataChanged();
    requestSceneUpdate();
}

QVariantList Lr2BarDistributionGraphItem::barCells() const {
    return m_barCellsValue;
}

void Lr2BarDistributionGraphItem::setBarCells(const QVariantList& value) {
    if (m_barCellsValue == value) {
        return;
    }
    m_barCellsValue = value;
    reconnectCells();
    emit barCellsChanged();
    requestSceneUpdate();
}

Lr2BarPositionCache* Lr2BarDistributionGraphItem::barPositionCache() const {
    return m_barPositionCache;
}

void Lr2BarDistributionGraphItem::setBarPositionCache(Lr2BarPositionCache* value) {
    if (m_barPositionCache == value) {
        return;
    }
    if (m_positionRevisionConnection) {
        disconnect(m_positionRevisionConnection);
    }
    if (m_positionSlotOffsetConnection) {
        disconnect(m_positionSlotOffsetConnection);
    }
    if (m_positionSlotCountConnection) {
        disconnect(m_positionSlotCountConnection);
    }

    m_barPositionCache = value;
    if (m_barPositionCache) {
        m_positionRevisionConnection = connect(
            m_barPositionCache,
            &Lr2BarPositionCache::revisionChanged,
            this,
            &Lr2BarDistributionGraphItem::requestSceneUpdate);
        m_positionSlotOffsetConnection = connect(
            m_barPositionCache,
            &Lr2BarPositionCache::slotOffsetChanged,
            this,
            &Lr2BarDistributionGraphItem::requestSceneUpdate);
        m_positionSlotCountConnection = connect(
            m_barPositionCache,
            &Lr2BarPositionCache::slotCountChanged,
            this,
            &Lr2BarDistributionGraphItem::requestSceneUpdate);
    } else {
        m_positionRevisionConnection = {};
        m_positionSlotOffsetConnection = {};
        m_positionSlotCountConnection = {};
    }

    emit barPositionCacheChanged();
    requestSceneUpdate();
}

qreal Lr2BarDistributionGraphItem::scaleOverride() const {
    return m_scaleOverride;
}

void Lr2BarDistributionGraphItem::setScaleOverride(qreal value) {
    if (!std::isfinite(value)) {
        value = 1.0;
    }
    if (std::abs(m_scaleOverride - value) <= 0.000001) {
        return;
    }
    m_scaleOverride = value;
    emit scaleOverrideChanged();
    requestSceneUpdate();
}

int Lr2BarDistributionGraphItem::frameOverrideBase() const {
    return m_frameOverrideBase;
}

void Lr2BarDistributionGraphItem::setFrameOverrideBase(int value) {
    if (m_frameOverrideBase == value) {
        return;
    }
    m_frameOverrideBase = value;
    emit frameOverrideBaseChanged();
    requestSceneUpdate();
}

QColor Lr2BarDistributionGraphItem::transColor() const {
    return m_transColor;
}

void Lr2BarDistributionGraphItem::setTransColor(const QColor& value) {
    if (m_transColor == value) {
        return;
    }
    m_transColor = value;
    emit transColorChanged();
    requestSceneUpdate();
}

bool Lr2BarDistributionGraphItem::colorKeyEnabled() const {
    return m_colorKeyEnabled;
}

void Lr2BarDistributionGraphItem::setColorKeyEnabled(bool value) {
    if (m_colorKeyEnabled == value) {
        return;
    }
    m_colorKeyEnabled = value;
    emit colorKeyEnabledChanged();
    requestSceneUpdate();
}

void Lr2BarDistributionGraphItem::parseSource() {
    m_source = {};
    if (!m_srcData.isValid() || m_srcData.isNull()) {
        loadSourceImage();
        return;
    }

    m_source.graphType = toInt(valueProperty(m_srcData, "graphType"));
    m_source.specialType = toInt(valueProperty(m_srcData, "specialType"));
    m_source.x = toReal(valueProperty(m_srcData, "x"));
    m_source.y = toReal(valueProperty(m_srcData, "y"));
    m_source.w = toReal(valueProperty(m_srcData, "w"));
    m_source.h = toReal(valueProperty(m_srcData, "h"));
    m_source.divX = std::max(1, toInt(valueProperty(m_srcData, "div_x"), 1));
    m_source.divY = std::max(1, toInt(valueProperty(m_srcData, "div_y"), 1));
    m_source.source = resolvedImageSource(m_srcData);
    loadSourceImage();
}

void Lr2BarDistributionGraphItem::loadSourceImage() {
    m_sourceImage = {};
    m_textureDirty = true;

    if (m_source.specialType == 2) {
        m_sourceImage = QImage(1, 1, QImage::Format_RGBA8888);
        m_sourceImage.fill(Qt::black);
        return;
    }

    if (m_source.source.isEmpty()) {
        return;
    }

    const QString path = localPathForSource(m_source.source);
    if (path.isEmpty()) {
        return;
    }

    QImageReader reader(path);
    reader.setAutoTransform(true);
    m_sourceImage = reader.read();
}

void Lr2BarDistributionGraphItem::reconnectCells() {
    for (const auto& connection : m_cellConnections) {
        disconnect(connection);
    }
    m_cellConnections.clear();
    m_barCells.clear();
    m_barCells.reserve(m_barCellsValue.size());

    for (const QVariant& value : m_barCellsValue) {
        auto* cell = qobject_cast<Lr2SelectBarCell*>(value.value<QObject*>());
        m_barCells.append(cell);
        if (!cell) {
            continue;
        }
        m_cellConnections.append(connect(
            cell,
            &Lr2SelectBarCell::revisionChanged,
            this,
            &Lr2BarDistributionGraphItem::requestSceneUpdate));
        m_cellConnections.append(connect(
            cell,
            &QObject::destroyed,
            this,
            &Lr2BarDistributionGraphItem::requestSceneUpdate));
    }
}

void Lr2BarDistributionGraphItem::requestSceneUpdate() {
    update();
}

QSGNode* Lr2BarDistributionGraphItem::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*) {
    const GraphState state = stateFromVariant(m_stateData);
    const bool hasTexture = !m_sourceImage.isNull();
    if (!state.valid || state.a <= 0.0 || m_barCells.isEmpty() || !m_barPositionCache || !hasTexture) {
        delete oldNode;
        return nullptr;
    }

    auto* node = static_cast<QSGGeometryNode*>(oldNode);
    if (!node) {
        node = new QSGGeometryNode;
        node->setGeometry(new QSGGeometry(graphAttributes(), 0, 0, QSGGeometry::UnsignedIntType));
        node->setFlag(QSGNode::OwnsGeometry, true);
        node->setMaterial(new DistributionGraphMaterial);
        node->setFlag(QSGNode::OwnsMaterial, true);
    }

    auto* material = static_cast<DistributionGraphMaterial*>(node->material());
    if (m_textureDirty || !material->texture()) {
        material->setTexture(window()->createTextureFromImage(m_sourceImage));
        if (material->texture()) {
            material->texture()->setFiltering(QSGTexture::Linear);
            material->texture()->setHorizontalWrapMode(QSGTexture::ClampToEdge);
            material->texture()->setVerticalWrapMode(QSGTexture::ClampToEdge);
        }
        m_textureDirty = false;
    }
    if (!material->texture()) {
        delete node;
        return nullptr;
    }
    material->transColor = m_transColor;
    material->textureSize = m_sourceImage.size();

    const int segmentCount = m_source.graphType == 0 ? 11 : 28;
    const int frameCount = std::max(1, m_source.divX * m_source.divY);
    const qreal fullW = std::abs(state.w);
    const qreal fullH = std::abs(state.h);
    if (segmentCount <= 0 || frameCount <= 0 || fullW <= 0.0 || fullH <= 0.0) {
        delete node;
        return nullptr;
    }

    const qreal imageW = std::max(1, m_sourceImage.width());
    const qreal imageH = std::max(1, m_sourceImage.height());
    qreal sourceX = m_source.x;
    qreal sourceY = m_source.y;
    qreal sourceW = m_source.w;
    qreal sourceH = m_source.h;
    if (sourceW <= 0.0 || sourceH <= 0.0 || sourceX < 0.0 || sourceY < 0.0) {
        sourceX = 0.0;
        sourceY = 0.0;
        sourceW = imageW;
        sourceH = imageH;
    }
    const qreal cellW = sourceW / m_source.divX;
    const qreal cellH = sourceH / m_source.divY;

    const int blendMode = normalizedBlendMode(state.blend, m_colorKeyEnabled);
    const bool colorKey = blendMode == 0;
    const bool nearest = state.filter == 0;
    const QPointF anchor = centerAnchor(state.center);
    const qreal baseX = state.x + std::min<qreal>(0.0, state.w);
    const qreal baseY = state.y + std::min<qreal>(0.0, state.h);
    const float tintR = normalizedColor(state.r);
    const float tintG = normalizedColor(state.g);
    const float tintB = normalizedColor(state.b);
    const float alpha = normalizedColor(state.a);

    std::vector<GraphVertex> vertices;
    std::vector<quint32> indices;
    vertices.reserve(static_cast<size_t>(m_barCells.size() * segmentCount * 4));
    indices.reserve(static_cast<size_t>(m_barCells.size() * segmentCount * 6));

    std::vector<qreal> amounts(static_cast<size_t>(segmentCount), 0.0);
    std::vector<qreal> starts(static_cast<size_t>(segmentCount), 0.0);
    std::vector<qreal> widths(static_cast<size_t>(segmentCount), 0.0);

    for (int slot = 0; slot < m_barCells.size(); ++slot) {
        const auto* cell = m_barCells.at(slot).data();
        if (!cell || !cell->isValid() || !cell->isFolderLike()) {
            continue;
        }

        const int row = m_barPositionCache->rowForSlot(slot);
        if (row <= 0 || row >= m_barPositionCache->count()) {
            continue;
        }

        qreal total = 0.0;
        std::fill(amounts.begin(), amounts.end(), 0.0);
        std::fill(starts.begin(), starts.end(), 0.0);
        std::fill(widths.begin(), widths.end(), 0.0);
        for (int segment = 0; segment < segmentCount; ++segment) {
            const qreal amount = graphValueAt(cell, m_source.graphType, segment);
            amounts[static_cast<size_t>(segment)] = amount;
            total += amount;
        }
        if (total <= 0.0) {
            continue;
        }

        qreal accumulated = 0.0;
        for (int segment = segmentCount - 1; segment >= 0; --segment) {
            const qreal amount = amounts[static_cast<size_t>(segment)];
            if (amount <= 0.0) {
                continue;
            }
            starts[static_cast<size_t>(segment)] = accumulated * fullW / total;
            widths[static_cast<size_t>(segment)] = amount * fullW / total;
            accumulated += amount;
        }

        const qreal rowX = m_barPositionCache->xAt(row);
        const qreal rowY = m_barPositionCache->yAt(row);
        for (int segment = 0; segment < segmentCount; ++segment) {
            const qreal segmentW = widths[static_cast<size_t>(segment)];
            if (segmentW <= 0.0) {
                continue;
            }

            const qreal x = (rowX + baseX + starts[static_cast<size_t>(segment)]) * m_scaleOverride;
            const qreal y = (rowY + baseY) * m_scaleOverride;
            const qreal w = segmentW * m_scaleOverride;
            const qreal h = fullH * m_scaleOverride;
            if (w <= 0.0 || h <= 0.0) {
                continue;
            }

            const int frame = std::clamp(m_frameOverrideBase + std::min(frameCount - 1, segment), 0, frameCount - 1);
            const int frameCol = frame % m_source.divX;
            const int frameRow = frame / m_source.divX;
            const qreal sx = sourceX + frameCol * cellW;
            const qreal sy = sourceY + frameRow * cellH;
            const qreal u0 = sx / imageW;
            const qreal v0 = sy / imageH;
            const qreal u1 = (sx + cellW) / imageW;
            const qreal v1 = (sy + cellH) / imageH;

            const QPointF rotationOrigin(x + w * anchor.x(), y + h * anchor.y());
            const QPointF p0 = rotatedPoint({x, y}, rotationOrigin, state.angle);
            const QPointF p1 = rotatedPoint({x + w, y}, rotationOrigin, state.angle);
            const QPointF p2 = rotatedPoint({x + w, y + h}, rotationOrigin, state.angle);
            const QPointF p3 = rotatedPoint({x, y + h}, rotationOrigin, state.angle);

            const quint32 base = static_cast<quint32>(vertices.size());
            const auto appendVertex = [&](const QPointF& p, float u, float v, float lu, float lv) {
                vertices.push_back({
                    static_cast<float>(p.x()),
                    static_cast<float>(p.y()),
                    u,
                    v,
                    lu,
                    lv,
                    tintR,
                    tintG,
                    tintB,
                    alpha,
                    static_cast<float>(blendMode),
                    colorKey ? 1.0f : 0.0f,
                    nearest ? 1.0f : 0.0f,
                    0.0f,
                    static_cast<float>(sx),
                    static_cast<float>(sy),
                    static_cast<float>(cellW),
                    static_cast<float>(cellH),
                });
            };

            appendVertex(p0, static_cast<float>(u0), static_cast<float>(v0), 0.0f, 0.0f);
            appendVertex(p1, static_cast<float>(u1), static_cast<float>(v0), 1.0f, 0.0f);
            appendVertex(p2, static_cast<float>(u1), static_cast<float>(v1), 1.0f, 1.0f);
            appendVertex(p3, static_cast<float>(u0), static_cast<float>(v1), 0.0f, 1.0f);
            indices.insert(indices.end(), {base, base + 1, base + 2, base, base + 2, base + 3});
        }
    }

    if (vertices.empty()) {
        delete node;
        return nullptr;
    }

    QSGGeometry* geometry = node->geometry();
    geometry->allocate(static_cast<int>(vertices.size()), static_cast<int>(indices.size()));
    geometry->setDrawingMode(QSGGeometry::DrawTriangles);
    std::memcpy(geometry->vertexData(), vertices.data(), vertices.size() * sizeof(GraphVertex));
    std::memcpy(geometry->indexData(), indices.data(), indices.size() * sizeof(quint32));
    geometry->markVertexDataDirty();
    geometry->markIndexDataDirty();
    node->markDirty(QSGNode::DirtyGeometry | QSGNode::DirtyMaterial);
    return node;
}
