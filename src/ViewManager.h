//
// Created by bobini on 11.08.23.
//

#ifndef RHYTHMGAME_VIEWMANAGER_H
#define RHYTHMGAME_VIEWMANAGER_H

#include <QQuickView>
class ViewManager : public QQuickView
{
    Q_OBJECT

    QQuickItem* current{};
    auto createComponentFromFile(const QString& qString, QQmlContext* context)
      -> QQuickItem*;

  public:
    void setView(const QString& filename, QQmlContext* context = nullptr);
    explicit ViewManager(QQmlEngine* engine, QWindow* parent = nullptr);
};

#endif // RHYTHMGAME_VIEWMANAGER_H
