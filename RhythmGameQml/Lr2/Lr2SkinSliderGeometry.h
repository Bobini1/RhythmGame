#pragma once

#include <QObject>
#include <QVariant>
#include <QtQml/qqmlregistration.h>

class Lr2SkinSliderGeometry : public QObject {
    Q_OBJECT
    QML_ELEMENT

public:
    explicit Lr2SkinSliderGeometry(QObject* parent = nullptr);

    Q_INVOKABLE bool isSelectScrollSlider(const QString& screenKey, const QVariant& src) const;
    Q_INVOKABLE bool isGenericSlider(const QString& screenKey, const QVariant& src) const;
    Q_INVOKABLE bool isGameplayProgressSlider(bool gameplayScreen, const QVariant& src) const;
    Q_INVOKABLE bool isGameplayLaneCoverSlider(bool gameplayScreen, const QVariant& src) const;
    Q_INVOKABLE bool isNumberRefSlider(const QVariant& src) const;
    Q_INVOKABLE QVariant trackState(const QVariant& src, const QVariant& baseState) const;
    Q_INVOKABLE QVariant translatedState(const QVariant& src,
                                         const QVariant& baseState,
                                         qreal position) const;
    Q_INVOKABLE qreal positionFromPointer(const QVariant& src,
                                          const QVariant& track,
                                          qreal pointerX,
                                          qreal pointerY) const;

private:
    struct Source {
        bool valid = false;
        bool slider = false;
        int sliderDirection = 0;
        int sliderRange = 0;
        int sliderType = 0;
        int sliderDisabled = 0;
        bool sliderRefNumber = false;
    };

    struct State {
        bool valid = false;
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
        qreal sortId = 0.0;
        int blend = 0;
        int filter = 0;
        int op1 = 0;
        int op2 = 0;
        int op3 = 0;
        int op4 = 0;
    };

    static bool readSource(const QVariant& value, Source& source);
    static bool readState(const QVariant& value, State& state);
    static QVariant stateToVariant(const State& state);
};
