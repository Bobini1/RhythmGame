#include "Lr2BlendSprite.h"

#include <QSGDynamicTexture>
#include <QSGGeometry>
#include <QSGGeometryNode>
#include <QSGMaterial>
#include <QSGMaterialShader>
#include <QSGTexture>
#include <QSGTextureProvider>

#include <array>
#include <cstring>

namespace {

class Lr2BlendMaterial final : public QSGMaterial
{
  public:
    Lr2BlendMaterial() { setFlag(Blending); }

    QSGMaterialType* type() const override
    {
        static QSGMaterialType materialType;
        return &materialType;
    }

    QSGMaterialShader* createShader(
      QSGRendererInterface::RenderMode) const override;

    int compare(const QSGMaterial* otherMaterial) const override
    {
        const auto* other = static_cast<const Lr2BlendMaterial*>(otherMaterial);
        const auto compareValue = [](const auto& lhs, const auto& rhs) {
            return lhs < rhs ? -1 : (rhs < lhs ? 1 : 0);
        };

        if (const auto result = compareValue(blendMode, other->blendMode);
            result != 0) {
            return result;
        }
        if (texture == nullptr || other->texture == nullptr) {
            return compareValue(texture != nullptr, other->texture != nullptr);
        }
        if (const auto result = compareValue(texture->comparisonKey(),
                                             other->texture->comparisonKey());
            result != 0) {
            return result;
        }
        if (const auto result =
              compareValue(tint.rgba64(), other->tint.rgba64());
            result != 0) {
            return result;
        }
        if (const auto result =
              compareValue(transColor.rgba64(), other->transColor.rgba64());
            result != 0) {
            return result;
        }
        if (const auto result =
              compareValue(colorKeyEnabled, other->colorKeyEnabled);
            result != 0) {
            return result;
        }
        return compareValue(smooth, other->smooth);
    }

    QSGTexture* texture = nullptr;
    QColor tint = Qt::white;
    QColor transColor = Qt::black;
    bool colorKeyEnabled = false;
    bool smooth = true;
    int blendMode = 1;
};

class Lr2BlendShader final : public QSGMaterialShader
{
  public:
    Lr2BlendShader()
    {
        setShaderFileName(VertexStage,
                          QStringLiteral(":/Lr2/Lr2BlendSprite.vert.qsb"));
        setShaderFileName(FragmentStage,
                          QStringLiteral(":/Lr2/Lr2BlendSprite.frag.qsb"));
        setFlag(UpdatesGraphicsPipelineState);
    }

    bool updateUniformData(RenderState& state,
                           QSGMaterial* newMaterial,
                           QSGMaterial* oldMaterial) override
    {
        auto* buffer = state.uniformData();
        Q_ASSERT(buffer->size() >= 120);
        auto* material = static_cast<Lr2BlendMaterial*>(newMaterial);
        auto* old = static_cast<Lr2BlendMaterial*>(oldMaterial);
        bool changed = false;

        if (state.isMatrixDirty()) {
            const auto matrix = state.combinedMatrix();
            std::memcpy(buffer->data(), matrix.constData(), 64);
            changed = true;
        }
        if (state.isOpacityDirty()) {
            const float opacity = state.opacity();
            std::memcpy(buffer->data() + 64, &opacity, sizeof(opacity));
            changed = true;
        }

        const auto writeColor = [buffer](qsizetype offset,
                                         const QColor& color) {
            const std::array<float, 4> values{
                color.redF(), color.greenF(), color.blueF(), color.alphaF()
            };
            std::memcpy(buffer->data() + offset, values.data(), sizeof(values));
        };
        if (old == nullptr || material->tint != old->tint) {
            writeColor(80, material->tint);
            changed = true;
        }
        if (old == nullptr || material->transColor != old->transColor) {
            writeColor(96, material->transColor);
            changed = true;
        }
        if (old == nullptr ||
            material->colorKeyEnabled != old->colorKeyEnabled) {
            const float enabled = material->colorKeyEnabled ? 1.0F : 0.0F;
            std::memcpy(buffer->data() + 112, &enabled, sizeof(enabled));
            changed = true;
        }
        if (old == nullptr || material->blendMode != old->blendMode) {
            const float blendMode = static_cast<float>(material->blendMode);
            std::memcpy(buffer->data() + 116, &blendMode, sizeof(blendMode));
            changed = true;
        }
        return changed;
    }

    void updateSampledImage(RenderState& state,
                            int binding,
                            QSGTexture** texture,
                            QSGMaterial* newMaterial,
                            QSGMaterial*) override
    {
        if (binding != 1) {
            return;
        }
        auto* material = static_cast<Lr2BlendMaterial*>(newMaterial);
        *texture = material->texture;
        if (*texture != nullptr) {
            (*texture)->commitTextureOperations(state.rhi(),
                                                state.resourceUpdateBatch());
        }
    }

    bool updateGraphicsPipelineState(RenderState&,
                                     GraphicsPipelineState* pipeline,
                                     QSGMaterial* newMaterial,
                                     QSGMaterial*) override
    {
        const auto* material = static_cast<Lr2BlendMaterial*>(newMaterial);
        pipeline->blendEnable = true;
        pipeline->separateBlendFactors = true;
        pipeline->srcAlpha = GraphicsPipelineState::Zero;
        pipeline->dstAlpha = GraphicsPipelineState::One;
        pipeline->opAlpha = GraphicsPipelineState::BlendOp::Add;

        switch (material->blendMode) {
            case 3: // DX_BLENDMODE_SUB (implemented by DxLib as reverse
                    // subtract)
                pipeline->srcColor = GraphicsPipelineState::SrcAlpha;
                pipeline->dstColor = GraphicsPipelineState::One;
                pipeline->opColor =
                  GraphicsPipelineState::BlendOp::ReverseSubtract;
                break;
            case 4: // DX_BLENDMODE_MUL
                pipeline->srcColor = GraphicsPipelineState::Zero;
                pipeline->dstColor = GraphicsPipelineState::SrcColor;
                pipeline->opColor = GraphicsPipelineState::BlendOp::Add;
                break;
            case 5: // DX_BLENDMODE_SUB2, DxLib's additive helper pass
                pipeline->srcColor = GraphicsPipelineState::SrcAlpha;
                pipeline->dstColor = GraphicsPipelineState::One;
                pipeline->opColor = GraphicsPipelineState::BlendOp::Add;
                break;
            case 9: // DX_BLENDMODE_INVDESTCOLOR
                pipeline->srcColor = GraphicsPipelineState::OneMinusDstColor;
                pipeline->dstColor = GraphicsPipelineState::Zero;
                pipeline->opColor = GraphicsPipelineState::BlendOp::Add;
                break;
            default:
                pipeline->srcColor = GraphicsPipelineState::SrcAlpha;
                pipeline->dstColor = GraphicsPipelineState::OneMinusSrcAlpha;
                pipeline->opColor = GraphicsPipelineState::BlendOp::Add;
                break;
        }
        return true;
    }
};

QSGMaterialShader*
Lr2BlendMaterial::createShader(QSGRendererInterface::RenderMode) const
{
    return new Lr2BlendShader;
}

class Lr2BlendNode final
  : public QObject
  , public QSGNode
{
  public:
    explicit Lr2BlendNode(QSGTextureProvider* provider)
      : m_provider(provider)
    {
        setFlag(UsePreprocess, true);
        m_material = new Lr2BlendMaterial;
        m_geometryNode.setMaterial(m_material);
        m_geometryNode.setFlag(QSGNode::OwnsMaterial);
        m_geometryNode.setGeometry(
          new QSGGeometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 4));
        m_geometryNode.setFlag(QSGNode::OwnsGeometry);
        connect(
          m_provider.data(),
          &QSGTextureProvider::textureChanged,
          this,
          [this]() { m_geometryNode.markDirty(QSGNode::DirtyMaterial); },
          Qt::DirectConnection);
    }

    void setState(const QRectF& targetRect,
                  const QRectF& sourceRect,
                  const QColor& tint,
                  const QColor& transColor,
                  bool colorKeyEnabled,
                  bool smooth,
                  int blendMode)
    {
        if (m_targetRect != targetRect || m_sourceRect != sourceRect) {
            m_targetRect = targetRect;
            m_sourceRect = sourceRect;
            m_geometryDirty = true;
        }

        if (m_material->tint != tint || m_material->transColor != transColor ||
            m_material->colorKeyEnabled != colorKeyEnabled ||
            m_material->smooth != smooth ||
            m_material->blendMode != blendMode) {
            m_material->tint = tint;
            m_material->transColor = transColor;
            m_material->colorKeyEnabled = colorKeyEnabled;
            m_material->smooth = smooth;
            m_material->blendMode = blendMode;
            m_geometryNode.markDirty(QSGNode::DirtyMaterial);
        }
    }

    void preprocess() override
    {
        auto* texture = m_provider ? m_provider->texture() : nullptr;
        if (auto* dynamicTexture = qobject_cast<QSGDynamicTexture*>(texture)) {
            dynamicTexture->updateTexture();
        }

        if (texture == nullptr) {
            if (m_geometryNode.parent() != nullptr) {
                removeChildNode(&m_geometryNode);
            }
            m_material->texture = nullptr;
            return;
        }

        texture->setFiltering(m_material->smooth ? QSGTexture::Linear
                                                 : QSGTexture::Nearest);
        texture->setMipmapFiltering(QSGTexture::None);
        texture->setHorizontalWrapMode(QSGTexture::ClampToEdge);
        texture->setVerticalWrapMode(QSGTexture::ClampToEdge);

        if (m_material->texture != texture) {
            m_material->texture = texture;
            m_geometryDirty = true;
            m_geometryNode.markDirty(QSGNode::DirtyMaterial);
        }
        if (m_geometryDirty) {
            const auto textureRect =
              texture->convertToNormalizedSourceRect(m_sourceRect);
            QSGGeometry::updateTexturedRectGeometry(
              m_geometryNode.geometry(), m_targetRect, textureRect);
            m_geometryNode.markDirty(QSGNode::DirtyGeometry);
            m_geometryDirty = false;
        }
        if (m_geometryNode.parent() == nullptr) {
            appendChildNode(&m_geometryNode);
        }
    }

  private:
    QPointer<QSGTextureProvider> m_provider;
    Lr2BlendMaterial* m_material = nullptr;
    QSGGeometryNode m_geometryNode;
    QRectF m_targetRect;
    QRectF m_sourceRect;
    bool m_geometryDirty = true;
};

} // namespace

Lr2BlendSprite::Lr2BlendSprite(QQuickItem* parent)
  : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
}

QQuickItem*
Lr2BlendSprite::source() const
{
    return m_source;
}

void
Lr2BlendSprite::setSource(QQuickItem* source)
{
    if (m_source == source) {
        return;
    }
    m_source = source;
    m_sourceChanged = true;
    emit sourceChanged();
    update();
}

QRectF
Lr2BlendSprite::sourceRect() const
{
    return m_sourceRect;
}

void
Lr2BlendSprite::setSourceRect(const QRectF& sourceRect)
{
    if (m_sourceRect == sourceRect) {
        return;
    }
    m_sourceRect = sourceRect;
    emit sourceRectChanged();
    update();
}

QColor
Lr2BlendSprite::tint() const
{
    return m_tint;
}

void
Lr2BlendSprite::setTint(const QColor& tint)
{
    if (m_tint == tint) {
        return;
    }
    m_tint = tint;
    emit tintChanged();
    update();
}

QColor
Lr2BlendSprite::transColor() const
{
    return m_transColor;
}

void
Lr2BlendSprite::setTransColor(const QColor& transColor)
{
    if (m_transColor == transColor) {
        return;
    }
    m_transColor = transColor;
    emit transColorChanged();
    update();
}

bool
Lr2BlendSprite::colorKeyEnabled() const
{
    return m_colorKeyEnabled;
}

void
Lr2BlendSprite::setColorKeyEnabled(bool enabled)
{
    if (m_colorKeyEnabled == enabled) {
        return;
    }
    m_colorKeyEnabled = enabled;
    emit colorKeyEnabledChanged();
    update();
}

bool
Lr2BlendSprite::smooth() const
{
    return m_smooth;
}

void
Lr2BlendSprite::setSmooth(bool smooth)
{
    if (m_smooth == smooth) {
        return;
    }
    m_smooth = smooth;
    emit smoothChanged();
    update();
}

int
Lr2BlendSprite::blendMode() const
{
    return m_blendMode;
}

void
Lr2BlendSprite::setBlendMode(int blendMode)
{
    if (m_blendMode == blendMode) {
        return;
    }
    m_blendMode = blendMode;
    emit blendModeChanged();
    update();
}

bool
Lr2BlendSprite::supportedBlendMode() const
{
    return supportsBlendMode(m_blendMode);
}

bool
Lr2BlendSprite::supportsBlendMode(int blendMode)
{
    return blendMode == 3 || blendMode == 4 || blendMode == 5 || blendMode == 9;
}

QSGNode*
Lr2BlendSprite::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*)
{
    if (!m_source || !m_source->isTextureProvider() ||
        !supportsBlendMode(m_blendMode)) {
        delete oldNode;
        return nullptr;
    }

    auto* node = static_cast<Lr2BlendNode*>(oldNode);
    if (m_sourceChanged) {
        delete node;
        node = nullptr;
        m_sourceChanged = false;
    }
    if (node == nullptr) {
        node = new Lr2BlendNode(m_source->textureProvider());
    }
    node->setState(boundingRect(),
                   m_sourceRect,
                   m_tint,
                   m_transColor,
                   m_colorKeyEnabled,
                   m_smooth,
                   m_blendMode);
    return node;
}
