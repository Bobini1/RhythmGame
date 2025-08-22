//
// Created by PC on 13/06/2025.
//

#include "Languages.h"

#include <QQmlEngine>
#include <qcoreapplication.h>
#include <qlibraryinfo.h>
#include <magic_enum/magic_enum.hpp>
#include <spdlog/spdlog.h>

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
    auto locales = QSet<QLocale>{};
    for (const auto& [key, theme] : availableThemes.asKeyValueRange()) {
        for (const auto& [lang, url] :
             theme.getTranslations().asKeyValueRange()) {
            auto locale = QLocale{ lang };
            if (locale.name() == "C") {
                spdlog::error("Unrecognized locale name: {}",
                              lang.toStdString());
                throw std::runtime_error("Unrecognized locale name.");
            }
            locales.insert(locale);
        }
    }
    auto sortedLocales = locales.values();
    std::ranges::sort(sortedLocales, [](const QLocale& a, const QLocale& b) {
        return getLanguageName(a.name()) < getLanguageName(b.name());
    });
    this->locales = std::move(sortedLocales);
    for (const auto& locale : this->locales) {
        languages.append(locale.name());
    }
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

static auto
determineLocaleToPick(const QLocale& locale,
                      const QString& themeName,
                      const QMap<QString, QUrl>& translations) -> QString
{
    auto localeToPick = locale.name();
    if (translations.contains(localeToPick)) {
        return localeToPick;
    }
    localeToPick = locale.name().split('_').first();
    if (translations.contains(localeToPick)) {
        return localeToPick;
    }
    spdlog::trace("Theme {} does not have translation for language: {}",
                  themeName.toStdString(),
                  localeToPick.toStdString());
    for (const auto& [lang, url] : translations.asKeyValueRange()) {
        if (lang.startsWith(localeToPick)) {
            return lang;
        }
    }
    return localeToPick;
}

void
Languages::setSelectedLanguage(const QString& language)
{
    if (selectedLanguage == language) {
        return;
    }

    const auto locale = QLocale(language);
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
          determineLocaleToPick(locale, themeName, translations);
        if (localeToPick.isEmpty()) {
            localeToPick =
              determineLocaleToPick(QLocale::system(), themeName, translations);
        }
        if (localeToPick.isEmpty()) {
            localeToPick = determineLocaleToPick(
              QLocale(QLocale::English), themeName, translations);
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

    selectedLanguage = language;
    emit selectedLanguageChanged();
}
auto
Languages::getLanguageName(const QString& language) -> QString
{
    return QLocale{ language }.nativeLanguageName();
}
auto
getClosestLanguageImpl(QLocale locale, const QStringList& languages) -> QString
{
    if (languages.contains(locale.name())) {
        return locale.name();
    }
    if (languages.contains(locale.name().split('_').first())) {
        return locale.name().split('_').first();
    }
    for (const auto& lang : languages) {
        if (lang.startsWith(locale.name().split('_').first())) {
            return lang;
        }
    }
    return {};
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
    auto locale = QLocale{ language };
    auto closestLanguage = getClosestLanguageImpl(locale, languages);
    if (!closestLanguage.isEmpty()) {
        return closestLanguage;
    }
    locale = QLocale::system();
    closestLanguage = getClosestLanguageImpl(locale, languages);
    if (!closestLanguage.isEmpty()) {
        return closestLanguage;
    }
    locale = QLocale(QLocale::English);
    closestLanguage = getClosestLanguageImpl(locale, languages);
    if (!closestLanguage.isEmpty()) {
        return closestLanguage;
    }
    return languages.first();
}
} // namespace resource_managers