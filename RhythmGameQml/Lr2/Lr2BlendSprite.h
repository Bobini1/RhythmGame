#pragma once

#include <QColor>
#include <QPointer>
#include <QQuickItem>
#include <QRectF>
#include <QtQml/qqmlregistration.h>

class Lr2BlendSprite : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(
      QQuickItem* source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(QRectF sourceRect READ sourceRect WRITE setSourceRect NOTIFY
                 sourceRectChanged)
    Q_PROPERTY(QColor tint READ tint WRITE setTint NOTIFY tintChanged)
    Q_PROPERTY(QColor transColor READ transColor WRITE setTransColor NOTIFY
                 transColorChanged)
    Q_PROPERTY(bool colorKeyEnabled READ colorKeyEnabled WRITE
                 setColorKeyEnabled NOTIFY colorKeyEnabledChanged)
    Q_PROPERTY(bool smooth READ smooth WRITE setSmooth NOTIFY smoothChanged)
    Q_PROPERTY(
      int blendMode READ blendMode WRITE setBlendMode NOTIFY blendModeChanged)
    Q_PROPERTY(
      bool supportedBlendMode READ supportedBlendMode NOTIFY blendModeChanged)

  public:
    explicit Lr2BlendSprite(QQuickItem* parent = nullptr);

    [[nodiscard]] QQuickItem* source() const;
    void setSource(QQuickItem* source);

    [[nodiscard]] QRectF sourceRect() const;
    void setSourceRect(const QRectF& sourceRect);

    [[nodiscard]] QColor tint() const;
    void setTint(const QColor& tint);

    [[nodiscard]] QColor transColor() const;
    void setTransColor(const QColor& transColor);

    [[nodiscard]] bool colorKeyEnabled() const;
    void setColorKeyEnabled(bool enabled);

    [[nodiscard]] bool smooth() const;
    void setSmooth(bool smooth);

    [[nodiscard]] int blendMode() const;
    void setBlendMode(int blendMode);

    [[nodiscard]] bool supportedBlendMode() const;
    [[nodiscard]] static bool supportsBlendMode(int blendMode);

  signals:
    void sourceChanged();
    void sourceRectChanged();
    void tintChanged();
    void transColorChanged();
    void colorKeyEnabledChanged();
    void smoothChanged();
    void blendModeChanged();

  protected:
    QSGNode* updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*) override;

  private:
    QPointer<QQuickItem> m_source;
    QRectF m_sourceRect;
    QColor m_tint = Qt::white;
    QColor m_transColor = Qt::black;
    bool m_colorKeyEnabled = false;
    bool m_smooth = true;
    int m_blendMode = 1;
    bool m_sourceChanged = false;
};
