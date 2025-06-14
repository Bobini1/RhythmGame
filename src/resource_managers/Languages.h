//
// Created by PC on 13/06/2025.
//

#ifndef LANGUAGES_H
#define LANGUAGES_H
#include "qml_components/ThemeFamily.h"

#include <QHash>
#include <QObject>
#include <qtranslator.h>

namespace resource_managers {

class Languages final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList languages READ getLanguages CONSTANT)
    Q_PROPERTY(QString selectedLanguage READ getSelectedLanguage NOTIFY
                 selectedLanguageChanged)
    QTranslator qtTranslator;
    std::unordered_map<QString, std::unique_ptr<QTranslator>> themeTranslators;
    QMap<QString, qml_components::ThemeFamily> availableThemes;

  public:
    explicit Languages(
      const QMap<QString, qml_components::ThemeFamily>& availableThemes,
      QObject* parent = nullptr);
    auto getLanguages() const -> const QStringList&;
    auto getSelectedLanguage() const -> QString;
    void setSelectedLanguage(const QString& language);
    Q_INVOKABLE static QString getLanguageName(const QString& language);

  signals:
    void selectedLanguageChanged();

  private:
    QStringList languages;
    QList<QLocale> locales;
    QString selectedLanguage;
};

} // namespace resource_managers

#endif // LANGUAGES_H
