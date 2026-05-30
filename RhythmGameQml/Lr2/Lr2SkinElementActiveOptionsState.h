#pragma once

#include <QObject>
#include <QSet>
#include <QVariant>
#include <QtQml/qqmlregistration.h>

class Lr2SkinElementActiveOptionsState : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QVariantList activeOptions READ activeOptions NOTIFY activeOptionsChanged)

public:
    explicit Lr2SkinElementActiveOptionsState(QObject* parent = nullptr);

    QVariantList activeOptions() const;
    QSet<int> activeOptionSet() const;
    bool setActiveOptions(const QVariantList& value);

signals:
    void activeOptionsChanged();

private:
    QVariantList m_activeOptions;
    QSet<int> m_activeOptionSet;
};

