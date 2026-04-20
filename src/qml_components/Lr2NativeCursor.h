#pragma once

#include <QImage>
#include <QQuickItem>
#include <QRectF>
#include <QSizeF>

class QQuickWindow;

namespace qml_components {

class Lr2NativeCursor : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(QRectF sourceRect READ sourceRect WRITE setSourceRect NOTIFY sourceRectChanged)
    Q_PROPERTY(QSizeF targetSize READ targetSize WRITE setTargetSize NOTIFY targetSizeChanged)

  public:
    explicit Lr2NativeCursor(QQuickItem* parent = nullptr);
    ~Lr2NativeCursor() override;

    auto active() const -> bool;
    void setActive(bool active);

    auto source() const -> QString;
    void setSource(const QString& source);

    auto sourceRect() const -> QRectF;
    void setSourceRect(const QRectF& sourceRect);

    auto targetSize() const -> QSizeF;
    void setTargetSize(const QSizeF& targetSize);

  signals:
    void activeChanged();
    void sourceChanged();
    void sourceRectChanged();
    void targetSizeChanged();

  private:
    void updateCursor();
    void clearCursor();
    auto cursorImage() -> QImage;
    static auto localPathForSource(const QString& source) -> QString;

    bool m_active = false;
    QString m_source;
    QRectF m_sourceRect;
    QSizeF m_targetSize;
    QQuickWindow* m_window = nullptr;
    QString m_loadedSource;
    QImage m_loadedImage;
    QString m_appliedKey;
    bool m_applied = false;
};

} // namespace qml_components
