#pragma once

#include "Lr2TimelineState.h"
#include "Lr2TimelineStateValue.h"

#include <QColor>
#include <QObject>
#include <QVariant>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

class Lr2SkinClock;
class Lr2SkinElementActiveOptionsState;

class Lr2TimelineFrameState : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QVariantList dsts READ dsts WRITE setDsts NOTIFY dstsChanged)
    Q_PROPERTY(int skinTime READ skinTime WRITE setSkinTime NOTIFY skinTimeChanged)
    Q_PROPERTY(Lr2SkinClock* skinClock READ skinClock WRITE setSkinClock NOTIFY skinClockChanged)
    Q_PROPERTY(int skinClockMode READ skinClockMode WRITE setSkinClockMode NOTIFY skinClockModeChanged)
    Q_PROPERTY(Lr2SkinElementActiveOptionsState* activeOptionsState READ activeOptionsState WRITE setActiveOptionsState NOTIFY activeOptionsStateChanged)
    Q_PROPERTY(QVariant activeOptions READ activeOptions WRITE setActiveOptions NOTIFY activeOptionsChanged)
    Q_PROPERTY(QVariant timers READ timers WRITE setTimers NOTIFY timersChanged)
    Q_PROPERTY(int timerFire READ timerFire WRITE setTimerFire NOTIFY timerFireChanged)
    Q_PROPERTY(bool stateOverrideEnabled READ stateOverrideEnabled WRITE setStateOverrideEnabled NOTIFY stateOverrideChanged)
    Q_PROPERTY(Lr2TimelineStateValue stateOverrideValue READ stateOverrideValue WRITE setStateOverrideValue NOTIFY stateOverrideChanged)
    Q_PROPERTY(bool forceHidden READ forceHidden WRITE setForceHidden NOTIFY forceHiddenChanged)
    Q_PROPERTY(bool sliderTranslationEnabled READ sliderTranslationEnabled WRITE setSliderTranslationEnabled NOTIFY sliderTranslationChanged)
    Q_PROPERTY(qreal sliderPosition READ sliderPosition WRITE setSliderPosition NOTIFY sliderTranslationChanged)
    Q_PROPERTY(int sliderRange READ sliderRange WRITE setSliderRange NOTIFY sliderTranslationChanged)
    Q_PROPERTY(int sliderDirection READ sliderDirection WRITE setSliderDirection NOTIFY sliderTranslationChanged)
    Q_PROPERTY(bool dstOffsetsEnabled READ dstOffsetsEnabled WRITE setDstOffsetsEnabled NOTIFY dstOffsetsChanged)
    Q_PROPERTY(qreal dstOffsetLiftY READ dstOffsetLiftY WRITE setDstOffsetLiftY NOTIFY dstOffsetsChanged)
    Q_PROPERTY(qreal dstOffsetLaneCoverY READ dstOffsetLaneCoverY WRITE setDstOffsetLaneCoverY NOTIFY dstOffsetsChanged)
    Q_PROPERTY(qreal dstOffsetHiddenY READ dstOffsetHiddenY WRITE setDstOffsetHiddenY NOTIFY dstOffsetsChanged)
    Q_PROPERTY(qreal dstOffsetHiddenA READ dstOffsetHiddenA WRITE setDstOffsetHiddenA NOTIFY dstOffsetsChanged)
    Q_PROPERTY(bool colorKeyEnabled READ colorKeyEnabled WRITE setColorKeyEnabled NOTIFY colorKeyEnabledChanged)
    Q_PROPERTY(bool supportsInvertedBlend READ supportsInvertedBlend WRITE setSupportsInvertedBlend NOTIFY supportsInvertedBlendChanged)
    Q_PROPERTY(bool canUseStaticState READ canUseStaticState NOTIFY staticStateChanged)
    Q_PROPERTY(Lr2TimelineStateValue staticState READ staticState NOTIFY staticStateChanged)
    Q_PROPERTY(QVariant timelineTimers READ timelineTimers NOTIFY timelineTimersChanged)
    Q_PROPERTY(Lr2TimelineStateValue directState READ directState NOTIFY directStateChanged)
    Q_PROPERTY(bool hasDirectState READ hasDirectState NOTIFY directStateChanged)
    Q_PROPERTY(bool hasTimelineState READ hasTimelineState NOTIFY hasTimelineStateChanged)
    Q_PROPERTY(Lr2TimelineStateValue state READ state NOTIFY directStateChanged)
    Q_PROPERTY(bool hasState READ hasState NOTIFY hasStateChanged)
    Q_PROPERTY(qreal x READ x NOTIFY geometryChanged)
    Q_PROPERTY(qreal y READ y NOTIFY geometryChanged)
    Q_PROPERTY(qreal w READ w NOTIFY geometryChanged)
    Q_PROPERTY(qreal h READ h NOTIFY geometryChanged)
    Q_PROPERTY(qreal a READ a NOTIFY alphaChanged)
    Q_PROPERTY(qreal r READ r NOTIFY colorChanged)
    Q_PROPERTY(qreal g READ g NOTIFY colorChanged)
    Q_PROPERTY(qreal b READ b NOTIFY colorChanged)
    Q_PROPERTY(qreal angle READ angle NOTIFY geometryChanged)
    Q_PROPERTY(int center READ center NOTIFY geometryChanged)
    Q_PROPERTY(int blend READ blend NOTIFY blendChanged)
    Q_PROPERTY(int filter READ filter NOTIFY filterChanged)
    Q_PROPERTY(int op4 READ op4 NOTIFY op4Changed)
    Q_PROPERTY(int rawBlendMode READ rawBlendMode NOTIFY blendChanged)
    Q_PROPERTY(int blendMode READ blendMode NOTIFY blendChanged)
    Q_PROPERTY(qreal tintR READ tintR NOTIFY colorChanged)
    Q_PROPERTY(qreal tintG READ tintG NOTIFY colorChanged)
    Q_PROPERTY(qreal tintB READ tintB NOTIFY colorChanged)
    Q_PROPERTY(bool hasColorTint READ hasColorTint NOTIFY colorChanged)
    Q_PROPERTY(QColor tintColor READ tintColor NOTIFY colorChanged)
    Q_PROPERTY(qreal opacity READ opacity NOTIFY alphaChanged)
    Q_PROPERTY(Lr2TimelineState* timelineState READ timelineState CONSTANT)

public:
    explicit Lr2TimelineFrameState(QObject* parent = nullptr);

    QVariantList dsts() const;
    void setDsts(const QVariantList& dsts);
    int skinTime() const;
    void setSkinTime(int skinTime);
    Lr2SkinClock* skinClock() const;
    void setSkinClock(Lr2SkinClock* skinClock);
    int skinClockMode() const;
    void setSkinClockMode(int skinClockMode);
    Lr2SkinElementActiveOptionsState* activeOptionsState() const;
    void setActiveOptionsState(Lr2SkinElementActiveOptionsState* activeOptionsState);
    QVariant activeOptions() const;
    void setActiveOptions(const QVariant& activeOptions);
    QVariant timers() const;
    void setTimers(const QVariant& timers);
    int timerFire() const;
    void setTimerFire(int timerFire);
    bool stateOverrideEnabled() const;
    void setStateOverrideEnabled(bool enabled);
    Lr2TimelineStateValue stateOverrideValue() const;
    void setStateOverrideValue(const Lr2TimelineStateValue& stateOverride);
    bool forceHidden() const;
    void setForceHidden(bool forceHidden);
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
    bool colorKeyEnabled() const;
    void setColorKeyEnabled(bool enabled);
    bool supportsInvertedBlend() const;
    void setSupportsInvertedBlend(bool supported);

    bool canUseStaticState() const;
    Lr2TimelineStateValue staticState() const;
    QVariant timelineTimers() const;
    Lr2TimelineStateValue directState() const;
    bool hasDirectState() const;
    bool hasTimelineState() const;
    Lr2TimelineStateValue state() const;
    bool hasState() const;
    qreal x() const;
    qreal y() const;
    qreal w() const;
    qreal h() const;
    qreal a() const;
    qreal r() const;
    qreal g() const;
    qreal b() const;
    qreal angle() const;
    int center() const;
    int blend() const;
    int filter() const;
    int op4() const;
    int rawBlendMode() const;
    int blendMode() const;
    qreal tintR() const;
    qreal tintG() const;
    qreal tintB() const;
    bool hasColorTint() const;
    QColor tintColor() const;
    qreal opacity() const;
    Lr2TimelineState* timelineState();

signals:
    void dstsChanged();
    void skinTimeChanged();
    void skinClockChanged();
    void skinClockModeChanged();
    void activeOptionsStateChanged();
    void activeOptionsChanged();
    void timersChanged();
    void timerFireChanged();
    void stateOverrideChanged();
    void forceHiddenChanged();
    void sliderTranslationChanged();
    void dstOffsetsChanged();
    void colorKeyEnabledChanged();
    void supportsInvertedBlendChanged();
    void staticStateChanged();
    void timelineTimersChanged();
    void directStateChanged();
    void hasTimelineStateChanged();
    void hasStateChanged();
    void geometryChanged();
    void alphaChanged();
    void colorChanged();
    void blendChanged();
    void filterChanged();
    void op4Changed();

private:
    struct StateFields {
        qreal x = 0.0;
        qreal y = 0.0;
        qreal w = 0.0;
        qreal h = 0.0;
        qreal a = 255.0;
        qreal r = 255.0;
        qreal g = 255.0;
        qreal b = 255.0;
        qreal angle = 0.0;
        int center = 0;
        int blend = 0;
        int filter = 0;
        int op4 = 0;
    };

    static bool sameReal(qreal left, qreal right);
    static bool sameStateValue(const Lr2TimelineStateValue& left, const Lr2TimelineStateValue& right);
    static StateFields fieldsFromState(const Lr2TimelineStateValue& state);
    static qreal clampedTint(qreal value);

    void updateTimelineConfiguration();
    void updateTimelineTimers();
    int normalizedBlendMode(int rawBlendMode) const;

private slots:
    void updateFrame();

private:
    Lr2TimelineState m_timeline;
    QVariantList m_dsts;
    int m_skinTime = 0;
    Lr2SkinClock* m_skinClock = nullptr;
    int m_skinClockMode = 0;
    Lr2SkinElementActiveOptionsState* m_activeOptionsState = nullptr;
    QVariant m_activeOptions = QVariantList {};
    QVariant m_timers = QVariantMap { { QStringLiteral("0"), 0 } };
    int m_timerFire = -2147483648;
    bool m_stateOverrideEnabled = false;
    Lr2TimelineStateValue m_stateOverrideValue;
    bool m_forceHidden = false;
    bool m_sliderTranslationEnabled = false;
    qreal m_sliderPosition = 0.0;
    int m_sliderRange = 0;
    int m_sliderDirection = 0;
    bool m_dstOffsetsEnabled = false;
    qreal m_dstOffsetLiftY = 0.0;
    qreal m_dstOffsetLaneCoverY = 0.0;
    qreal m_dstOffsetHiddenY = 0.0;
    qreal m_dstOffsetHiddenA = 0.0;
    bool m_colorKeyEnabled = false;
    bool m_supportsInvertedBlend = true;

    bool m_canUseStaticState = false;
    bool m_staticStateValid = false;
    Lr2TimelineStateValue m_staticState;
    QVariant m_timelineTimers;
    Lr2TimelineStateValue m_directState;
    bool m_hasDirectState = false;
    bool m_hasTimelineState = false;
    bool m_hasState = false;
    StateFields m_fields;
    int m_rawBlendMode = 1;
    int m_blendMode = 1;
    qreal m_tintR = 1.0;
    qreal m_tintG = 1.0;
    qreal m_tintB = 1.0;
    bool m_hasColorTint = false;
    QColor m_tintColor = QColor::fromRgbF(1.0, 1.0, 1.0, 1.0);
    qreal m_opacity = 0.0;
};
