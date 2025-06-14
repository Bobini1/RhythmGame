//
// Created by PC on 13/06/2025.
//

#include "Languages.h"

#include <qcoreapplication.h>
#include <qlibraryinfo.h>
#include <magic_enum/magic_enum.hpp>
#include <spdlog/spdlog.h>

namespace resource_managers {
Languages::Languages(
  const QMap<QString, qml_components::ThemeFamily>& availableThemes,
  QObject* parent)
  : QObject(parent)
  , availableThemes(availableThemes)
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

static auto determineLocaleToPick(const QLocale& locale, const QString& themeName, const QMap<QString, QUrl>& translations) -> QString {
    auto localeToPick = locale.name();
    if (!translations.contains(localeToPick)) {
        localeToPick = locale.name().split('_').first();
        if (!translations.contains(localeToPick)) {
            spdlog::trace(
                "Theme {} does not have translation for language: {}",
                themeName.toStdString(),
                localeToPick.toStdString());
            for (const auto& [lang, url] : translations.asKeyValueRange()) {
                if (lang.startsWith(localeToPick)) {
                    return lang;
                }
            }
            spdlog::trace(
                "Theme {} does not have translation for language: {}, picking English",
                themeName.toStdString(),
                localeToPick.toStdString());
            for (const auto& [lang, url] : translations.asKeyValueRange()) {
                if (QLocale{ lang }.language() == QLocale::English) {
                    return lang;
                }
            }
            return "";
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
        if (auto localeToPick =
              determineLocaleToPick(locale, themeName, translations);
            !localeToPick.isEmpty()) {
            if (auto url = translations.value(localeToPick); !url.isValid()) {
                QCoreApplication::removeTranslator(translator.get());
            } else if (!translator->load(url.toLocalFile())) {
                spdlog::error(
                    "Failed to load theme translation for language {}: in theme {}",
                    localeToPick.toStdString(),
                    themeName.toStdString());
                QCoreApplication::removeTranslator(translator.get());
            } else {
                QCoreApplication::installTranslator(translator.get());
            }
        }
    }

    selectedLanguage = language;
    emit selectedLanguageChanged();
}
auto
Languages::getLanguageName(const QString& language) -> QString
{
    return QLocale{ language }.nativeLanguageName();
}
} // namespace resource_managers