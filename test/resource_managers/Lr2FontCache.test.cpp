#include <catch2/catch_test_macros.hpp>

#include "resource_managers/Lr2FontCache.h"

#include <QFile>
#include <QTemporaryDir>
#include <QTextStream>

namespace {

auto
writeTestFont(const QString& path) -> void
{
    QFile file(path);
    REQUIRE(file.open(QIODevice::WriteOnly | QIODevice::Text));

    QTextStream stream(&file);
    stream << "#S,18\n";
    stream << "#R,65,0,0,0,3,18\n";   // CP932 0x41 -> A
    stream << "#R,177,0,3,0,3,18\n";  // CP932 0xb1 -> half-width katakana A
    stream << "#R,608,0,6,0,3,18\n";  // CP932 0x82a0 -> hiragana A
    stream << "#R,5050,0,9,0,3,18\n"; // CP932 0x93fa -> day/sun kanji
}

} // namespace

TEST_CASE("LR2 font character ids decode as CP932",
          "[LR2FONT][resource_managers]")
{
    QTemporaryDir tempDir;
    REQUIRE(tempDir.isValid());

    const auto fontPath = tempDir.filePath("font.lr2font");
    writeTestFont(fontPath);

    const auto* dict =
      resource_managers::Lr2FontCache::instance().load(fontPath);
    REQUIRE(dict != nullptr);

    constexpr auto halfWidthKatakanaA = static_cast<char32_t>(0xFF71);
    constexpr auto hiraganaA = static_cast<char32_t>(0x3042);
    constexpr auto dayKanji = static_cast<char32_t>(0x65E5);
    constexpr auto plusMinus = static_cast<char32_t>(0x00B1);

    CHECK(dict->glyphs.contains(U'A'));
    CHECK(dict->glyphs.contains(halfWidthKatakanaA));
    CHECK(dict->glyphs.contains(hiraganaA));
    CHECK(dict->glyphs.contains(dayKanji));
    CHECK(!dict->glyphs.contains(plusMinus));
}
