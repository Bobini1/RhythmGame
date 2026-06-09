#pragma once

#include "Lr2SkinElementActiveOptionsState.h"
#include "Lr2SkinElementDescriptorValue.h"
#include "Lr2SkinRuntimeTypes.h"
#include "Lr2SkinTimerState.h"

#include <QAbstractItemModel>
#include <QHash>
#include <QList>
#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QSet>
#include <QVariant>
#include <QtQml/qqmlregistration.h>

class Lr2SkinElementTimerState;

class Lr2SkinRuntime : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QObject* skinModel READ skinModel WRITE setSkinModel NOTIFY skinModelChanged)
    Q_PROPERTY(QList<int> runtimeActiveOptions READ runtimeActiveOptions WRITE setRuntimeActiveOptions NOTIFY runtimeActiveOptionsChanged)
    Q_PROPERTY(Lr2SkinTimerState* timerState READ timerState WRITE setTimerState NOTIFY timerStateChanged)
    Q_PROPERTY(QString screenKey READ screenKey WRITE setScreenKey NOTIFY screenKeyChanged)
    Q_PROPERTY(bool gameplayScreen READ gameplayScreen WRITE setGameplayScreen NOTIFY gameplayScreenChanged)
    Q_PROPERTY(qreal selectBarElementSortBase READ selectBarElementSortBase WRITE setSelectBarElementSortBase NOTIFY selectBarElementSortBaseChanged)
    Q_PROPERTY(QVariantList elementDescriptors READ elementDescriptors NOTIFY elementDescriptorsChanged)
    Q_PROPERTY(int descriptorRevision READ descriptorRevision NOTIFY elementDescriptorsChanged)
    Q_PROPERTY(QVariantList elementTimerStates READ elementTimerStates NOTIFY elementTimerStatesChanged)
    Q_PROPERTY(QVariantList noteDstTimerFires READ noteDstTimerFires NOTIFY noteDstTimerFiresChanged)
    Q_PROPERTY(QVariantList lineDstTimerFires READ lineDstTimerFires NOTIFY lineDstTimerFiresChanged)
    Q_PROPERTY(bool noteFieldUsesActiveOptions READ noteFieldUsesActiveOptions NOTIFY noteFieldUsesActiveOptionsChanged)

public:
    explicit Lr2SkinRuntime(QObject* parent = nullptr);

    QObject* skinModel() const;
    void setSkinModel(QObject* model);

    QList<int> runtimeActiveOptions() const;
    void setRuntimeActiveOptions(const QList<int>& value);

    Lr2SkinTimerState* timerState() const;
    void setTimerState(Lr2SkinTimerState* value);

    QString screenKey() const;
    void setScreenKey(const QString& value);

    bool gameplayScreen() const;
    void setGameplayScreen(bool value);

    qreal selectBarElementSortBase() const;
    void setSelectBarElementSortBase(qreal value);

    QVariantList elementDescriptors() const;
    int descriptorRevision() const;
    QVariantList elementTimerStates() const;
    QVariantList noteDstTimerFires() const;
    QVariantList lineDstTimerFires() const;

    Q_INVOKABLE Lr2SkinElementDescriptorValue descriptor(int index) const;
    Q_INVOKABLE QVariantList elementActiveOptionsForElement(int index) const;
    Q_INVOKABLE Lr2SkinElementActiveOptionsState* elementActiveOptionsState(int index) const;
    Q_INVOKABLE Lr2TimelineStateValue staticStateForElement(int index) const;
    Q_INVOKABLE Lr2TimelineStateValue stateForElement(int index, int skinTime) const;
    Q_INVOKABLE Lr2TimelineStateValue sliderTrackStateForElement(int index, int skinTime) const;
    Q_INVOKABLE Lr2TimelineStateValue noteDstState(int index, int skinTime) const;
    Q_INVOKABLE Lr2TimelineStateValue lineDstState(int index, int skinTime) const;
    Q_INVOKABLE Lr2TimelineStateValue noteDstStateForTimerFire(int index, int skinTime, int timerFire) const;
    Q_INVOKABLE Lr2TimelineStateValue lineDstStateForTimerFire(int index, int skinTime, int timerFire) const;
    Q_INVOKABLE bool noteDstStateUsesSkinTime(int index) const;
    Q_INVOKABLE bool lineDstStateUsesSkinTime(int index) const;
    Q_INVOKABLE int dstTimerFire(int index) const;
    Q_INVOKABLE int srcTimerFire(int index) const;
    Q_INVOKABLE bool dstTimerCanFire(int index) const;
    Q_INVOKABLE bool srcTimerCanFire(int index) const;
    Q_INVOKABLE QObject* elementTimerState(int index) const;
    bool noteFieldUsesActiveOptions() const;

signals:
    void skinModelChanged();
    void runtimeActiveOptionsChanged();
    void timerStateChanged();
    void screenKeyChanged();
    void gameplayScreenChanged();
    void selectBarElementSortBaseChanged();
    void elementDescriptorsChanged();
    void elementTimerStatesChanged();
    void noteDstTimerFiresChanged();
    void lineDstTimerFiresChanged();
    void noteFieldUsesActiveOptionsChanged();

private slots:
    void rebuildDescriptors();
    void updateTimerFires();
    void updateGameplayTimerFires();
    void updateSelectInfoTimerFires();

private:
    struct TimerSnapshot {
        bool dstTimerCanFire = false;
        bool srcTimerCanFire = false;
        int dstTimerFire = -1;
        int srcTimerFire = -1;
    };

    struct ElementDescriptor : public Lr2SkinElementDescriptorValue {
        lr2skin::runtime::Source source;
        QVector<lr2skin::runtime::Dst> dsts;
        lr2skin::runtime::DstAnalysis dstAnalysis;
        bool elementActive = true;
        QVariantList elementActiveOptions;
        Lr2TimelineStateValue staticState;
        TimerSnapshot timers;
    };

    struct LaneDescriptor {
        QVector<lr2skin::runtime::Dst> dsts;
        lr2skin::runtime::DstAnalysis analysis;
        Lr2TimelineStateValue staticState;
    };

    QVariant modelData(int row, const char* roleName) const;
    QVariantList modelListProperty(const char* name) const;
    LaneDescriptor buildLaneDescriptor(const QVariant& dsts) const;
    ElementDescriptor buildDescriptor(int index,
                                      int type,
                                      const QVariant& sourceValue,
                                      const QVariantList& dstValues,
                                      const QVariantList& noteDsts) const;
    TimerSnapshot timerSnapshotFor(const ElementDescriptor& descriptor) const;
    Lr2TimelineStateValue stateForLane(const QVector<LaneDescriptor>& lanes,
                                       const QVariantList& timerFires,
                                       int index,
                                       int skinTime) const;
    Lr2TimelineStateValue stateForLaneWithTimerFire(const QVector<LaneDescriptor>& lanes,
                                                    int index,
                                                    int skinTime,
                                                    int timerFire) const;
    bool laneStateUsesSkinTime(const QVector<LaneDescriptor>& lanes, int index) const;
    int laneTimerFire(const LaneDescriptor& lane) const;
    QVariantList timerFiresForLanes(const QVector<LaneDescriptor>& lanes) const;
    void updateNoteFieldTimerFires();
    const ElementDescriptor* descriptorAt(int index) const;
    void updateTimerFiresForIndexes(const QVector<int>& indexes);
    void ensureElementTimerStateCount(int count);
    void ensureElementActiveOptionsStateCount(int count);
    void updateElementTimerState(int index, const TimerSnapshot& snapshot);
    bool updateElementActiveOptionsState(int index, const QVariantList& activeOptions, bool active);
    void resetElementTimerStates();
    void resetElementActiveOptionsStates();
    void reconnectSkinModel();
    void reconnectTimerState();
    void refreshActiveOptions(const QVector<int>& changedOptionIds = {});
    void notifyElementDataChanged();

    QPointer<QAbstractItemModel> m_skinModel;
    QPointer<Lr2SkinTimerState> m_timerState;
    QList<int> m_runtimeActiveOptions;
    QSet<int> m_activeOptionSet;
    QString m_screenKey;
    bool m_gameplayScreen = false;
    qreal m_selectBarElementSortBase = 0.0;
    QVector<ElementDescriptor> m_descriptors;
    int m_descriptorRevision = 0;
    QVector<Lr2SkinElementTimerState*> m_elementTimerStates;
    QVector<Lr2SkinElementActiveOptionsState*> m_elementActiveOptionsStates;
    QVector<int> m_activeOptionDescriptorIndexes;
    QHash<int, QVector<int>> m_activeOptionDescriptorIndexesByOption;
    QVector<int> m_activeOptionRefreshIndexMarks;
    int m_activeOptionRefreshMark = 0;
    QVector<int> m_timerDescriptorIndexes;
    QVector<int> m_selectInfoTimerDescriptorIndexes;
    QHash<int, QVector<int>> m_timerDescriptorIndexesByTimer;
    QSet<int> m_noteFieldTimerIds;
    QVector<LaneDescriptor> m_noteLaneDescriptors;
    QVector<LaneDescriptor> m_lineLaneDescriptors;
    QVariantList m_noteDstTimerFires;
    QVariantList m_lineDstTimerFires;
    bool m_noteFieldUsesActiveOptions = false;
    QList<QMetaObject::Connection> m_skinModelConnections;
    QList<QMetaObject::Connection> m_timerStateConnections;
};
