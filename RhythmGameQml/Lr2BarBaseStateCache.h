#pragma once

#include <QObject>
#include <QSet>
#include <QVariant>
#include <QVariantList>
#include <QVector>
#include <QtQml/qqmlregistration.h>

class Lr2BarBaseStateCache : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QVariantList barRows READ barRows WRITE setBarRows NOTIFY barRowsChanged)
    Q_PROPERTY(int selectedRow READ selectedRow WRITE setSelectedRow NOTIFY selectedRowChanged)
    Q_PROPERTY(int skinTime READ skinTime WRITE setSkinTime NOTIFY skinTimeChanged)
    Q_PROPERTY(QVariant timers READ timers WRITE setTimers NOTIFY timersChanged)
    Q_PROPERTY(QVariant activeOptions READ activeOptions WRITE setActiveOptions NOTIFY activeOptionsChanged)
    Q_PROPERTY(QVariantList baseStates READ baseStates NOTIFY baseStatesChanged)
    Q_PROPERTY(int animationLimit READ animationLimit NOTIFY animationLimitChanged)

public:
    explicit Lr2BarBaseStateCache(QObject* parent = nullptr);

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

    QVariantList baseStates() const;
    int animationLimit() const;

signals:
    void barRowsChanged();
    void selectedRowChanged();
    void skinTimeChanged();
    void timersChanged();
    void activeOptionsChanged();
    void baseStatesChanged();
    void animationLimitChanged();

private:
    struct Dst {
        bool valid = false;
        int time = 0;
        int x = 0;
        int y = 0;
        int w = 0;
        int h = 0;
        int acc = 0;
        int a = 255;
        int r = 255;
        int g = 255;
        int b = 255;
        int blend = 0;
        int filter = 0;
        int angle = 0;
        int center = 0;
        int sortId = 0;
        int loop = 0;
        int timer = 0;
        int op1 = 0;
        int op2 = 0;
        int op3 = 0;
        int op4 = 0;
    };

    struct Row {
        QVector<Dst> offDsts;
        QVector<Dst> onDsts;
    };

    void rebuildRows();
    void rebuildActiveOptionSet();
    void rebuildBaseStates();
    void updateAnimationLimit();
    int effectiveSkinTime(int requestedTime) const;
    qreal timerFire(int timerIdx) const;
    bool allOpsMatch(const Dst& dst) const;
    bool checkSingleOp(int op) const;

    static QVector<Dst> parseDsts(const QVariant& value);
    static bool readDst(const QVariant& value, Dst& dst);
    static QVariant rowField(const QVariant& row, const QString& name);
    static int freezeEndTime(const QVector<Dst>& dsts);
    static QVariant currentState(const QVector<Dst>& dsts,
                                 int globalTime,
                                 qreal timerFire,
                                 const Lr2BarBaseStateCache& cache);
    static QVariant copyDstAsState(const Dst& dst, const Dst& controlDst);
    static qreal applyAccel(qreal progress, int accType);

    QVariantList m_barRows;
    QVector<Row> m_rows;
    int m_selectedRow = 0;
    int m_requestedSkinTime = 0;
    int m_effectiveSkinTime = 0;
    QVariant m_timers;
    QVariant m_activeOptions;
    QSet<int> m_activeOptionSet;
    QVariantList m_baseStates;
    int m_animationLimit = 0;
};
