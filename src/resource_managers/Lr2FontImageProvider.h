#pragma once

#include <QImage>
#include <QQuickImageProvider>
#include <QSizeF>

namespace resource_managers {

struct Lr2RenderedFontText
{
    QImage image;
    QSizeF naturalSize;
};

// URL formats served:
//
//   image://lr2font/<absolute-font-path>?text=<urlencoded-string>
//     Composes the entire bitmap-font string into one image. The provider keeps
//     font atlases cached through Lr2FontCache, but intentionally does not keep
//     a global cache of full strings: select screens can contain thousands of
//     high-cardinality titles that would evict useful pixmaps and churn memory.
class Lr2FontImageProvider : public QQuickImageProvider
{
  public:
    Lr2FontImageProvider();

    static Lr2RenderedFontText renderedText(const QString& fontPath,
                                            const QString& text);
    static QImage textImage(const QString& fontPath, const QString& text);

    QImage requestImage(const QString& id,
                        QSize* size,
                        const QSize& requestedSize) override;
};

} // namespace resource_managers
