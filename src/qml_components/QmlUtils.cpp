//
// Created by PC on 08/06/2025.
//

#include "QmlUtils.h"

namespace qml_components {
auto
QmlUtilsAttached::qmlAttachedProperties(QObject* object) -> QmlUtilsAttached*
{
    return new QmlUtilsAttached(object);
}
QmlUtilsAttached::QmlUtilsAttached(QObject* parent)
  : QObject(parent)
{
}
auto
QmlUtilsAttached::themeName() const -> QString
{
    const auto* context = QQmlEngine::contextForObject(parent());
    auto themeName = QString{};
    while (context != nullptr) {
        const auto url = context->baseUrl();
        if (auto name = getThemeNameForRootFile(url); !name.isEmpty()) {
            themeName = name;
        }
        context = context->parentContext();
    }
    return themeName;
}
auto
QmlUtilsAttached::fileName() const -> QString
{
    const auto* context = QQmlEngine::contextForObject(parent());
    if (context != nullptr) {
        const auto url = context->baseUrl();
        return url.toLocalFile();
    }
    return {};
}
} // qml_components