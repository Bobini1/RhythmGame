#pragma once

#include "Lr2GameplayFrameState.h"
#include "Lr2SelectVisualState.h"
#include "Lr2SkinClock.h"

#include <QObject>
#include <QPointer>
#include <QtQml/qqmlregistration.h>

class Lr2SkinFrameDriver : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(Lr2SkinClock* clock READ clock WRITE setClock NOTIFY clockChanged)
    Q_PROPERTY(Lr2GameplayFrameState* gameplayFrameState READ gameplayFrameState WRITE setGameplayFrameState NOTIFY gameplayFrameStateChanged)
    Q_PROPERTY(Lr2SelectVisualState* selectVisualState READ selectVisualState WRITE setSelectVisualState NOTIFY selectVisualStateChanged)
    Q_PROPERTY(bool gameplayScreen READ gameplayScreen WRITE setGameplayScreen NOTIFY gameplayScreenChanged)
    Q_PROPERTY(bool gameplayStartupPending READ gameplayStartupPending WRITE setGameplayStartupPending NOTIFY gameplayStartupPendingChanged)
    Q_PROPERTY(int currentFps READ currentFps NOTIFY currentFpsChanged)
    Q_PROPERTY(int fpsSampleIntervalMs READ fpsSampleIntervalMs WRITE setFpsSampleIntervalMs NOTIFY fpsSampleIntervalMsChanged)

public:
    explicit Lr2SkinFrameDriver(QObject* parent = nullptr);

    Lr2SkinClock* clock() const;
    void setClock(Lr2SkinClock* clock);

    Lr2GameplayFrameState* gameplayFrameState() const;
    void setGameplayFrameState(Lr2GameplayFrameState* state);

    Lr2SelectVisualState* selectVisualState() const;
    void setSelectVisualState(Lr2SelectVisualState* state);

    bool gameplayScreen() const;
    void setGameplayScreen(bool value);

    bool gameplayStartupPending() const;
    void setGameplayStartupPending(bool value);

    int currentFps() const;

    int fpsSampleIntervalMs() const;
    void setFpsSampleIntervalMs(int value);

    Q_INVOKABLE void tick(qreal smoothFrameTime);

signals:
    void clockChanged();
    void gameplayFrameStateChanged();
    void selectVisualStateChanged();
    void gameplayScreenChanged();
    void gameplayStartupPendingChanged();
    void currentFpsChanged();
    void fpsSampleIntervalMsChanged();
    void gameplayStartupTickRequested();

private:
    void setCurrentFps(int value);

    QPointer<Lr2SkinClock> m_clock;
    QPointer<Lr2GameplayFrameState> m_gameplayFrameState;
    QPointer<Lr2SelectVisualState> m_selectVisualState;
    bool m_gameplayScreen = false;
    bool m_gameplayStartupPending = false;
    int m_currentFps = 0;
    int m_fpsSampleIntervalMs = 250;
    qint64 m_lastFpsSampleMs = 0;
};
