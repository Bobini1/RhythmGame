#pragma once

#include "Lr2SkinRuntimeTypes.h"
#include "Lr2TimelineStateValue.h"

#include <QObject>
#include <QSet>
#include <QVariant>
#include <QVariantList>
#include <QVector>
#include <QtQml/qqmlregistration.h>

class Lr2BarBaseStateResolver : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QVariantList barRows READ barRows WRITE setBarRows NOTIFY barRowsChanged)
    Q_PROPERTY(int selectedRow READ selectedRow WRITE setSelectedRow NOTIFY selectedRowChanged)
    Q_PROPERTY(int skinTime READ skinTime WRITE setSkinTime NOTIFY skinTimeChanged)
    Q_PROPERTY(QVariant timers READ timers WRITE setTimers NOTIFY timersChanged)
    Q_PROPERTY(QVariant activeOptions READ activeOptions WRITE setActiveOptions NOTIFY activeOptionsChanged)
    Q_PROPERTY(int baseStatesRevision READ baseStatesRevision NOTIFY baseStatesChanged)
    Q_PROPERTY(int animationLimit READ animationLimit NOTIFY animationLimitChanged)
    Q_PROPERTY(bool fastScrollActive READ fastScrollActive NOTIFY baseStatesChanged)
    Q_PROPERTY(qreal fastScrollDx READ fastScrollDx NOTIFY baseStatesChanged)
    Q_PROPERTY(qreal fastScrollDy READ fastScrollDy NOTIFY baseStatesChanged)

public:
    explicit Lr2BarBaseStateResolver(QObject* parent = nullptr);

    QVariantList barRows() const;
    void setBarRows(const QVariantList& rows);

    int selectedRow() const;
    void setSelectedRow(int row);

    int skinTime() const;
    void setSkinTime(int time);

    QVariant timers() const;
    void setTimers(const QVariant& timers);

    QVariant activeOptions() const;
    void setActiveOptions(const QVariant& options);

    int baseStatesRevision() const;
    int animationLimit() const;
    bool fastScrollActive() const;
    qreal fastScrollDx() const;
    qreal fastScrollDy() const;
    Q_INVOKABLE int stateCount() const;
    Q_INVOKABLE Lr2TimelineStateValue stateAt(int row) const;
    Q_INVOKABLE bool stateNeedsInterpolationAt(int row) const;
    Q_INVOKABLE Lr2TimelineStateValue positionlessStateAt(int row) const;
    Lr2TimelineStateValue stateValueAt(int row) const;
    Lr2TimelineStateValue positionlessStateValueAt(int row) const;
    bool stateValidAt(int row) const;
    Q_INVOKABLE qreal stateXAt(int row) const;
    Q_INVOKABLE qreal stateYAt(int row) const;

signals:
    void barRowsChanged();
    void selectedRowChanged();
    void skinTimeChanged();
    void timersChanged();
    void activeOptionsChanged();
    void baseStatesChanged();
    void animationLimitChanged();

private:
    using Dst = lr2skin::runtime::Dst;
    using State = Lr2TimelineStateValue;

    struct Row {
        QVector<Dst> offDsts;
        QVector<Dst> onDsts;
    };

    void rebuildRows();
    void rebuildActiveOptionSet();
    void rebuildBaseStates();
    void rebuildFastScrollStep();
    void updateAnimationLimit();
    int effectiveSkinTime(int requestedTime) const;
    qreal timerFire(int timerIdx) const;

    static QVariant rowField(const QVariant& row, const QString& name);
    static bool statesEqual(const QVector<State>& lhs, const QVector<State>& rhs);

    QVariantList m_barRows;
    QVector<Row> m_rows;
    int m_selectedRow = 0;
    int m_requestedSkinTime = 0;
    int m_effectiveSkinTime = 0;
    QVariant m_timers;
    QVariant m_activeOptions;
    QSet<int> m_activeOptionSet;
    QVector<State> m_baseStates;
    int m_baseStatesRevision = 0;
    int m_animationLimit = 0;
    bool m_fastScrollActive = false;
    qreal m_fastScrollDx = 0.0;
    qreal m_fastScrollDy = 0.0;
};
