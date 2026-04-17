#pragma once

#include <QQuickImageProvider>

namespace resource_managers {

// URL formats served:
//
//   image://lr2font/<absolute-font-path>?atlas=<N>
//     Returns the Nth texture atlas from the font, at its natural size.
//     Used by the char-by-char Lr2BitmapFontText QML component, which
//     clips each glyph out of a shared atlas via sourceClipRect.
//
//   image://lr2font/<absolute-font-path>?text=<urlencoded-string>
//     Legacy: composes the entire string into a single QImage. Kept so
//     that any callers that haven't been migrated to the atlas API still
//     work, but prefer the atlas form — it caches per-atlas instead of
//     per-string.
class Lr2FontImageProvider : public QQuickImageProvider
{
  public:
    Lr2FontImageProvider();

    QImage requestImage(const QString& id,
                        QSize* size,
                        const QSize& requestedSize) override;
};

} // namespace resource_managers
