#pragma once

#include <QObject>
#include <QMetaObject>
#include <QPointer>
#include <QVariant>
#include <QVector4D>
#include <QtQml/qqmlregistration.h>

class Lr2SkinClock;

class Lr2AnimationFrameState : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QObject* skinClock READ skinClock WRITE setSkinClock NOTIFY skinClockChanged)
    Q_PROPERTY(int clockMode READ clockMode WRITE setClockMode NOTIFY clockModeChanged)
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(QVariant sourceData READ sourceData WRITE setSourceData NOTIFY sourceDataChanged)
    Q_PROPERTY(int skinTime READ skinTime WRITE setSkinTime NOTIFY skinTimeChanged)
    Q_PROPERTY(QVariant timers READ timers WRITE setTimers NOTIFY timersChanged)
    Q_PROPERTY(int timerFire READ timerFire WRITE setTimerFire NOTIFY timerFireChanged)
    Q_PROPERTY(int frameOverride READ frameOverride WRITE setFrameOverride NOTIFY frameOverrideChanged)
    Q_PROPERTY(int textureWidth READ textureWidth WRITE setTextureWidth NOTIFY textureWidthChanged)
    Q_PROPERTY(int textureHeight READ textureHeight WRITE setTextureHeight NOTIFY textureHeightChanged)
    Q_PROPERTY(int frameIndex READ frameIndex NOTIFY frameIndexChanged)
    Q_PROPERTY(QVector4D sourceRect READ sourceRect NOTIFY sourceRectChanged)

public:
    enum ClockMode {
        ManualClock = 0,
        RenderClock = 1,
        SelectSourceClock = 2,
        BarClock = 3,
        GlobalClock = 4,
        SelectLiveClock = 5
    };
    Q_ENUM(ClockMode)

    explicit Lr2AnimationFrameState(QObject* parent = nullptr);

    QObject* skinClock() const;
    void setSkinClock(QObject* clock);

    int clockMode() const;
    void setClockMode(int mode);

    bool isEnabled() const;
    void setEnabled(bool enabled);

    QVariant sourceData() const;
    void setSourceData(const QVariant& sourceData);

    int skinTime() const;
    void setSkinTime(int skinTime);

    QVariant timers() const;
    void setTimers(const QVariant& timers);

    int timerFire() const;
    void setTimerFire(int timerFire);

    int frameOverride() const;
    void setFrameOverride(int frameOverride);

    int textureWidth() const;
    void setTextureWidth(int textureWidth);

    int textureHeight() const;
    void setTextureHeight(int textureHeight);

    int frameIndex() const;
    QVector4D sourceRect() const;

signals:
    void skinClockChanged();
    void clockModeChanged();
    void enabledChanged();
    void sourceDataChanged();
    void skinTimeChanged();
    void timersChanged();
    void timerFireChanged();
    void frameOverrideChanged();
    void textureWidthChanged();
    void textureHeightChanged();
    void frameIndexChanged();
    void sourceRectChanged();

private:
    struct Source {
        bool valid = false;
        int divX = 1;
        int divY = 1;
        int cycle = 0;
        int timer = 0;
        int x = 0;
        int y = 0;
        int w = 0;
        int h = 0;
    };

    void rebuildSource();
    void reconnectClock();
    void updateSkinTimeFromClock();
    void updateFrameIndex();
    int clockSkinTime() const;
    qreal effectiveTimerFire() const;
    qreal timerValue(int timerIdx) const;
    static bool readSource(const QVariant& value, Source& source);
    static int frameCount(const Source& source);
    static QVector4D sourceRectFor(const Source& source, int frameIndex, int textureWidth, int textureHeight);

    bool m_enabled = true;
    QPointer<Lr2SkinClock> m_skinClock;
    QMetaObject::Connection m_clockConnection;
    int m_clockMode = ManualClock;
    QVariant m_sourceData;
    Source m_source;
    int m_skinTime = 0;
    QVariant m_timers;
    int m_timerFire = -2147483648;
    int m_frameOverride = -1;
    int m_textureWidth = 0;
    int m_textureHeight = 0;
    int m_frameIndex = 0;
    QVector4D m_sourceRect = QVector4D(0.0f, 0.0f, 1.0f, 1.0f);
};
