#pragma once

#include <QColor>
#include <QImage>
#include <QQuickItem>
#include <QSizeF>
#include <QString>
#include <QtQml/qqmlregistration.h>

class Lr2BitmapFontTexture : public QQuickItem {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString fontPath READ fontPath WRITE setFontPath NOTIFY fontPathChanged)
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(QColor textColor READ textColor WRITE setTextColor NOTIFY textColorChanged)
    Q_PROPERTY(int textureFilter READ textureFilter WRITE setTextureFilter NOTIFY textureFilterChanged)
    Q_PROPERTY(qreal naturalWidth READ naturalWidth NOTIFY naturalSizeChanged)
    Q_PROPERTY(qreal naturalHeight READ naturalHeight NOTIFY naturalSizeChanged)
    Q_PROPERTY(qreal textureHeight READ textureHeight NOTIFY naturalSizeChanged)

public:
    explicit Lr2BitmapFontTexture(QQuickItem* parent = nullptr);

    QString fontPath() const;
    void setFontPath(const QString& value);

    QString text() const;
    void setText(const QString& value);

    QColor textColor() const;
    void setTextColor(const QColor& value);

    int textureFilter() const;
    void setTextureFilter(int value);

    qreal naturalWidth() const;
    qreal naturalHeight() const;
    qreal textureHeight() const;

signals:
    void fontPathChanged();
    void textChanged();
    void textColorChanged();
    void textureFilterChanged();
    void naturalSizeChanged();

protected:
    QSGNode* updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*) override;
    void geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) override;

private:
    void rebuildImage();
    static bool colorNeedsTint(const QColor& color);
    static QImage tintedImage(const QImage& image, const QColor& color);

    QString m_fontPath;
    QString m_text;
    QImage m_baseImage;
    QColor m_textColor = Qt::white;
    QImage m_image;
    QSizeF m_naturalSize;
    int m_textureFilter = 1;
    bool m_textureDirty = true;
};
