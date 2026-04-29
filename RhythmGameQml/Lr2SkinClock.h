#pragma once

#include <QObject>
#include <QString>
#include <QtQml/qqmlregistration.h>

class Lr2SkinClock : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(qreal nowMs READ nowMs WRITE setNowMs NOTIFY nowMsChanged)
    Q_PROPERTY(qreal sceneStartMs READ sceneStartMs WRITE setSceneStartMs NOTIFY sceneStartMsChanged)
    Q_PROPERTY(int resolutionMs READ resolutionMs WRITE setResolutionMs NOTIFY resolutionMsChanged)
    Q_PROPERTY(QString screenKey READ screenKey WRITE setScreenKey NOTIFY screenKeyChanged)
    Q_PROPERTY(int selectAnimationLimit READ selectAnimationLimit WRITE setSelectAnimationLimit NOTIFY selectAnimationLimitChanged)
    Q_PROPERTY(int barAnimationLimit READ barAnimationLimit WRITE setBarAnimationLimit NOTIFY barAnimationLimitChanged)
    Q_PROPERTY(int selectInfoAnimationLimit READ selectInfoAnimationLimit WRITE setSelectInfoAnimationLimit NOTIFY selectInfoAnimationLimitChanged)
    Q_PROPERTY(int selectInfoStartSkinTime READ selectInfoStartSkinTime WRITE setSelectInfoStartSkinTime NOTIFY selectInfoStartSkinTimeChanged)
    Q_PROPERTY(int globalSkinTime READ globalSkinTime NOTIFY globalSkinTimeChanged)
    Q_PROPERTY(int selectLiveSkinTime READ selectLiveSkinTime NOTIFY selectLiveSkinTimeChanged)
    Q_PROPERTY(int selectSourceSkinTime READ selectSourceSkinTime NOTIFY selectSourceSkinTimeChanged)
    Q_PROPERTY(int selectInfoElapsed READ selectInfoElapsed NOTIFY selectInfoElapsedChanged)
    Q_PROPERTY(int renderSkinTime READ renderSkinTime NOTIFY renderSkinTimeChanged)
    Q_PROPERTY(int barSkinTime READ barSkinTime NOTIFY barSkinTimeChanged)

public:
    explicit Lr2SkinClock(QObject* parent = nullptr);

    qreal nowMs() const;
    void setNowMs(qreal value);

    qreal sceneStartMs() const;
    void setSceneStartMs(qreal value);

    int resolutionMs() const;
    void setResolutionMs(int value);

    QString screenKey() const;
    void setScreenKey(const QString& value);

    int selectAnimationLimit() const;
    void setSelectAnimationLimit(int value);

    int barAnimationLimit() const;
    void setBarAnimationLimit(int value);

    int selectInfoAnimationLimit() const;
    void setSelectInfoAnimationLimit(int value);

    int selectInfoStartSkinTime() const;
    void setSelectInfoStartSkinTime(int value);

    int globalSkinTime() const;
    int selectLiveSkinTime() const;
    int selectSourceSkinTime() const;
    int selectInfoElapsed() const;
    int renderSkinTime() const;
    int barSkinTime() const;

    Q_INVOKABLE qreal quantize(qreal now) const;
    Q_INVOKABLE void advance(qreal now);
    Q_INVOKABLE void restart(qreal now);

signals:
    void nowMsChanged();
    void sceneStartMsChanged();
    void resolutionMsChanged();
    void screenKeyChanged();
    void selectAnimationLimitChanged();
    void barAnimationLimitChanged();
    void selectInfoAnimationLimitChanged();
    void selectInfoStartSkinTimeChanged();
    void globalSkinTimeChanged();
    void selectLiveSkinTimeChanged();
    void selectSourceSkinTimeChanged();
    void selectInfoElapsedChanged();
    void renderSkinTimeChanged();
    void barSkinTimeChanged();

private:
    void recompute();
    static bool sameReal(qreal lhs, qreal rhs);

    qreal m_nowMs = 0.0;
    qreal m_sceneStartMs = 0.0;
    int m_resolutionMs = 10;
    QString m_screenKey;
    int m_selectAnimationLimit = 3200;
    int m_barAnimationLimit = 2200;
    int m_selectInfoAnimationLimit = 1000;
    int m_selectInfoStartSkinTime = 0;
    int m_globalSkinTime = 0;
    int m_selectLiveSkinTime = 0;
    int m_selectSourceSkinTime = 0;
    int m_selectInfoElapsed = 0;
    int m_renderSkinTime = 0;
    int m_barSkinTime = 0;
};
