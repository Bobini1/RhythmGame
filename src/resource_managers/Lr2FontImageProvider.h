#pragma once

#include <QQuickImageProvider>

namespace resource_managers {

// URL formats served:
//
//   image://lr2font/<absolute-font-path>?text=<urlencoded-string>
//     Composes the entire bitmap-font string into one cached image. Qt Quick
//     then moves/scales it as a normal texture instead of building glyph items.
class Lr2FontImageProvider : public QQuickImageProvider
{
  public:
    Lr2FontImageProvider();

    QImage requestImage(const QString& id,
                        QSize* size,
                        const QSize& requestedSize) override;
};

} // namespace resource_managers
