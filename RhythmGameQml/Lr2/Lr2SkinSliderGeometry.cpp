#include "Lr2SkinSliderGeometry.h"

#include "Lr2SkinRuntimeTypes.h"

#include <cmath>

namespace {
bool readSliderSource(const QVariant& value, lr2skin::runtime::Source& source) {
    return lr2skin::runtime::readSource(value, source) && source.slider;
}

} // namespace

Lr2SkinSliderGeometry::Lr2SkinSliderGeometry(QObject* parent) : QObject(parent) {}

bool Lr2SkinSliderGeometry::isSelectScrollSlider(const QString& screenKey, const QVariant& src) const {
    lr2skin::runtime::Source source;
    return screenKey == QStringLiteral("select")
        && readSliderSource(src, source)
        && source.sliderType == 1
        && source.sliderRange > 0
        && source.sliderDisabled == 0;
}

bool Lr2SkinSliderGeometry::isGenericSlider(const QString& screenKey, const QVariant& src) const {
    lr2skin::runtime::Source source;
    return screenKey == QStringLiteral("select")
        && readSliderSource(src, source)
        && !(source.sliderType == 1 && source.sliderRange > 0 && source.sliderDisabled == 0)
        && source.sliderRange > 0
        && source.sliderDisabled == 0;
}

bool Lr2SkinSliderGeometry::isGameplayProgressSlider(bool gameplayScreen, const QVariant& src) const {
    lr2skin::runtime::Source source;
    return gameplayScreen
        && readSliderSource(src, source)
        && source.sliderType == 6
        && source.sliderRange > 0;
}

bool Lr2SkinSliderGeometry::isGameplayLaneCoverSlider(bool gameplayScreen, const QVariant& src) const {
    lr2skin::runtime::Source source;
    return gameplayScreen
        && readSliderSource(src, source)
        && (source.sliderType == 4 || source.sliderType == 5)
        && source.sliderRange > 0;
}

bool Lr2SkinSliderGeometry::isNumberRefSlider(const QVariant& src) const {
    lr2skin::runtime::Source source;
    return readSliderSource(src, source)
        && source.sliderRefNumber
        && source.sliderRange > 0;
}

Lr2TimelineStateValue Lr2SkinSliderGeometry::trackState(const QVariant& src, Lr2TimelineStateValue baseState) const {
    lr2skin::runtime::Source source;
    return readSliderSource(src, source)
        ? lr2skin::runtime::sliderTrackState(source, baseState)
        : Lr2TimelineStateValue {};
}

Lr2TimelineStateValue Lr2SkinSliderGeometry::translatedState(const QVariant& src,
                                                             Lr2TimelineStateValue baseState,
                                                             qreal position) const {
    lr2skin::runtime::Source source;
    if (!readSliderSource(src, source) || !baseState.valid) {
        return {};
    }

    if (!std::isfinite(position)) {
        position = 0.0;
    }
    return lr2skin::runtime::translatedSliderState(
        baseState,
        position,
        source.sliderRange,
        source.sliderDirection);
}

qreal Lr2SkinSliderGeometry::positionFromPointer(const QVariant& src,
                                                 Lr2TimelineStateValue track,
                                                 qreal pointerX,
                                                 qreal pointerY) const {
    lr2skin::runtime::Source source;
    if (!readSliderSource(src, source) || !track.valid) {
        return 0.0;
    }
    return lr2skin::runtime::sliderPositionFromPointer(source, track, pointerX, pointerY);
}
