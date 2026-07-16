#include "FontResolver.h"

#include <QFontDatabase>
#include <QRawFont>

#include <algorithm>

namespace {
auto
isCjkScript(char32_t character) -> bool
{
    switch (QChar::script(character)) {
        case QChar::Script_Bopomofo:
        case QChar::Script_Han:
        case QChar::Script_Hangul:
        case QChar::Script_Hiragana:
        case QChar::Script_Katakana:
            return true;
        default:
            return false;
    }
}

auto
fallbackScripts(const QString& localeScript) -> QList<QChar::Script>
{
    if (localeScript == QStringLiteral("Hans")) {
        return { QChar::Script_Han };
    }
    if (localeScript == QStringLiteral("Hant")) {
        return { QChar::Script_Han, QChar::Script_Bopomofo };
    }
    if (localeScript == QStringLiteral("Jpan")) {
        return { QChar::Script_Han,
                 QChar::Script_Hiragana,
                 QChar::Script_Katakana };
    }
    if (localeScript == QStringLiteral("Kore")) {
        return { QChar::Script_Han, QChar::Script_Hangul };
    }
    return {};
}
} // namespace

FontResolver::FontResolver(QObject* parent)
  : QObject(parent)
{
}

QFont
FontResolver::resolve(QFont font, const QStringList& families) const
{
    auto uniqueFamilies = QStringList{};
    for (const auto& family : families) {
        const auto trimmedFamily = family.trimmed();
        if (!trimmedFamily.isEmpty() &&
            !uniqueFamilies.contains(trimmedFamily, Qt::CaseInsensitive)) {
            uniqueFamilies.append(trimmedFamily);
        }
    }
    if (!uniqueFamilies.isEmpty()) {
        font.setFamilies(uniqueFamilies);
    }

    auto strategy = static_cast<int>(font.styleStrategy());
    strategy &= ~static_cast<int>(QFont::NoFontMerging);
    strategy |= static_cast<int>(QFont::ContextFontMerging);
    font.setStyleStrategy(static_cast<QFont::StyleStrategy>(strategy));
    return font;
}

bool
FontResolver::containsCjkScript(const QString& text) const
{
    for (const auto character : text.toUcs4()) {
        if (isCjkScript(character)) {
            return true;
        }
    }
    return false;
}

bool
FontResolver::supportsCjkCharacters(const QFont& font,
                                    const QString& text) const
{
    const auto characters = text.toUcs4();
    if (std::ranges::none_of(characters, isCjkScript)) {
        return true;
    }

    const auto rawFont = QRawFont::fromFont(font);
    for (const auto character : characters) {
        if (isCjkScript(character) &&
            (!rawFont.isValid() || !rawFont.supportsCharacter(character))) {
            return false;
        }
    }
    return true;
}

void
FontResolver::setLocaleFallbackFont(const QString& localeScript,
                                    const QString& family)
{
    if (configuredLocaleScript == localeScript &&
        configuredLocaleFamily == family) {
        return;
    }

    for (auto iterator = localeFallbackFonts.cbegin();
         iterator != localeFallbackFonts.cend();
         ++iterator) {
        auto families =
          QFontDatabase::applicationFallbackFontFamilies(iterator.key());
        families.removeAll(iterator.value());
        QFontDatabase::setApplicationFallbackFontFamilies(iterator.key(),
                                                          families);
    }
    localeFallbackFonts.clear();
    configuredLocaleScript = localeScript;
    configuredLocaleFamily = family;

    const auto trimmedFamily = family.trimmed();
    if (trimmedFamily.isEmpty()) {
        return;
    }

    for (const auto script : fallbackScripts(localeScript)) {
        auto families = QFontDatabase::applicationFallbackFontFamilies(script);
        families.removeAll(trimmedFamily);
        families.prepend(trimmedFamily);
        QFontDatabase::setApplicationFallbackFontFamilies(script, families);
        localeFallbackFonts.insert(script, trimmedFamily);
    }
}
