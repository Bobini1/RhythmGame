//
// Created by bobini on 27.11.23.
//

#ifndef THEMECONFIG_H
#define THEMECONFIG_H
#include "ThemeFamily.h"

#include <QQmlPropertyMap>

namespace qml_components {

class Themes : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap availableThemeFamilies READ getAvailableThemeFamilies
                 CONSTANT)

    QVariantMap themes;

  public:
    auto getAvailableThemeFamilies() const -> QVariantMap;

    explicit Themes(QMap<QString, ThemeFamily> themes);
};

} // namespace qml_components

#endif // THEMECONFIG_H
