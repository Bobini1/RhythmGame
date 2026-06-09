#pragma once

#include <QHash>
#include <QList>
#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>

class Lr2ResolvedTextRegistry : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QList<int> activeTextIds READ activeTextIds NOTIFY activeTextIdsChanged)
    Q_PROPERTY(int activeTextIdCount READ activeTextIdCount NOTIFY activeTextIdsChanged)
    Q_PROPERTY(int activeTextIdRevision READ activeTextIdRevision NOTIFY activeTextIdsChanged)

public:
    explicit Lr2ResolvedTextRegistry(QObject* parent = nullptr);

    QList<int> activeTextIds() const;
    int activeTextIdCount() const;
    int activeTextIdRevision() const;

    QString textFor(int sourceTextId) const;
    void retainTextId(int sourceTextId);
    void releaseTextId(int sourceTextId);

    Q_INVOKABLE void setText(int sourceTextId, const QString& text);
    Q_INVOKABLE int activeTextIdAt(int index) const;
    Q_INVOKABLE bool isTextIdActive(int sourceTextId) const;

signals:
    void activeTextIdsChanged();
    void textChanged(int sourceTextId, const QString& text);

private:
    void appendActiveTextId(int sourceTextId);
    void removeActiveTextId(int sourceTextId);

    QHash<int, int> m_refCounts;
    QHash<int, QString> m_texts;
    QList<int> m_activeTextIds;
    int m_activeTextIdRevision = 0;
};
