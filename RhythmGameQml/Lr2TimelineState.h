#pragma once

#include <QObject>
#include <QMetaObject>
#include <QPointer>
#include <QSet>
#include <QVariant>
#include <QVariantList>
#include <QVector>
#include <QtQml/qqmlregistration.h>

class Lr2SkinClock;
class Lr2SkinElementActiveOptionsState;

class Lr2TimelineState : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QObject* skinClock READ skinClock WRITE setSkinClock NOTIFY skinClockChanged)
    Q_PROPERTY(int clockMode READ clockMode WRITE setClockMode NOTIFY clockModeChanged)
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(QVariantList dsts READ dsts WRITE setDsts NOTIFY dstsChanged)
    Q_PROPERTY(int skinTime READ skinTime WRITE setSkinTime NOTIFY skinTimeChanged)
    Q_PROPERTY(QVariant timers READ timers WRITE setTimers NOTIFY timersChanged)
    Q_PROPERTY(int timerFire READ timerFire WRITE setTimerFire NOTIFY timerFireChanged)
    Q_PROPERTY(QObject* activeOptionsState READ activeOptionsState WRITE setActiveOptionsState NOTIFY activeOptionsStateChanged)
    Q_PROPERTY(QVariant activeOptions READ activeOptions WRITE setActiveOptions NOTIFY activeOptionsChanged)
    Q_PROPERTY(bool sliderTranslationEnabled READ sliderTranslationEnabled WRITE setSliderTranslationEnabled NOTIFY sliderTranslationChanged)
    Q_PROPERTY(qreal sliderPosition READ sliderPosition WRITE setSliderPosition NOTIFY sliderTranslationChanged)
    Q_PROPERTY(int sliderRange READ sliderRange WRITE setSliderRange NOTIFY sliderTranslationChanged)
    Q_PROPERTY(int sliderDirection READ sliderDirection WRITE setSliderDirection NOTIFY sliderTranslationChanged)
    Q_PROPERTY(bool dstOffsetsEnabled READ dstOffsetsEnabled WRITE setDstOffsetsEnabled NOTIFY dstOffsetsChanged)
    Q_PROPERTY(qreal dstOffsetLiftY READ dstOffsetLiftY WRITE setDstOffsetLiftY NOTIFY dstOffsetsChanged)
    Q_PROPERTY(qreal dstOffsetLaneCoverY READ dstOffsetLaneCoverY WRITE setDstOffsetLaneCoverY NOTIFY dstOffsetsChanged)
    Q_PROPERTY(qreal dstOffsetHiddenY READ dstOffsetHiddenY WRITE setDstOffsetHiddenY NOTIFY dstOffsetsChanged)
    Q_PROPERTY(qreal dstOffsetHiddenA READ dstOffsetHiddenA WRITE setDstOffsetHiddenA NOTIFY dstOffsetsChanged)
    Q_PROPERTY(QVariant state READ state NOTIFY stateChanged)
    Q_PROPERTY(bool hasState READ hasState NOTIFY stateChanged)
    Q_PROPERTY(qreal stateX READ stateX NOTIFY stateChanged)
    Q_PROPERTY(qreal stateY READ stateY NOTIFY stateChanged)
    Q_PROPERTY(qreal stateW READ stateW NOTIFY stateChanged)
    Q_PROPERTY(qreal stateH READ stateH NOTIFY stateChanged)
    Q_PROPERTY(qreal stateA READ stateA NOTIFY stateChanged)
    Q_PROPERTY(qreal stateR READ stateR NOTIFY stateChanged)
    Q_PROPERTY(qreal stateG READ stateG NOTIFY stateChanged)
    Q_PROPERTY(qreal stateB READ stateB NOTIFY stateChanged)
    Q_PROPERTY(qreal stateAngle READ stateAngle NOTIFY stateChanged)
    Q_PROPERTY(int stateCenter READ stateCenter NOTIFY stateChanged)
    Q_PROPERTY(qreal stateSortId READ stateSortId NOTIFY stateChanged)
    Q_PROPERTY(int stateBlend READ stateBlend NOTIFY stateChanged)
    Q_PROPERTY(int stateFilter READ stateFilter NOTIFY stateChanged)
    Q_PROPERTY(int stateOp1 READ stateOp1 NOTIFY stateChanged)
    Q_PROPERTY(int stateOp2 READ stateOp2 NOTIFY stateChanged)
    Q_PROPERTY(int stateOp3 READ stateOp3 NOTIFY stateChanged)
    Q_PROPERTY(int stateOp4 READ stateOp4 NOTIFY stateChanged)
    Q_PROPERTY(bool canUseStaticState READ canUseStaticState NOTIFY analysisChanged)
    Q_PROPERTY(QVariant staticState READ staticState NOTIFY analysisChanged)
    Q_PROPERTY(bool usesActiveOptions READ usesActiveOptions NOTIFY analysisChanged)
    Q_PROPERTY(bool usesDynamicTimer READ usesDynamicTimer NOTIFY analysisChanged)
    Q_PROPERTY(bool loopsContinuously READ loopsContinuously NOTIFY analysisChanged)
    Q_PROPERTY(int scratchRotationSide READ scratchRotationSide NOTIFY analysisChanged)
    Q_PROPERTY(int firstTimer READ firstTimer NOTIFY analysisChanged)
    Q_PROPERTY(int firstSortId READ firstSortId NOTIFY analysisChanged)

public:
    enum ClockMode {
        ManualClock = 0,
        RenderClock = 1,
        SelectSourceClock = 2,
        BarClock = 3,
        GlobalClock = 4,
        SelectLiveClock = 5,
        SelectInfoClock = 6
    };
    Q_ENUM(ClockMode)

    explicit Lr2TimelineState(QObject* parent = nullptr);

    QObject* skinClock() const;
    void setSkinClock(QObject* clock);

    int clockMode() const;
    void setClockMode(int mode);

    bool isEnabled() const;
    void setEnabled(bool enabled);

    QVariantList dsts() const;
    void setDsts(const QVariantList& dsts);

    int skinTime() const;
    void setSkinTime(int time);

    QVariant timers() const;
    void setTimers(const QVariant& timers);

    int timerFire() const;
    void setTimerFire(int timerFire);

    QObject* activeOptionsState() const;
    void setActiveOptionsState(QObject* state);

    QVariant activeOptions() const;
    void setActiveOptions(const QVariant& options);

    bool sliderTranslationEnabled() const;
    void setSliderTranslationEnabled(bool enabled);

    qreal sliderPosition() const;
    void setSliderPosition(qreal position);

    int sliderRange() const;
    void setSliderRange(int range);

    int sliderDirection() const;
    void setSliderDirection(int direction);

    bool dstOffsetsEnabled() const;
    void setDstOffsetsEnabled(bool enabled);

    qreal dstOffsetLiftY() const;
    void setDstOffsetLiftY(qreal value);

    qreal dstOffsetLaneCoverY() const;
    void setDstOffsetLaneCoverY(qreal value);

    qreal dstOffsetHiddenY() const;
    void setDstOffsetHiddenY(qreal value);

    qreal dstOffsetHiddenA() const;
    void setDstOffsetHiddenA(qreal value);

    QVariant state() const;
    bool hasState() const;
    qreal stateX() const;
    qreal stateY() const;
    qreal stateW() const;
    qreal stateH() const;
    qreal stateA() const;
    qreal stateR() const;
    qreal stateG() const;
    qreal stateB() const;
    qreal stateAngle() const;
    int stateCenter() const;
    qreal stateSortId() const;
    int stateBlend() const;
    int stateFilter() const;
    int stateOp1() const;
    int stateOp2() const;
    int stateOp3() const;
    int stateOp4() const;
    bool canUseStaticState() const;
    QVariant staticState() const;
    bool usesActiveOptions() const;
    bool usesDynamicTimer() const;
    bool loopsContinuously() const;
    int scratchRotationSide() const;
    int firstTimer() const;
    int firstSortId() const;

    Q_INVOKABLE QVariant stateFor(const QVariantList& dsts,
                                  int skinTime,
                                  const QVariant& timers,
                                  const QVariant& activeOptions) const;
    Q_INVOKABLE QVariant stateFromTimerFire(const QVariantList& dsts,
                                            int skinTime,
                                            int timerFire,
                                            const QVariant& activeOptions) const;
    Q_INVOKABLE QVariant staticStateFor(const QVariantList& dsts) const;
    Q_INVOKABLE bool canUseStaticStateFor(const QVariantList& dsts) const;
    Q_INVOKABLE bool usesActiveOptionsFor(const QVariantList& dsts) const;
    Q_INVOKABLE bool usesDynamicTimerFor(const QVariantList& dsts) const;
    Q_INVOKABLE bool loopsContinuouslyFor(const QVariantList& dsts) const;
    Q_INVOKABLE int scratchRotationSideFor(const QVariantList& dsts) const;
    Q_INVOKABLE int firstTimerFor(const QVariantList& dsts) const;
    Q_INVOKABLE int firstSortIdFor(const QVariantList& dsts) const;
    Q_INVOKABLE int timerFireFor(const QVariant& timers, int timerIdx) const;
    Q_INVOKABLE int sourceTimerFor(const QVariant& src) const;
    Q_INVOKABLE bool sourceUsesDynamicTimer(const QVariant& src) const;
    Q_INVOKABLE bool sourceCyclesContinuously(const QVariant& src) const;

signals:
    void skinClockChanged();
    void clockModeChanged();
    void enabledChanged();
    void dstsChanged();
    void skinTimeChanged();
    void timersChanged();
    void timerFireChanged();
    void activeOptionsStateChanged();
    void activeOptionsChanged();
    void sliderTranslationChanged();
    void dstOffsetsChanged();
    void stateChanged();
    void analysisChanged();

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
        QVector<int> offsets;
    };

    struct State {
        bool valid = false;
        qreal x = 0;
        qreal y = 0;
        qreal w = 0;
        qreal h = 0;
        qreal a = 255;
        qreal r = 255;
        qreal g = 255;
        qreal b = 255;
        qreal angle = 0;
        int center = 0;
        qreal sortId = 0;
        int blend = 0;
        int filter = 0;
        int op1 = 0;
        int op2 = 0;
        int op3 = 0;
        int op4 = 0;
    };

    struct Source {
        bool valid = false;
        int divX = 1;
        int divY = 1;
        int cycle = 0;
        int timer = 0;
    };

    struct DstAnalysis {
        bool canUseStaticState = false;
        bool usesActiveOptions = false;
        bool usesDynamicTimer = false;
        bool loopsContinuously = false;
        int scratchRotationSide = 0;
        int firstTimer = 0;
        int firstSortId = 0;
    };

    void rebuildDsts();
    void rebuildActiveOptionSet();
    void rebuildAnalysis();
    void reconnectClock();
    void reconnectActiveOptionsState();
    void activeOptionsStateDidChange();
    void updateSkinTimeFromClock();
    void updateAnimationLimit();
    void updateState();
    int clockSkinTime() const;
    int effectiveSkinTime(int requestedTime) const;
    qreal effectiveTimerFire() const;
    qreal timerValue(int timerIdx) const;
    bool allOpsMatch(const Dst& dst) const;
    bool checkSingleOp(int op) const;
    State translatedSliderState(State state) const;
    State offsetDstState(State state) const;
    void assignState(const State& state);

    static bool readDst(const QVariant& value, Dst& dst);
    static QVector<Dst> readDsts(const QVariantList& dsts);
    static bool readSource(const QVariant& value, Source& source);
    static QVector<int> readOffsets(const QVariant& value);
    static DstAnalysis analyzeDsts(const QVector<Dst>& dsts);
    static int animationLimitFor(const QVector<Dst>& dsts);
    static State currentState(const QVector<Dst>& dsts,
                              int globalTime,
                              qreal timerFire,
                              const Lr2TimelineState& cache);
    static State copyDstAsState(const Dst& dst, const Dst& controlDst);
    static QVariant stateToVariant(const State& state);
    static qreal applyAccel(qreal progress, int accType);
    static bool sameState(const State& lhs, const State& rhs);

    bool m_enabled = true;
    QPointer<Lr2SkinClock> m_skinClock;
    QMetaObject::Connection m_clockConnection;
    int m_clockMode = ManualClock;
    QVariantList m_dstsValue;
    QVector<Dst> m_dsts;
    int m_requestedSkinTime = 0;
    int m_effectiveSkinTime = 0;
    QVariant m_timers;
    int m_timerFire = -2147483648;
    QPointer<QObject> m_activeOptionsState;
    QMetaObject::Connection m_activeOptionsStateConnection;
    QVariant m_activeOptions;
    QSet<int> m_activeOptionSet;
    bool m_sliderTranslationEnabled = false;
    qreal m_sliderPosition = 0.0;
    int m_sliderRange = 0;
    int m_sliderDirection = 0;
    bool m_dstOffsetsEnabled = false;
    qreal m_dstOffsetLiftY = 0.0;
    qreal m_dstOffsetLaneCoverY = 0.0;
    qreal m_dstOffsetHiddenY = 0.0;
    qreal m_dstOffsetHiddenA = 0.0;
    State m_state;
    int m_animationLimit = -1;
    bool m_canUseStaticState = false;
    bool m_usesActiveOptions = false;
    bool m_usesDynamicTimer = false;
    bool m_loopsContinuously = false;
    int m_scratchRotationSide = 0;
    int m_firstTimer = 0;
    int m_firstSortId = 0;
};
