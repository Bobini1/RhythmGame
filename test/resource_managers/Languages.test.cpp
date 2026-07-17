#include "resource_managers/Languages.h"

#include <catch2/catch_test_macros.hpp>

#include <QStringList>

using resource_managers::Languages;

TEST_CASE("Language tags preserve scripts while normalizing separators",
          "[languages][locale]")
{
    CHECK(Languages::canonicalLanguageTag(QStringLiteral("zh_Hans_CN")) ==
          QStringLiteral("zh-Hans-CN"));
    CHECK(Languages::canonicalLanguageTag(QStringLiteral("zh-hant-tw")) ==
          QStringLiteral("zh-Hant-TW"));
    CHECK(Languages::canonicalLanguageTag(QStringLiteral("pl_PL")) ==
          QStringLiteral("pl-PL"));
    CHECK(Languages::canonicalLanguageTag(QStringLiteral("en")) ==
          QStringLiteral("en"));
    CHECK(Languages::canonicalLanguageTag(QStringLiteral("not-a-locale"))
            .isEmpty());
}

TEST_CASE("Language locale metadata resolves script and territory",
          "[languages][locale]")
{
    CHECK(Languages::getLanguageScript(QStringLiteral("en")) ==
          QStringLiteral("Latn"));
    CHECK(Languages::getLanguageScript(QStringLiteral("zh")) ==
          QStringLiteral("Hans"));
    CHECK(Languages::getLanguageScript(QStringLiteral("zh-Hant-HK")) ==
          QStringLiteral("Hant"));
    CHECK(Languages::getLanguageScript(QStringLiteral("ja")) ==
          QStringLiteral("Jpan"));
    CHECK(Languages::getLanguageScript(QStringLiteral("ko")) ==
          QStringLiteral("Kore"));
    CHECK(Languages::getLanguageTerritory(QStringLiteral("zh-Hant-HK")) ==
          QStringLiteral("HK"));
}

TEST_CASE("Language matching distinguishes simplified and traditional Han",
          "[languages][locale]")
{
    const auto choices = QStringList{ QStringLiteral("en"),
                                      QStringLiteral("zh-Hans"),
                                      QStringLiteral("zh-Hant") };

    CHECK(Languages::getClosestLanguage(QStringLiteral("zh_CN"), choices) ==
          QStringLiteral("zh-Hans"));
    CHECK(Languages::getClosestLanguage(QStringLiteral("zh-Hant-TW"),
                                        choices) == QStringLiteral("zh-Hant"));
}

TEST_CASE("Language matching uses territory within a shared script",
          "[languages][locale]")
{
    const auto choices = QStringList{ QStringLiteral("zh-Hant-TW"),
                                      QStringLiteral("zh-Hant-HK"),
                                      QStringLiteral("zh-Hant") };

    CHECK(Languages::getClosestLanguage(QStringLiteral("zh_HK"), choices) ==
          QStringLiteral("zh-Hant-HK"));
    CHECK(Languages::getClosestLanguage(QStringLiteral("zh_TW"), choices) ==
          QStringLiteral("zh-Hant-TW"));
}

TEST_CASE("Language matching returns original localized-object keys",
          "[languages][locale]")
{
    const auto choices =
      QStringList{ QStringLiteral("en_US"), QStringLiteral("zh_Hant") };

    CHECK(Languages::getClosestLanguage(QStringLiteral("zh-Hant-TW"),
                                        choices) == QStringLiteral("zh_Hant"));
    CHECK(Languages::getClosestLanguage(QStringLiteral("en-GB"), choices) ==
          QStringLiteral("en_US"));
}

TEST_CASE("Language matching rejects an explicitly incompatible script",
          "[languages][locale]")
{
    const auto choices =
      QStringList{ QStringLiteral("zh-Hant"), QStringLiteral("zh") };

    CHECK(Languages::getClosestLanguage(QStringLiteral("zh-Hans-CN"),
                                        choices) == QStringLiteral("zh"));
}
