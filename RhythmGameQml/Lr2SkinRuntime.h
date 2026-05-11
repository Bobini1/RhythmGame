#pragma once

#include "Lr2SkinRuntimeTypes.h"

#include <QAbstractItemModel>
#include <QList>
#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QVariant>
#include <QtQml/qqmlregistration.h>

class Lr2SkinTimerState;
class Lr2SkinElementTimerState;
class Lr2SkinElementActiveOptionsState;

class Lr2SkinRuntime : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QObject* skinModel READ skinModel WRITE setSkinModel NOTIFY skinModelChanged)
    Q_PROPERTY(QVariant runtimeActiveOptions READ runtimeActiveOptions WRITE setRuntimeActiveOptions NOTIFY runtimeActiveOptionsChanged)
    Q_PROPERTY(QObject* timerState READ timerState WRITE setTimerState NOTIFY timerStateChanged)
    Q_PROPERTY(QString screenKey READ screenKey WRITE setScreenKey NOTIFY screenKeyChanged)
    Q_PROPERTY(bool gameplayScreen READ gameplayScreen WRITE setGameplayScreen NOTIFY gameplayScreenChanged)
    Q_PROPERTY(qreal selectBarElementSortBase READ selectBarElementSortBase WRITE setSelectBarElementSortBase NOTIFY selectBarElementSortBaseChanged)
    Q_PROPERTY(int revision READ revision NOTIFY revisionChanged)
    Q_PROPERTY(int timerRevision READ timerRevision NOTIFY timerRevisionChanged)
    Q_PROPERTY(int selectInfoTimerRevision READ selectInfoTimerRevision NOTIFY selectInfoTimerRevisionChanged)
    Q_PROPERTY(int activeOptionsRevision READ activeOptionsRevision NOTIFY activeOptionsRevisionChanged)

public:
    explicit Lr2SkinRuntime(QObject* parent = nullptr);

    QObject* skinModel() const;
    void setSkinModel(QObject* model);

    QVariant runtimeActiveOptions() const;
    void setRuntimeActiveOptions(const QVariant& value);

    QObject* timerState() const;
    void setTimerState(QObject* value);

    QString screenKey() const;
    void setScreenKey(const QString& value);

    bool gameplayScreen() const;
    void setGameplayScreen(bool value);

    qreal selectBarElementSortBase() const;
    void setSelectBarElementSortBase(qreal value);

    int revision() const;
    int timerRevision() const;
    int selectInfoTimerRevision() const;
    int activeOptionsRevision() const;

    Q_INVOKABLE QVariantMap descriptor(int index) const;
    Q_INVOKABLE QVariantList elementActiveOptionsForElement(int index) const;
    Q_INVOKABLE QObject* elementActiveOptionsState(int index) const;
    Q_INVOKABLE QVariant staticStateForElement(int index) const;
    Q_INVOKABLE QVariant stateForElement(int index, int skinTime) const;
    Q_INVOKABLE QVariant sliderTrackStateForElement(int index, int skinTime) const;
    Q_INVOKABLE QVariant noteDstState(int index, int skinTime) const;
    Q_INVOKABLE QVariant lineDstState(int index, int skinTime) const;
    Q_INVOKABLE int dstTimerFire(int index) const;
    Q_INVOKABLE int srcTimerFire(int index) const;
    Q_INVOKABLE bool dstTimerCanFire(int index) const;
    Q_INVOKABLE bool srcTimerCanFire(int index) const;
    Q_INVOKABLE QObject* elementTimerState(int index) const;
    Q_INVOKABLE bool noteFieldUsesActiveOptions() const;
    Q_INVOKABLE bool noteFieldUsesTimers() const;

signals:
    void skinModelChanged();
    void runtimeActiveOptionsChanged();
    void timerStateChanged();
    void screenKeyChanged();
    void gameplayScreenChanged();
    void selectBarElementSortBaseChanged();
    void revisionChanged();
    void timerRevisionChanged();
    void selectInfoTimerRevisionChanged();
    void activeOptionsRevisionChanged();

private slots:
    void rebuildDescriptors();
    void updateTimerFires();
    void updateSelectInfoTimerFires();

private:
    struct TimerSnapshot {
        bool dstTimerCanFire = false;
        bool srcTimerCanFire = false;
        int dstTimerFire = -1;
        int srcTimerFire = -1;
    };

    struct ElementDescriptor {
        int index = -1;
        int type = -1;
        lr2skin::runtime::Source source;
        QVector<lr2skin::runtime::Dst> dsts;
        lr2skin::runtime::DstAnalysis dstAnalysis;
        QVariantList elementActiveOptions;
        QVariant staticState;
        qreal z = 0.0;
        bool usesSkinTime = false;
        bool usesElementSkinTime = false;
        bool useDirectElementSkinClock = false;
        bool needsManualElementSkinTime = false;
        int elementSkinClockMode = lr2skin::runtime::ManualClock;
        bool sourceHasFrameAnimation = false;
        bool usesSelectHeldButtonTimer = false;
        bool usesLiveDstClock = false;
        bool usesLiveSourceClock = false;
        bool usesLiveSelectClock = false;
        bool usesDynamicDstTimer = false;
        bool usesDynamicSrcTimer = false;
        bool spriteUsesDirectSkinClock = false;
        int spriteSkinClockMode = lr2skin::runtime::ManualClock;
        int spriteSourceSkinClockMode = lr2skin::runtime::ManualClock;
        int spriteStateOverrideKind = lr2skin::runtime::NoSpriteStateOverride;
        bool usesSpriteStateOverride = false;
        bool usesSpriteForceHidden = false;
        bool usesButtonFrameOverride = false;
        bool sourceMouseCursor = false;
        bool dstOffsetsEnabled = false;
        int dstOffsetSide = 1;
        int scratchRotationSide = 0;
        int dstTimer = 0;
        int srcTimer = 0;
        bool selectScrollSlider = false;
        bool genericSlider = false;
        bool gameplayProgressSlider = false;
        bool gameplayLaneCoverSlider = false;
        bool numberRefSlider = false;
        int buttonId = 0;
        TimerSnapshot timers;
    };

    struct LaneDescriptor {
        QVector<lr2skin::runtime::Dst> dsts;
        lr2skin::runtime::DstAnalysis analysis;
        QVariant staticState;
    };

    QVariant modelData(int row, const char* roleName) const;
    QVariantList modelListProperty(const char* name) const;
    QVariantMap descriptorMap(const ElementDescriptor& descriptor) const;
    LaneDescriptor buildLaneDescriptor(const QVariant& dsts) const;
    ElementDescriptor buildDescriptor(int index,
                                      int type,
                                      const QVariant& sourceValue,
                                      const QVariantList& dstValues,
                                      const QVariantList& noteDsts) const;
    TimerSnapshot timerSnapshotFor(const ElementDescriptor& descriptor) const;
    QVariant stateForLane(const QVector<LaneDescriptor>& lanes, int index, int skinTime) const;
    const ElementDescriptor* descriptorAt(int index) const;
    void updateTimerFiresForIndexes(const QVector<int>& indexes);
    void ensureElementTimerStateCount(int count);
    void ensureElementActiveOptionsStateCount(int count);
    void updateElementTimerState(int index, const TimerSnapshot& snapshot);
    bool updateElementActiveOptionsState(int index, const QVariantList& activeOptions);
    void resetElementTimerStates();
    void resetElementActiveOptionsStates();
    void reconnectSkinModel();
    void reconnectTimerState();
    void refreshActiveOptions();
    void bumpRevision();
    void bumpTimerRevision();
    void bumpSelectInfoTimerRevision();
    void bumpActiveOptionsRevision();

    QPointer<QAbstractItemModel> m_skinModel;
    QPointer<Lr2SkinTimerState> m_timerState;
    QVariant m_runtimeActiveOptions;
    QSet<int> m_activeOptionSet;
    QString m_screenKey;
    bool m_gameplayScreen = false;
    qreal m_selectBarElementSortBase = 0.0;
    QVector<ElementDescriptor> m_descriptors;
    QVector<Lr2SkinElementTimerState*> m_elementTimerStates;
    QVector<Lr2SkinElementActiveOptionsState*> m_elementActiveOptionsStates;
    QVector<int> m_timerDescriptorIndexes;
    QVector<int> m_selectInfoTimerDescriptorIndexes;
    QVector<LaneDescriptor> m_noteLaneDescriptors;
    QVector<LaneDescriptor> m_lineLaneDescriptors;
    bool m_noteFieldUsesActiveOptions = false;
    bool m_noteFieldUsesTimers = false;
    int m_revision = 0;
    int m_timerRevision = 0;
    int m_selectInfoTimerRevision = 0;
    int m_activeOptionsRevision = 0;
    QList<QMetaObject::Connection> m_skinModelConnections;
    QList<QMetaObject::Connection> m_timerStateConnections;
};
