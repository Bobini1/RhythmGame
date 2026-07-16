#ifndef FONTRESOLVER_H
#define FONTRESOLVER_H

#include <QFont>
#include <QHash>
#include <QObject>
#include <QStringList>
#include <qqmlintegration.h>

class FontResolver : public QObject
{
    Q_OBJECT
    QML_SINGLETON
    QML_ELEMENT

  public:
    explicit FontResolver(QObject* parent = nullptr);

    Q_INVOKABLE QFont resolve(QFont font, const QStringList& families) const;
    Q_INVOKABLE bool containsCjkScript(const QString& text) const;
    Q_INVOKABLE bool supportsCjkCharacters(const QFont& font,
                                           const QString& text) const;
    Q_INVOKABLE void setLocaleFallbackFont(const QString& localeScript,
                                           const QString& family);

  private:
    QHash<QChar::Script, QString> localeFallbackFonts;
    QString configuredLocaleScript;
    QString configuredLocaleFamily;
};

#endif // FONTRESOLVER_H
