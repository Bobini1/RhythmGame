#pragma once

#include <QObject>

class Lr2SkinElementTimerState : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool dstTimerCanFire READ dstTimerCanFire NOTIFY dstTimerCanFireChanged)
    Q_PROPERTY(bool srcTimerCanFire READ srcTimerCanFire NOTIFY srcTimerCanFireChanged)
    Q_PROPERTY(int dstTimerFire READ dstTimerFire NOTIFY dstTimerFireChanged)
    Q_PROPERTY(int srcTimerFire READ srcTimerFire NOTIFY srcTimerFireChanged)

public:
    explicit Lr2SkinElementTimerState(QObject* parent = nullptr);

    bool dstTimerCanFire() const;
    bool srcTimerCanFire() const;
    int dstTimerFire() const;
    int srcTimerFire() const;

    void setSnapshot(bool dstCanFire, int dstFire, bool srcCanFire, int srcFire);

signals:
    void dstTimerCanFireChanged();
    void srcTimerCanFireChanged();
    void dstTimerFireChanged();
    void srcTimerFireChanged();

private:
    bool m_dstTimerCanFire = false;
    bool m_srcTimerCanFire = false;
    int m_dstTimerFire = -1;
    int m_srcTimerFire = -1;
};
