#pragma once

#include "Lr2ResolvedTextRegistry.h"

#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QtQml/qqmlregistration.h>

class Lr2ResolvedText : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(Lr2ResolvedTextRegistry* registry READ registry WRITE setRegistry NOTIFY registryChanged)
    Q_PROPERTY(int sourceTextId READ sourceTextId WRITE setSourceTextId NOTIFY sourceTextIdChanged)
    Q_PROPERTY(bool searchText READ isSearchText WRITE setSearchText NOTIFY searchTextChanged)
    Q_PROPERTY(QString editingText READ editingText WRITE setEditingText NOTIFY editingTextChanged)
    Q_PROPERTY(QString text READ text NOTIFY textChanged)

public:
    explicit Lr2ResolvedText(QObject* parent = nullptr);
    ~Lr2ResolvedText() override;

    Lr2ResolvedTextRegistry* registry() const;
    void setRegistry(Lr2ResolvedTextRegistry* value);

    int sourceTextId() const;
    void setSourceTextId(int value);

    bool isSearchText() const;
    void setSearchText(bool value);

    QString editingText() const;
    void setEditingText(const QString& value);

    QString text() const;

signals:
    void registryChanged();
    void sourceTextIdChanged();
    void searchTextChanged();
    void editingTextChanged();
    void textChanged();

private:
    void registerTextId();
    void unregisterTextId();
    void reconnectRegistry();
    void syncFallbackText();
    void updateText();

    QPointer<Lr2ResolvedTextRegistry> m_registry;
    QMetaObject::Connection m_registryTextConnection;
    QMetaObject::Connection m_registryDestroyedConnection;
    int m_sourceTextId = -1;
    int m_registeredTextId = -1;
    bool m_searchText = false;
    QString m_editingText;
    QString m_fallbackText;
    QString m_text;
};
