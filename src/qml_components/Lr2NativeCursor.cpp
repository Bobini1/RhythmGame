#include "Lr2NativeCursor.h"

#include <QCursor>
#include <QGuiApplication>
#include <QPixmap>
#include <QQuickWindow>
#include <QUrl>

namespace qml_components {

namespace {

auto
transparentCursor() -> QCursor
{
    auto pixmap = QPixmap{ 1, 1 };
    pixmap.fill(Qt::transparent);
    return QCursor{ pixmap, 0, 0 };
}

auto
cursorKey(const QString& source, const QRect& sourceRect, const QSize& target)
  -> QString
{
    return QStringLiteral("%1|%2,%3,%4,%5|%6x%7")
      .arg(source)
      .arg(sourceRect.x())
      .arg(sourceRect.y())
      .arg(sourceRect.width())
      .arg(sourceRect.height())
      .arg(target.width())
      .arg(target.height());
}

} // namespace

Lr2NativeCursor::Lr2NativeCursor(QQuickItem* parent) : QQuickItem(parent)
{
    setFlag(ItemHasContents, false);
    connect(this,
            &QQuickItem::windowChanged,
            this,
            [this](QQuickWindow* window) {
                clearCursor();
                m_window = window;
                updateCursor();
            });
}

Lr2NativeCursor::~Lr2NativeCursor()
{
    clearCursor();
}

auto
Lr2NativeCursor::active() const -> bool
{
    return m_active;
}

void
Lr2NativeCursor::setActive(bool active)
{
    if (m_active == active) {
        return;
    }
    m_active = active;
    emit activeChanged();
    updateCursor();
}

auto
Lr2NativeCursor::source() const -> QString
{
    return m_source;
}

void
Lr2NativeCursor::setSource(const QString& source)
{
    if (m_source == source) {
        return;
    }
    m_source = source;
    emit sourceChanged();
    updateCursor();
}

auto
Lr2NativeCursor::sourceRect() const -> QRectF
{
    return m_sourceRect;
}

void
Lr2NativeCursor::setSourceRect(const QRectF& sourceRect)
{
    if (m_sourceRect == sourceRect) {
        return;
    }
    m_sourceRect = sourceRect;
    emit sourceRectChanged();
    updateCursor();
}

auto
Lr2NativeCursor::targetSize() const -> QSizeF
{
    return m_targetSize;
}

void
Lr2NativeCursor::setTargetSize(const QSizeF& targetSize)
{
    if (m_targetSize == targetSize) {
        return;
    }
    m_targetSize = targetSize;
    emit targetSizeChanged();
    updateCursor();
}

void
Lr2NativeCursor::updateCursor()
{
    if (!m_window) {
        return;
    }

    if (!m_active) {
        clearCursor();
        return;
    }

    auto image = cursorImage();
    if (image.isNull() || !m_targetSize.isValid() || m_targetSize.isEmpty()) {
        if (m_appliedKey == QStringLiteral("blank")) {
            return;
        }
        if (m_applied) {
            QGuiApplication::changeOverrideCursor(transparentCursor());
        } else {
            QGuiApplication::setOverrideCursor(transparentCursor());
        }
        m_applied = true;
        m_appliedKey = QStringLiteral("blank");
        return;
    }

    const auto target =
      QSize{ qMax(1, qRound(m_targetSize.width())),
             qMax(1, qRound(m_targetSize.height())) };
    if (image.size() != target) {
        image = image.scaled(target,
                             Qt::IgnoreAspectRatio,
                             Qt::FastTransformation);
    }

    const auto rect = m_sourceRect.toAlignedRect();
    const auto key = cursorKey(m_source, rect, target);
    if (m_appliedKey == key) {
        return;
    }

    const auto pixmap = QPixmap::fromImage(image);
    if (pixmap.isNull()) {
        if (m_applied) {
            QGuiApplication::changeOverrideCursor(transparentCursor());
        } else {
            QGuiApplication::setOverrideCursor(transparentCursor());
        }
        m_applied = true;
        m_appliedKey = QStringLiteral("blank");
        return;
    }

    if (m_applied) {
        QGuiApplication::changeOverrideCursor(QCursor{ pixmap, 0, 0 });
    } else {
        QGuiApplication::setOverrideCursor(QCursor{ pixmap, 0, 0 });
    }
    m_applied = true;
    m_appliedKey = key;
}

void
Lr2NativeCursor::clearCursor()
{
    if (!m_applied) {
        return;
    }
    QGuiApplication::restoreOverrideCursor();
    m_applied = false;
    m_appliedKey.clear();
}

auto
Lr2NativeCursor::cursorImage() -> QImage
{
    if (m_source.isEmpty()) {
        m_loadedSource.clear();
        m_loadedImage = {};
        return {};
    }

    if (m_loadedSource != m_source) {
        m_loadedSource = m_source;
        m_loadedImage = QImage{ localPathForSource(m_source) };
    }

    if (m_loadedImage.isNull()) {
        return {};
    }

    auto rect = m_sourceRect.toAlignedRect();
    if (!rect.isValid() || rect.isEmpty()) {
        return m_loadedImage;
    }

    rect = rect.intersected(m_loadedImage.rect());
    if (!rect.isValid() || rect.isEmpty()) {
        return {};
    }
    return m_loadedImage.copy(rect);
}

auto
Lr2NativeCursor::localPathForSource(const QString& source) -> QString
{
    const auto url = QUrl{ source };
    if (url.isLocalFile()) {
        return url.toLocalFile();
    }
    if (url.scheme() == QStringLiteral("qrc")) {
        return QStringLiteral(":") + url.path();
    }
    return source;
}

} // namespace qml_components
