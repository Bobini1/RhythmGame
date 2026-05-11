#include "Lr2SkinSliderGeometry.h"

#include "gameplay_logic/lr2_skin/Lr2SkinParser.h"

#include <QJSValue>
#include <QVariantMap>

#include <algorithm>
#include <cmath>

using gameplay_logic::lr2_skin::Lr2SrcImage;

namespace {
int mapInt(const QVariantMap& map, const QString& name, int fallback) {
    const auto it = map.constFind(name);
    return it == map.constEnd() || !it->isValid() || it->isNull()
        ? fallback
        : it->toInt();
}

qreal mapReal(const QVariantMap& map, const QString& name, qreal fallback) {
    const auto it = map.constFind(name);
    return it == map.constEnd() || !it->isValid() || it->isNull()
        ? fallback
        : it->toDouble();
}

bool mapBool(const QVariantMap& map, const QString& name, bool fallback) {
    const auto it = map.constFind(name);
    return it == map.constEnd() || !it->isValid() || it->isNull()
        ? fallback
        : it->toBool();
}

int jsInt(const QJSValue& value, const QString& name, int fallback) {
    const QJSValue field = value.property(name);
    return field.isUndefined() || field.isNull() ? fallback : field.toInt();
}

qreal jsReal(const QJSValue& value, const QString& name, qreal fallback) {
    const QJSValue field = value.property(name);
    return field.isUndefined() || field.isNull() ? fallback : field.toNumber();
}

bool jsBool(const QJSValue& value, const QString& name, bool fallback) {
    const QJSValue field = value.property(name);
    return field.isUndefined() || field.isNull() ? fallback : field.toBool();
}
} // namespace

Lr2SkinSliderGeometry::Lr2SkinSliderGeometry(QObject* parent) : QObject(parent) {}

bool Lr2SkinSliderGeometry::isSelectScrollSlider(const QString& screenKey, const QVariant& src) const {
    Source source;
    return screenKey == QStringLiteral("select")
        && readSource(src, source)
        && source.slider
        && source.sliderType == 1
        && source.sliderRange > 0
        && source.sliderDisabled == 0;
}

bool Lr2SkinSliderGeometry::isGenericSlider(const QString& screenKey, const QVariant& src) const {
    Source source;
    return screenKey == QStringLiteral("select")
        && readSource(src, source)
        && source.slider
        && !(source.sliderType == 1 && source.sliderRange > 0 && source.sliderDisabled == 0)
        && source.sliderRange > 0
        && source.sliderDisabled == 0;
}

bool Lr2SkinSliderGeometry::isGameplayProgressSlider(bool gameplayScreen, const QVariant& src) const {
    Source source;
    return gameplayScreen
        && readSource(src, source)
        && source.slider
        && source.sliderType == 6
        && source.sliderRange > 0;
}

bool Lr2SkinSliderGeometry::isGameplayLaneCoverSlider(bool gameplayScreen, const QVariant& src) const {
    Source source;
    return gameplayScreen
        && readSource(src, source)
        && source.slider
        && (source.sliderType == 4 || source.sliderType == 5)
        && source.sliderRange > 0;
}

bool Lr2SkinSliderGeometry::isNumberRefSlider(const QVariant& src) const {
    Source source;
    return readSource(src, source)
        && source.slider
        && source.sliderRefNumber
        && source.sliderRange > 0;
}

QVariant Lr2SkinSliderGeometry::trackState(const QVariant& src, const QVariant& baseState) const {
    Source source;
    State track;
    if (!readSource(src, source) || !source.slider || !readState(baseState, track)) {
        return {};
    }

    const qreal range = std::max(1, source.sliderRange);
    switch (source.sliderDirection) {
    case 0:
        track.y -= range;
        track.h += range;
        break;
    case 1:
        track.w += range;
        break;
    case 2:
        track.h += range;
        break;
    case 3:
        track.x -= range;
        track.w += range;
        break;
    default:
        return {};
    }

    QVariantMap value;
    value.insert(QStringLiteral("x"), track.x);
    value.insert(QStringLiteral("y"), track.y);
    value.insert(QStringLiteral("w"), track.w);
    value.insert(QStringLiteral("h"), track.h);
    return value;
}

QVariant Lr2SkinSliderGeometry::translatedState(const QVariant& src,
                                                const QVariant& baseState,
                                                qreal position) const {
    Source source;
    State state;
    if (!readSource(src, source) || !source.slider || !readState(baseState, state)) {
        return {};
    }

    if (!std::isfinite(position)) {
        position = 0.0;
    }
    const qreal sliderOffset = position * std::max(1, source.sliderRange);
    switch (source.sliderDirection) {
    case 0:
        state.y -= sliderOffset;
        break;
    case 1:
        state.x += sliderOffset;
        break;
    case 2:
        state.y += sliderOffset;
        break;
    case 3:
        state.x -= sliderOffset;
        break;
    default:
        return {};
    }
    return stateToVariant(state);
}

qreal Lr2SkinSliderGeometry::positionFromPointer(const QVariant& src,
                                                 const QVariant& track,
                                                 qreal pointerX,
                                                 qreal pointerY) const {
    Source source;
    State state;
    if (!readSource(src, source) || !source.slider || !readState(track, state)) {
        return 0.0;
    }

    const qreal range = std::max(1, source.sliderRange);
    const qreal moveX = (state.w - range) / 2.0;
    const qreal moveY = (state.h - range) / 2.0;
    qreal position = 0.0;
    switch (source.sliderDirection) {
    case 0: {
        const qreal start = state.y + moveY;
        const qreal end = state.y + state.h - moveY;
        position = end != start ? (end - pointerY) / (end - start) : 0.0;
        break;
    }
    case 1: {
        const qreal start = state.x + moveX;
        const qreal end = state.x + state.w - moveX;
        position = end != start ? (pointerX - start) / (end - start) : 0.0;
        break;
    }
    case 2: {
        const qreal start = state.y + moveY;
        const qreal end = state.y + state.h - moveY;
        position = end != start ? (pointerY - start) / (end - start) : 0.0;
        break;
    }
    case 3: {
        const qreal start = state.x + moveX;
        const qreal end = state.x + state.w - moveX;
        position = end != start ? (end - pointerX) / (end - start) : 0.0;
        break;
    }
    default:
        return 0.0;
    }

    return std::clamp(position, 0.0, 1.0);
}

bool Lr2SkinSliderGeometry::readSource(const QVariant& value, Source& source) {
    if (!value.isValid() || value.isNull()) {
        return false;
    }

    if (value.canConvert<Lr2SrcImage>()) {
        const auto parsed = value.value<Lr2SrcImage>();
        source.valid = true;
        source.slider = parsed.slider;
        source.sliderDirection = parsed.sliderDirection;
        source.sliderRange = parsed.sliderRange;
        source.sliderType = parsed.sliderType;
        source.sliderDisabled = parsed.sliderDisabled;
        source.sliderRefNumber = parsed.sliderRefNumber;
        return true;
    }

    if (value.canConvert<QVariantMap>()) {
        const QVariantMap map = value.toMap();
        source.valid = true;
        source.slider = mapBool(map, QStringLiteral("slider"), false);
        source.sliderDirection = mapInt(map, QStringLiteral("sliderDirection"), 0);
        source.sliderRange = mapInt(map, QStringLiteral("sliderRange"), 0);
        source.sliderType = mapInt(map, QStringLiteral("sliderType"), 0);
        source.sliderDisabled = mapInt(map, QStringLiteral("sliderDisabled"), 0);
        source.sliderRefNumber = mapBool(map, QStringLiteral("sliderRefNumber"), false);
        return true;
    }

    if (!value.canConvert<QJSValue>()) {
        return false;
    }

    const QJSValue jsValue = value.value<QJSValue>();
    if (!jsValue.isObject()) {
        return false;
    }

    source.valid = true;
    source.slider = jsBool(jsValue, QStringLiteral("slider"), false);
    source.sliderDirection = jsInt(jsValue, QStringLiteral("sliderDirection"), 0);
    source.sliderRange = jsInt(jsValue, QStringLiteral("sliderRange"), 0);
    source.sliderType = jsInt(jsValue, QStringLiteral("sliderType"), 0);
    source.sliderDisabled = jsInt(jsValue, QStringLiteral("sliderDisabled"), 0);
    source.sliderRefNumber = jsBool(jsValue, QStringLiteral("sliderRefNumber"), false);
    return true;
}

bool Lr2SkinSliderGeometry::readState(const QVariant& value, State& state) {
    if (!value.isValid() || value.isNull()) {
        return false;
    }

    if (value.canConvert<QVariantMap>()) {
        const QVariantMap map = value.toMap();
        state.valid = true;
        state.x = mapReal(map, QStringLiteral("x"), 0.0);
        state.y = mapReal(map, QStringLiteral("y"), 0.0);
        state.w = mapReal(map, QStringLiteral("w"), 0.0);
        state.h = mapReal(map, QStringLiteral("h"), 0.0);
        state.a = mapReal(map, QStringLiteral("a"), 255.0);
        state.r = mapReal(map, QStringLiteral("r"), 255.0);
        state.g = mapReal(map, QStringLiteral("g"), 255.0);
        state.b = mapReal(map, QStringLiteral("b"), 255.0);
        state.angle = mapReal(map, QStringLiteral("angle"), 0.0);
        state.center = mapInt(map, QStringLiteral("center"), 0);
        state.sortId = mapReal(map, QStringLiteral("sortId"), 0.0);
        state.blend = mapInt(map, QStringLiteral("blend"), 0);
        state.filter = mapInt(map, QStringLiteral("filter"), 0);
        state.op1 = mapInt(map, QStringLiteral("op1"), 0);
        state.op2 = mapInt(map, QStringLiteral("op2"), 0);
        state.op3 = mapInt(map, QStringLiteral("op3"), 0);
        state.op4 = mapInt(map, QStringLiteral("op4"), 0);
        return true;
    }

    if (!value.canConvert<QJSValue>()) {
        return false;
    }

    const QJSValue jsValue = value.value<QJSValue>();
    if (!jsValue.isObject()) {
        return false;
    }

    state.valid = true;
    state.x = jsReal(jsValue, QStringLiteral("x"), 0.0);
    state.y = jsReal(jsValue, QStringLiteral("y"), 0.0);
    state.w = jsReal(jsValue, QStringLiteral("w"), 0.0);
    state.h = jsReal(jsValue, QStringLiteral("h"), 0.0);
    state.a = jsReal(jsValue, QStringLiteral("a"), 255.0);
    state.r = jsReal(jsValue, QStringLiteral("r"), 255.0);
    state.g = jsReal(jsValue, QStringLiteral("g"), 255.0);
    state.b = jsReal(jsValue, QStringLiteral("b"), 255.0);
    state.angle = jsReal(jsValue, QStringLiteral("angle"), 0.0);
    state.center = jsInt(jsValue, QStringLiteral("center"), 0);
    state.sortId = jsReal(jsValue, QStringLiteral("sortId"), 0.0);
    state.blend = jsInt(jsValue, QStringLiteral("blend"), 0);
    state.filter = jsInt(jsValue, QStringLiteral("filter"), 0);
    state.op1 = jsInt(jsValue, QStringLiteral("op1"), 0);
    state.op2 = jsInt(jsValue, QStringLiteral("op2"), 0);
    state.op3 = jsInt(jsValue, QStringLiteral("op3"), 0);
    state.op4 = jsInt(jsValue, QStringLiteral("op4"), 0);
    return true;
}

QVariant Lr2SkinSliderGeometry::stateToVariant(const State& state) {
    if (!state.valid) {
        return {};
    }

    QVariantMap value;
    value.insert(QStringLiteral("x"), state.x);
    value.insert(QStringLiteral("y"), state.y);
    value.insert(QStringLiteral("w"), state.w);
    value.insert(QStringLiteral("h"), state.h);
    value.insert(QStringLiteral("a"), state.a);
    value.insert(QStringLiteral("r"), state.r);
    value.insert(QStringLiteral("g"), state.g);
    value.insert(QStringLiteral("b"), state.b);
    value.insert(QStringLiteral("angle"), state.angle);
    value.insert(QStringLiteral("center"), state.center);
    value.insert(QStringLiteral("sortId"), state.sortId);
    value.insert(QStringLiteral("blend"), state.blend);
    value.insert(QStringLiteral("filter"), state.filter);
    value.insert(QStringLiteral("op1"), state.op1);
    value.insert(QStringLiteral("op2"), state.op2);
    value.insert(QStringLiteral("op3"), state.op3);
    value.insert(QStringLiteral("op4"), state.op4);
    return value;
}
