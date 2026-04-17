#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

namespace resource_managers {

// QML-exposed frontend to Lr2FontCache. Lets the char-by-char
// Lr2BitmapFontText component look up glyph metrics and render glyphs
// directly from the atlas via sourceClipRect. Registered as a QML
// singleton in main.cpp.
class Lr2FontCatalog : public QObject
{
    Q_OBJECT

  public:
    explicit Lr2FontCatalog(QObject* parent = nullptr);

    // Pixel height reported by the font's #S directive, or 0 if unknown.
    Q_INVOKABLE int lineHeight(const QString& fontPath);

    // Layout info for a single codepoint:
    //   { atlas: int, x: int, y: int, w: int, h: int, found: bool }
    // A missing glyph returns { found: false, advance: int } with a space-
    // like advance so callers can skip the character gracefully.
    Q_INVOKABLE QVariantMap glyph(const QString& fontPath, int codepoint);

    // Returns one QVariantMap per codepoint in the string, in order.
    Q_INVOKABLE QVariantList layout(const QString& fontPath,
                                    const QString& text);
};

} // namespace resource_managers
