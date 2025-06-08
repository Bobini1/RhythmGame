//
// Created by PC on 08/06/2025.
//

#ifndef QMLUTILS_H
#define QMLUTILS_H
#include <QObject>
#include <QQmlEngine>
#include <QQuickItem>
#include <qqmlcontext.h>
#include <qqmlintegration.h>

namespace qml_components {

class QmlUtilsAttached : public QObject
{
    Q_OBJECT
    QML_ATTACHED(QmlUtilsAttached)
    QML_ELEMENT
    Q_PROPERTY(QString themeName READ themeName CONSTANT)

  public:
    inline static std::function<QString(const QUrl&)>
      getThemeNameForRootFile;
    static auto qmlAttachedProperties(QObject* object) -> QmlUtilsAttached*
    {
        return new QmlUtilsAttached(object);
    }
    explicit QmlUtilsAttached(QObject* parent = nullptr)
      : QObject(parent)
    {
    }
    auto themeName() const -> QString
    {
        const auto* context = QQmlEngine::contextForObject(parent());
        auto themeName = QString{};
        while (context != nullptr) {
            const auto url = context->baseUrl();
            if (auto name = getThemeNameForRootFile(url);
                !name.isEmpty()) {
                themeName = name;
            }
            context = context->parentContext();
        }
        return themeName;
    }
};

} // namespace qml_components

#endif // QMLUTILS_H
