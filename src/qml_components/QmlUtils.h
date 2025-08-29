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

/**
 * @brief Provides utility functions for QML as an attached property.
 * @note Its name in QML is `QmlUtils`.
 *
 */
class QmlUtilsAttached : public QObject
{
    Q_OBJECT
    QML_ATTACHED(QmlUtilsAttached)
    QML_ELEMENT
    /**
     * @brief The name of the theme that this QML file belongs to.
     */
    Q_PROPERTY(QString themeName READ themeName CONSTANT)
    /**
     * @brief The file name of the QML file that this object is attached to.
     */
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
