//
// Created by PC on 13/06/2025.
//

#include "Languages.h"

#include <qcoreapplication.h>
#include <qlibraryinfo.h>
#include <magic_enum/magic_enum.hpp>
#include <spdlog/spdlog.h>

namespace resource_managers {
Languages::Languages(QObject* parent)
  : QObject(parent)
{
    QCoreApplication::installTranslator(&qtTranslator);
    for (const auto& locale : magic_enum::enum_values<QLocale::Language>()) {
        languages.append(QLocale{ locale }.name());
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
auto
Languages::setSelectedLanguage(const QString& language) -> void
{
    if (selectedLanguage == language) {
        return;
    }
    using namespace Qt::Literals::StringLiterals;
    if (!qtTranslator.load(QLocale(language), u"qt"_s, u"_"_s,
                          QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
        spdlog::error("Failed to load Qt translation for language: {}",
                          language.toStdString());
    }

    selectedLanguage = language;
    emit selectedLanguageChanged();
}
} // resource_managers