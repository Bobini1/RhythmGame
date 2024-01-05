//
// Created by bobini on 20.11.23.
//

#ifndef SKIN_H
#define SKIN_H
#include <QObject>
#include <QMap>

namespace qml_components {
class ThemeFamily
{
    Q_GADGET
    Q_PROPERTY(QString path READ getPath CONSTANT)
    Q_PROPERTY(QVariantMap themes READ getThemes CONSTANT)

  public:
    auto getPath() const -> QString;
    auto getThemes() const -> QVariantMap;
    ThemeFamily() = default; // necessary for Qt
    ThemeFamily(QString path, QMap<QString, QUrl> themes);

  private:
    QString path;
    QVariantMap themes;
};
} // namespace qml_components
#endif // SKIN_H
