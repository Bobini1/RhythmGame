#pragma once

#include <QHash>
#include <QJSValue>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QSet>
#include <QString>
#include <QtQml/qqmlregistration.h>

class Lr2ResolvedText;

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
    void retainTextId(int sourceTextId, Lr2ResolvedText* listener = nullptr);
    void releaseTextId(int sourceTextId, Lr2ResolvedText* listener = nullptr);

    Q_INVOKABLE void setText(int sourceTextId, const QString& text);
    Q_INVOKABLE int activeTextIdAt(int index) const;
    Q_INVOKABLE bool isTextIdActive(int sourceTextId) const;
    Q_INVOKABLE void queueAllTextRefresh();
    Q_INVOKABLE void queueTextRefreshIds(const QList<int>& sourceTextIds);
    Q_INVOKABLE bool refreshQueuedTexts(const QJSValue& resolveText);

signals:
    void activeTextIdsChanged();

private:
    void appendActiveTextId(int sourceTextId);
    void removeActiveTextId(int sourceTextId);
    QList<int> takeQueuedTextRefreshIds();
    void addTextListener(int sourceTextId, Lr2ResolvedText* listener);
    void removeTextListener(int sourceTextId, Lr2ResolvedText* listener);
    void notifyTextListeners(int sourceTextId, const QString& text);

    QHash<int, int> m_refCounts;
    QHash<int, QString> m_texts;
    QHash<int, QList<QPointer<Lr2ResolvedText>>> m_textListeners;
    QList<int> m_activeTextIds;
    QSet<int> m_queuedTextRefreshIds;
    int m_activeTextIdRevision = 0;
    bool m_queuedTextRefreshAll = false;
};
