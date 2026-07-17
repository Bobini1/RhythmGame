//
// Created by PC on 13/06/2025.
//

#include "Languages.h"

#include <QQmlEngine>
#include <qcoreapplication.h>
#include <qlibraryinfo.h>
#include <magic_enum/magic_enum.hpp>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <optional>

namespace {
struct LocaleTag
{
    QString canonical;
    QLocale locale;
    bool hasExplicitScript = false;
    bool hasExplicitTerritory = false;
};

auto
isAsciiLetters(const QStringView text) -> bool
{
    return std::ranges::all_of(text, [](const QChar character) {
        return (character >= u'A' && character <= u'Z') ||
               (character >= u'a' && character <= u'z');
    });
}

auto
isAsciiDigits(const QStringView text) -> bool
{
    return std::ranges::all_of(text, [](const QChar character) {
        return character >= u'0' && character <= u'9';
    });
}

auto
parseLocaleTag(QString languageTag) -> std::optional<LocaleTag>
{
    languageTag = languageTag.trimmed();
    languageTag.replace(u'_', u'-');
    const auto parts = languageTag.split(u'-', Qt::SkipEmptyParts);
    if (parts.empty() || parts.size() > 3) {
        return std::nullopt;
    }

    const auto languagePart = QStringView(parts[0]);
    if ((languagePart.size() < 2 || languagePart.size() > 3) ||
        !isAsciiLetters(languagePart)) {
        return std::nullopt;
    }

    const auto language = QLocale::codeToLanguage(languagePart);
    if (language == QLocale::AnyLanguage || language == QLocale::C) {
        return std::nullopt;
    }

    auto script = QLocale::AnyScript;
    auto territory = QLocale::AnyTerritory;
    auto hasExplicitScript = false;
    auto hasExplicitTerritory = false;

    for (auto index = 1; index < parts.size(); ++index) {
        const auto part = QStringView(parts[index]);
        if (!hasExplicitScript && part.size() == 4 && isAsciiLetters(part)) {
            script = QLocale::codeToScript(part);
            if (script == QLocale::AnyScript) {
                return std::nullopt;
            }
            hasExplicitScript = true;
            continue;
        }

        const auto territoryShape =
          (part.size() == 2 && isAsciiLetters(part)) ||
          (part.size() == 3 && isAsciiDigits(part));
        if (!hasExplicitTerritory && territoryShape) {
            territory = QLocale::codeToTerritory(part);
            if (territory == QLocale::AnyTerritory) {
                return std::nullopt;
            }
            hasExplicitTerritory = true;
            continue;
        }
        return std::nullopt;
    }

    const auto locale = QLocale(language, script, territory);
    auto languageCode =
      QLocale::languageToCode(locale.language(), QLocale::ISO639Part1);
    if (languageCode.isEmpty()) {
        languageCode = QLocale::languageToCode(locale.language());
    }
    if (languageCode.isEmpty()) {
        return std::nullopt;
    }

    auto canonicalParts = QStringList{ languageCode.toLower() };
    if (hasExplicitScript) {
        canonicalParts.append(QLocale::scriptToCode(locale.script()));
    }
    if (hasExplicitTerritory) {
        canonicalParts.append(QLocale::territoryToCode(locale.territory()));
    }

    return LocaleTag{ canonicalParts.join(u'-'),
                      locale,
                      hasExplicitScript,
                      hasExplicitTerritory };
}

auto
localeMatchScore(const LocaleTag& requested, const LocaleTag& candidate) -> int
{
    if (requested.locale.language() != candidate.locale.language()) {
        return -1;
    }
    if (requested.canonical == candidate.canonical) {
        return 10'000;
    }

    const auto requestedScript = requested.locale.script();
    const auto candidateScript = candidate.locale.script();
    const auto candidateIsSpecific =
      candidate.hasExplicitScript || candidate.hasExplicitTerritory;
    if (candidateIsSpecific && requestedScript != QLocale::AnyScript &&
        candidateScript != QLocale::AnyScript &&
        requestedScript != candidateScript) {
        return -1;
    }

    auto score = 100;
    if (requestedScript == candidateScript) {
        score += 40;
    }
    if (candidate.hasExplicitScript) {
        score += 5;
    }
    if (candidate.hasExplicitTerritory) {
        score += requested.locale.territory() == candidate.locale.territory()
                   ? 20
                   : -10;
    }
    return score;
}

auto
closestLanguageForTag(const QString& language, const QStringList& languages)
  -> QString
{
    if (languages.contains(language)) {
        return language;
    }

    const auto requested = parseLocaleTag(language);
    if (!requested) {
        return {};
    }

    auto bestLanguage = QString{};
    auto bestScore = -1;
    for (const auto& candidateName : languages) {
        const auto candidate = parseLocaleTag(candidateName);
        if (!candidate) {
            continue;
        }
        const auto score = localeMatchScore(*requested, *candidate);
        if (score > bestScore) {
            bestScore = score;
            bestLanguage = candidateName;
        }
    }
    return bestLanguage;
}
} // namespace

namespace resource_managers {
Languages::Languages(
  const QMap<QString, qml_components::ThemeFamily>& availableThemes,
  QQmlEngine* qmlEngine,
  QObject* parent)
  : QObject(parent)
  , availableThemes(availableThemes)
  , engine(qmlEngine)
{
    for (const auto& [themeName, theme] : availableThemes.asKeyValueRange()) {
        auto translator = std::make_unique<QTranslator>();
        themeTranslators[themeName] = std::move(translator);
    }
    auto languageTags = QSet<QString>{};
    for (const auto& [key, theme] : availableThemes.asKeyValueRange()) {
        for (const auto& [lang, url] :
             theme.getTranslations().asKeyValueRange()) {
            const auto canonical = canonicalLanguageTag(lang);
            if (canonical.isEmpty()) {
                spdlog::error("Unrecognized locale name: {}",
                              lang.toStdString());
                throw std::runtime_error("Unrecognized locale name.");
            }
            languageTags.insert(canonical);
        }
    }
    languages = languageTags.values();
    std::ranges::sort(languages, [](const QString& a, const QString& b) {
        const auto nameComparison =
          QString::localeAwareCompare(getLanguageName(a), getLanguageName(b));
        return nameComparison == 0 ? a < b : nameComparison < 0;
    });
}
auto
Languages::getLanguages() const -> const QStringList&
{
    return languages;
}
auto
Languages::getSelectedLanguage() const -> QString
{
    return selectedLanguage;
}

auto
Languages::getSelectedScript() const -> QString
{
    return getLanguageScript(selectedLanguage);
}

auto
Languages::getSelectedTerritory() const -> QString
{
    return getLanguageTerritory(selectedLanguage);
}

auto
Languages::getSystemLanguage() -> QString
{
    const auto locale = QLocale::system();
    auto language = canonicalLanguageTag(locale.bcp47Name());
    if (language.isEmpty()) {
        language = canonicalLanguageTag(locale.name());
    }
    return language;
}

QString
Languages::canonicalLanguageTag(const QString& language)
{
    const auto parsed = parseLocaleTag(language);
    return parsed ? parsed->canonical : QString{};
}

static auto
determineLocaleToPick(const QString& language,
                      const QString& themeName,
                      const QMap<QString, QUrl>& translations) -> QString
{
    const auto localeToPick =
      closestLanguageForTag(language, translations.keys());
    if (!localeToPick.isEmpty()) {
        return localeToPick;
    }

    spdlog::trace("Theme {} does not have translation for language: {}",
                  themeName.toStdString(),
                  language.toStdString());
    return {};
}

void
Languages::setSelectedLanguage(const QString& language)
{
    auto canonicalLanguage = canonicalLanguageTag(language);
    if (canonicalLanguage.isEmpty()) {
        canonicalLanguage = getSystemLanguage();
    }
    if (selectedLanguage == canonicalLanguage) {
        return;
    }

    const auto locale = QLocale(canonicalLanguage);
    using namespace Qt::Literals::StringLiterals;
    if (!qtTranslator.load(
          locale,
          u"qt"_s,
          u"_"_s,
          QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
        spdlog::warn("Failed to load Qt translation for language: {}",
                     language.toStdString());
    }
    for (const auto& [themeName, translator] : themeTranslators) {
        auto translations = availableThemes.value(themeName).getTranslations();
        auto localeToPick =
          determineLocaleToPick(canonicalLanguage, themeName, translations);
        if (localeToPick.isEmpty()) {
            localeToPick = determineLocaleToPick(
              getSystemLanguage(), themeName, translations);
        }
        if (localeToPick.isEmpty()) {
            localeToPick = determineLocaleToPick(
              QStringLiteral("en"), themeName, translations);
        }
        if (auto url = translations.value(localeToPick); !url.isValid()) {
            QCoreApplication::removeTranslator(translator.get());
        } else if (!translator->load(url.toLocalFile())) {
            spdlog::error("Failed to load theme translation for language "
                          "{}: in theme {}",
                          localeToPick.toStdString(),
                          themeName.toStdString());
            QCoreApplication::removeTranslator(translator.get());
        } else {
            QCoreApplication::installTranslator(translator.get());
        }
    }
    engine->retranslate();

    selectedLanguage = canonicalLanguage;
    emit selectedLanguageChanged();
}
auto
Languages::getLanguageName(const QString& language) -> QString
{
    const auto parsed = parseLocaleTag(language);
    if (!parsed) {
        return language;
    }

    auto name = parsed->locale.nativeLanguageName();
    if (name.isEmpty()) {
        name = QLocale::languageToString(parsed->locale.language());
    }
    if (parsed->hasExplicitScript) {
        name += QStringLiteral(" (%1)").arg(
          QLocale::scriptToString(parsed->locale.script()));
    } else if (parsed->hasExplicitTerritory) {
        auto territoryName = parsed->locale.nativeTerritoryName();
        if (territoryName.isEmpty()) {
            territoryName =
              QLocale::territoryToString(parsed->locale.territory());
        }
        name += QStringLiteral(" (%1)").arg(territoryName);
    }
    return name;
}

QString
Languages::getLanguageScript(const QString& language)
{
    const auto parsed = parseLocaleTag(language);
    return parsed ? QLocale::scriptToCode(parsed->locale.script()) : QString{};
}

QString
Languages::getLanguageTerritory(const QString& language)
{
    const auto parsed = parseLocaleTag(language);
    return parsed
             ? QLocale::territoryToCode(parsed->locale.territory())
             : QString{};
}

QString
Languages::getClosestLanguage(QString language, const QStringList& languages)
{
    if (languages.contains(language)) {
        return language;
    }
    if (languages.isEmpty()) {
        return {};
    }
    auto closestLanguage = closestLanguageForTag(language, languages);
    if (!closestLanguage.isEmpty()) {
        return closestLanguage;
    }
    closestLanguage = closestLanguageForTag(getSystemLanguage(), languages);
    if (!closestLanguage.isEmpty()) {
        return closestLanguage;
    }
    closestLanguage = closestLanguageForTag(QStringLiteral("en"), languages);
    if (!closestLanguage.isEmpty()) {
        return closestLanguage;
    }
    return languages.first();
}
} // namespace resource_managers
