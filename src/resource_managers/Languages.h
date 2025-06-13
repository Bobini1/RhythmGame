//
// Created by PC on 13/06/2025.
//

#ifndef LANGUAGES_H
#define LANGUAGES_H
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
    QList<std::unique_ptr<QTranslator>> themeTranslators;

  public:
    explicit Languages(QObject* parent = nullptr);
    auto getLanguages() const -> const QStringList&;
    auto getSelectedLanguage() const -> QString;
    auto setSelectedLanguage(const QString& language) -> void;

  signals:
    void selectedLanguageChanged();

  private:
    QStringList languages;
    QString selectedLanguage;
};

} // namespace resource_managers

#endif //LANGUAGES_H
