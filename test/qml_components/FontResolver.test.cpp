#include "FontResolver.h"

#include <catch2/catch_test_macros.hpp>

#include <QFont>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QStringList>

TEST_CASE("FontResolver creates an ordered deduplicated fallback stack",
          "[FontResolver]")
{
    auto source = QFont(QStringLiteral("Primary"));
    source.setPixelSize(31);
    source.setWeight(QFont::DemiBold);
    source.setItalic(true);
    source.setFeature(QFont::Tag("tnum"), 1);
    source.setStyleStrategy(QFont::NoFontMerging);

    const auto resolved =
      FontResolver{}.resolve(source,
                             QStringList{ QStringLiteral("Primary"),
                                          QString{},
                                          QStringLiteral("Fallback"),
                                          QStringLiteral("primary"),
                                          QStringLiteral("Fallback") });

    CHECK(resolved.families() ==
          QStringList{ QStringLiteral("Primary"), QStringLiteral("Fallback") });
    CHECK(resolved.pixelSize() == 31);
    CHECK(resolved.weight() == QFont::DemiBold);
    CHECK(resolved.italic());
    CHECK(resolved.featureValue(QFont::Tag("tnum")) == 1);
    CHECK((resolved.styleStrategy() & QFont::ContextFontMerging) != 0);
    CHECK((resolved.styleStrategy() & QFont::NoFontMerging) == 0);
}

TEST_CASE("FontResolver leaves the source family when no stack is supplied",
          "[FontResolver]")
{
    const auto source = QFont(QStringLiteral("Primary"));
    const auto resolved = FontResolver{}.resolve(source, {});

    CHECK(resolved.families() == source.families());
}

TEST_CASE("FontResolver detects CJK scripts in metadata", "[FontResolver]")
{
    const auto resolver = FontResolver{};

    CHECK(resolver.containsCjkScript(QString(QChar(0x6F22))));
    CHECK(resolver.containsCjkScript(QString(QChar(0x3042))));
    CHECK(resolver.containsCjkScript(QString(QChar(0xD55C))));
    CHECK_FALSE(resolver.containsCjkScript(QStringLiteral("RhythmGame 123")));
    CHECK(resolver.supportsCjkCharacters(QFont{},
                                         QStringLiteral("RhythmGame 123")));
}

TEST_CASE("FontResolver registers and restores locale-specific fallbacks",
          "[FontResolver]")
{
    int argc = 1;
    char applicationName[] = "FontResolverTests";
    char* argv[]{ applicationName, nullptr };
    QGuiApplication application(argc, argv);

    const auto family = QStringLiteral("RhythmGame FontResolver Test");
    const auto previousHan =
      QFontDatabase::applicationFallbackFontFamilies(QChar::Script_Han);
    const auto previousBopomofo =
      QFontDatabase::applicationFallbackFontFamilies(QChar::Script_Bopomofo);
    auto resolver = FontResolver{};

    resolver.setLocaleFallbackFont(QStringLiteral("Hant"), family);

    CHECK(QFontDatabase::applicationFallbackFontFamilies(QChar::Script_Han)
            .first() == family);
    CHECK(QFontDatabase::applicationFallbackFontFamilies(QChar::Script_Bopomofo)
            .first() == family);

    resolver.setLocaleFallbackFont(QStringLiteral("Latn"), {});

    CHECK(QFontDatabase::applicationFallbackFontFamilies(QChar::Script_Han) ==
          previousHan);
    CHECK(QFontDatabase::applicationFallbackFontFamilies(
            QChar::Script_Bopomofo) == previousBopomofo);
}
