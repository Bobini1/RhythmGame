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
    Q_PROPERTY(QString fileName READ fileName CONSTANT)

  public:
    inline static std::function<QString(const QUrl&)>
      getThemeNameForRootFile;
    static auto qmlAttachedProperties(QObject* object) -> QmlUtilsAttached*;
    explicit QmlUtilsAttached(QObject* parent = nullptr);
    auto themeName() const -> QString;
    auto fileName() const -> QString;
};

} // namespace qml_components

#endif // QMLUTILS_H
