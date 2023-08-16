//
// Created by bobini on 11.08.23.
//

#include <QQuickItem>
#include <QQmlEngine>

#include "ViewManager.h"
void
ViewManager::setView(const QString& filename, QQmlContext* context)
{
    if (current != nullptr) {
        current->setProperty("visible", false);
    }
    auto* root = rootObject();

    // Call a method to load the component and create an instance of it
    auto* newItem = createComponentFromFile(filename, context);
    newItem->setProperty("parent", QVariant::fromValue<QObject*>(root));
    newItem->setProperty("visible", QVariant::fromValue(true));

    QQmlEngine::setObjectOwnership(newItem, QQmlEngine::CppOwnership);

    // Cleanup current object
    if (current != nullptr) {
        current->deleteLater();
    }

    current = newItem;
}
auto
ViewManager::createComponentFromFile(const QString& qString,
                                     QQmlContext* context) -> QQuickItem*
{
    QQmlComponent component(engine(), QUrl::fromLocalFile(qString));
    if (component.isError()) {
        qWarning() << component.errors();
        return nullptr;
    }
    Q_ASSERT(component.isReady());
    auto* object = component.create(context);
    if (object != nullptr) {
        return qobject_cast<QQuickItem*>(object);
    }
    return nullptr;
}
ViewManager::ViewManager(QQmlEngine* engine, QWindow* parent)
  : QQuickView(engine, parent)
{
    setSource(QUrl("qrc:///qt/qml/RhythmGameQml/ContentFrame.qml"));
}
