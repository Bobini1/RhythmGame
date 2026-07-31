//
// Created by PC on 13/06/2025.
//

#ifndef LANGUAGES_H
#define LANGUAGES_H
#include "qml_components/ThemeFamily.h"

#include <QHash>
#include <QLocale>
#include <QObject>
#include <qtranslator.h>

class QQmlEngine;
namespace resource_managers {

/**
 * @brief Manages the available languages and the selected language.
 */
class Languages final : public QObject
{
    Q_OBJECT
    /**
     * @brief The list of available languages.
     * @details This is a list of language codes, e.g. "en", "de", "fr", etc.
     * The list is populated from the available translation files, defined in
     * `theme.json` of each theme.
     */
    Q_PROPERTY(QStringList languages READ getLanguages CONSTANT)
    /**
     * @brief The currently selected language.
     * @details This is the language code of the currently selected language.
     * It will change the language for the whole application.
     * @note You can't set this property from QML, set GeneralVars::language
     * on the resource_managers::ProfileList::mainProfile instead.
     */
    Q_PROPERTY(QString selectedLanguage READ getSelectedLanguage NOTIFY
                 selectedLanguageChanged)
    Q_PROPERTY(QString selectedScript READ getSelectedScript NOTIFY
                 selectedLanguageChanged)
    Q_PROPERTY(QString selectedTerritory READ getSelectedTerritory NOTIFY
                 selectedLanguageChanged)
    Q_PROPERTY(QString systemLanguage READ getSystemLanguage CONSTANT)
    QTranslator qtTranslator;
    std::unordered_map<QString, std::unique_ptr<QTranslator>> themeTranslators;
    QMap<QString, qml_components::ThemeFamily> availableThemes;
    QQmlEngine* engine;

  public:
    explicit Languages(
      const QMap<QString, qml_components::ThemeFamily>& availableThemes,
      QQmlEngine* qmlEngine,
      QObject* parent = nullptr);
    auto getLanguages() const -> const QStringList&;
    auto getSelectedLanguage() const -> QString;
    auto getSelectedScript() const -> QString;
    auto getSelectedTerritory() const -> QString;
    static auto getSystemLanguage() -> QString;
    void setSelectedLanguage(const QString& language);
    /**
     * Converts a supported locale identifier to a BCP 47 language tag.

     * * Both underscore and hyphen separators are accepted.
     */
    Q_INVOKABLE static QString canonicalLanguageTag(const QString& language);
    /**
     * Returns the native, user-friendly name of the given language code.
     */
    Q_INVOKABLE static QString getLanguageName(const QString& language);
    Q_INVOKABLE static QString getLanguageScript(const QString& language);
    Q_INVOKABLE static QString getLanguageTerritory(const QString& language);
    /**
     * Returns the closest matching language from the given list of languages.
     * Will default to "en" if no match is found.
     */
    Q_INVOKABLE static QString getClosestLanguage(QString language,
                                                  const QStringList& languages);

  signals:
    void selectedLanguageChanged();

  private:
    QStringList languages;
    QString selectedLanguage;
};

} // namespace resource_managers

#endif // LANGUAGES_H
