#pragma once

#include "Lr2TimelineStateValue.h"

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
    Q_INVOKABLE Lr2TimelineStateValue trackState(const QVariant& src, Lr2TimelineStateValue baseState) const;
    Q_INVOKABLE Lr2TimelineStateValue translatedState(const QVariant& src,
                                                       Lr2TimelineStateValue baseState,
                                                       qreal position) const;
    Q_INVOKABLE qreal positionFromPointer(const QVariant& src,
                                          Lr2TimelineStateValue track,
                                          qreal pointerX,
                                          qreal pointerY) const;
};
