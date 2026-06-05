#include "Lr2SkinRuntimeTypes.h"

#include "gameplay_logic/lr2_skin/Lr2SkinParser.h"

#include <QJSValue>
#include <QVariantMap>

#include <algorithm>
#include <cmath>

using gameplay_logic::lr2_skin::Lr2Dst;
using gameplay_logic::lr2_skin::Lr2SrcBarGraph;
using gameplay_logic::lr2_skin::Lr2SrcBarImage;
using gameplay_logic::lr2_skin::Lr2SrcBarNumber;
using gameplay_logic::lr2_skin::Lr2SrcBarText;
using gameplay_logic::lr2_skin::Lr2SrcImage;
using gameplay_logic::lr2_skin::Lr2SrcNumber;
using gameplay_logic::lr2_skin::Lr2SrcText;

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

bool mapHas(const QVariantMap& map, const QString& name) {
    const auto it = map.constFind(name);
    return it != map.constEnd() && it->isValid() && !it->isNull();
}

int jsInt(const QJSValue& value, const QString& name, int fallback) {
    const QJSValue field = value.property(name);
    return field.isUndefined() || field.isNull() ? fallback : field.toInt();
}

bool jsBool(const QJSValue& value, const QString& name, bool fallback) {
    const QJSValue field = value.property(name);
    return field.isUndefined() || field.isNull() ? fallback : field.toBool();
}

QString jsString(const QJSValue& value, const QString& name) {
    const QJSValue field = value.property(name);
    return field.isUndefined() || field.isNull() ? QString {} : field.toString();
}

bool jsHas(const QJSValue& value, const QString& name) {
    const QJSValue field = value.property(name);
    return !field.isUndefined() && !field.isNull();
}

bool activeOptionPresent(int option, const QVariant& activeOptions) {
    const QVariantList list = activeOptions.toList();
    if (!list.isEmpty()) {
        for (const QVariant& entry : list) {
            if (std::abs(entry.toInt()) == option) {
                return true;
            }
        }
        return false;
    }

    if (!activeOptions.canConvert<QJSValue>()) {
        return false;
    }

    const QJSValue value = activeOptions.value<QJSValue>();
    if (!value.isObject()) {
        return false;
    }

    const QJSValue lookup = value.property(QStringLiteral("__lookup"));
    if (lookup.isObject()) {
        const QJSValue present = lookup.property(QString::number(option));
        if (!present.isUndefined() && !present.isNull()) {
            return present.toBool();
        }
    }

    const int length = value.property(QStringLiteral("length")).toInt();
    for (int i = 0; i < length; ++i) {
        const QJSValue entry = value.property(static_cast<quint32>(i));
        if (entry.isNumber() && std::abs(entry.toInt()) == option) {
            return true;
        }
    }
    return false;
}

using SourcePredicate = bool (*)(const lr2skin::runtime::Source&);

bool sourceTreeHas(const QVariant& value, SourcePredicate predicate, int depth);

bool sourceListTreeHas(const QVariantList& values, SourcePredicate predicate, int depth) {
    for (const QVariant& child : values) {
        if (sourceTreeHas(child, predicate, depth + 1)) {
            return true;
        }
    }
    return false;
}

bool sourceMapTreeHas(const QVariantMap& value, SourcePredicate predicate, int depth) {
    return sourceTreeHas(value.value(QStringLiteral("source")), predicate, depth + 1)
        || sourceTreeHas(value.value(QStringLiteral("sources")), predicate, depth + 1);
}

bool sourceJsChildTreeHas(const QJSValue& value,
                          const QString& propertyName,
                          SourcePredicate predicate,
                          int depth) {
    const QJSValue child = value.property(propertyName);
    return !child.isUndefined()
        && !child.isNull()
        && sourceTreeHas(child.toVariant(), predicate, depth + 1);
}

bool sourceJsTreeHas(const QJSValue& value, SourcePredicate predicate, int depth) {
    return value.isObject()
        && (sourceJsChildTreeHas(value, QStringLiteral("source"), predicate, depth)
            || sourceJsChildTreeHas(value, QStringLiteral("sources"), predicate, depth));
}

bool sourceTreeHas(const QVariant& value, SourcePredicate predicate, int depth) {
    if (!value.isValid() || value.isNull() || depth > 4) {
        return false;
    }

    lr2skin::runtime::Source source;
    if (lr2skin::runtime::readSource(value, source) && predicate(source)) {
        return true;
    }

    if (value.canConvert<Lr2SrcBarImage>()) {
        const Lr2SrcBarImage barImage = value.value<Lr2SrcBarImage>();
        return sourceTreeHas(barImage.source, predicate, depth + 1)
            || sourceTreeHas(QVariant::fromValue(barImage.sources), predicate, depth + 1);
    }
    if (value.canConvert<Lr2SrcBarNumber>()) {
        return sourceTreeHas(value.value<Lr2SrcBarNumber>().source, predicate, depth + 1);
    }

    const QVariantList values = lr2skin::runtime::readVariantList(value);
    if (!values.isEmpty() && sourceListTreeHas(values, predicate, depth)) {
        return true;
    }

    if (value.canConvert<QVariantMap>() && sourceMapTreeHas(value.toMap(), predicate, depth)) {
        return true;
    }

    return value.canConvert<QJSValue>()
        && sourceJsTreeHas(value.value<QJSValue>(), predicate, depth);
}

} // namespace

namespace lr2skin::runtime {

QVariantList readVariantList(const QVariant& value) {
    const QVariantList list = value.toList();
    if (!list.isEmpty()) {
        return list;
    }

    if (!value.canConvert<QJSValue>()) {
        return {};
    }

    const QJSValue jsValue = value.value<QJSValue>();
    if (!jsValue.isArray()) {
        return {};
    }

    QVariantList result;
    const int length = jsValue.property(QStringLiteral("length")).toInt();
    result.reserve(length);
    for (int i = 0; i < length; ++i) {
        result.append(jsValue.property(static_cast<quint32>(i)).toVariant());
    }
    return result;
}

QVector<int> readOffsets(const QVariant& value) {
    QVector<int> offsets;

    const QVariantList list = value.toList();
    if (!list.isEmpty()) {
        offsets.reserve(list.size());
        for (const QVariant& entry : list) {
            bool ok = false;
            const int offset = entry.toInt(&ok);
            if (ok) {
                offsets.append(offset);
            }
        }
        return offsets;
    }

    if (!value.canConvert<QJSValue>()) {
        return offsets;
    }

    const QJSValue jsValue = value.value<QJSValue>();
    if (!jsValue.isArray()) {
        return offsets;
    }

    const int length = jsValue.property(QStringLiteral("length")).toInt();
    offsets.reserve(length);
    for (int i = 0; i < length; ++i) {
        const QJSValue entry = jsValue.property(static_cast<quint32>(i));
        if (entry.isNumber()) {
            offsets.append(entry.toInt());
        }
    }
    return offsets;
}

bool readDst(const QVariant& value, Dst& dst) {
    if (value.canConvert<Lr2TimelineStateValue>()) {
        const auto state = value.value<Lr2TimelineStateValue>();
        dst.valid = state.valid;
        dst.x = state.x;
        dst.y = state.y;
        dst.w = state.w;
        dst.h = state.h;
        dst.a = state.a;
        dst.r = state.r;
        dst.g = state.g;
        dst.b = state.b;
        dst.angle = state.angle;
        dst.center = state.center;
        dst.sortId = state.sortId;
        dst.blend = state.blend;
        dst.filter = state.filter;
        dst.op1 = state.op1;
        dst.op2 = state.op2;
        dst.op3 = state.op3;
        dst.op4 = state.op4;
        return state.valid;
    }

    if (value.canConvert<Lr2Dst>()) {
        const auto parsed = value.value<Lr2Dst>();
        dst.valid = true;
        dst.time = parsed.time;
        dst.x = parsed.x;
        dst.y = parsed.y;
        dst.w = parsed.w;
        dst.h = parsed.h;
        dst.acc = parsed.acc;
        dst.a = parsed.a;
        dst.r = parsed.r;
        dst.g = parsed.g;
        dst.b = parsed.b;
        dst.blend = parsed.blend;
        dst.filter = parsed.filter;
        dst.angle = parsed.angle;
        dst.center = parsed.center;
        dst.sortId = parsed.sortId;
        dst.loop = parsed.loop;
        dst.timer = parsed.timer;
        dst.op1 = parsed.op1;
        dst.op2 = parsed.op2;
        dst.op3 = parsed.op3;
        dst.op4 = parsed.op4;
        dst.offsets = readOffsets(parsed.offsets);
        return true;
    }

    if (value.canConvert<QVariantMap>()) {
        const QVariantMap map = value.toMap();
        dst.valid = true;
        dst.time = mapInt(map, QStringLiteral("time"), 0);
        dst.x = mapInt(map, QStringLiteral("x"), 0);
        dst.y = mapInt(map, QStringLiteral("y"), 0);
        dst.w = mapInt(map, QStringLiteral("w"), 0);
        dst.h = mapInt(map, QStringLiteral("h"), 0);
        dst.acc = mapInt(map, QStringLiteral("acc"), 0);
        dst.a = mapInt(map, QStringLiteral("a"), 255);
        dst.r = mapInt(map, QStringLiteral("r"), 255);
        dst.g = mapInt(map, QStringLiteral("g"), 255);
        dst.b = mapInt(map, QStringLiteral("b"), 255);
        dst.blend = mapInt(map, QStringLiteral("blend"), 0);
        dst.filter = mapInt(map, QStringLiteral("filter"), 0);
        dst.angle = mapInt(map, QStringLiteral("angle"), 0);
        dst.center = mapInt(map, QStringLiteral("center"), 0);
        dst.sortId = mapInt(map, QStringLiteral("sortId"), 0);
        dst.loop = mapInt(map, QStringLiteral("loop"), 0);
        dst.timer = mapInt(map, QStringLiteral("timer"), 0);
        dst.op1 = mapInt(map, QStringLiteral("op1"), 0);
        dst.op2 = mapInt(map, QStringLiteral("op2"), 0);
        dst.op3 = mapInt(map, QStringLiteral("op3"), 0);
        dst.op4 = mapInt(map, QStringLiteral("op4"), 0);
        const auto offsetsIt = map.constFind(QStringLiteral("offsets"));
        if (offsetsIt != map.constEnd()) {
            dst.offsets = readOffsets(*offsetsIt);
        }
        return true;
    }

    if (!value.canConvert<QJSValue>()) {
        return false;
    }

    const QJSValue jsValue = value.value<QJSValue>();
    if (!jsValue.isObject()) {
        return false;
    }

    dst.valid = true;
    dst.time = jsInt(jsValue, QStringLiteral("time"), 0);
    dst.x = jsInt(jsValue, QStringLiteral("x"), 0);
    dst.y = jsInt(jsValue, QStringLiteral("y"), 0);
    dst.w = jsInt(jsValue, QStringLiteral("w"), 0);
    dst.h = jsInt(jsValue, QStringLiteral("h"), 0);
    dst.acc = jsInt(jsValue, QStringLiteral("acc"), 0);
    dst.a = jsInt(jsValue, QStringLiteral("a"), 255);
    dst.r = jsInt(jsValue, QStringLiteral("r"), 255);
    dst.g = jsInt(jsValue, QStringLiteral("g"), 255);
    dst.b = jsInt(jsValue, QStringLiteral("b"), 255);
    dst.blend = jsInt(jsValue, QStringLiteral("blend"), 0);
    dst.filter = jsInt(jsValue, QStringLiteral("filter"), 0);
    dst.angle = jsInt(jsValue, QStringLiteral("angle"), 0);
    dst.center = jsInt(jsValue, QStringLiteral("center"), 0);
    dst.sortId = jsInt(jsValue, QStringLiteral("sortId"), 0);
    dst.loop = jsInt(jsValue, QStringLiteral("loop"), 0);
    dst.timer = jsInt(jsValue, QStringLiteral("timer"), 0);
    dst.op1 = jsInt(jsValue, QStringLiteral("op1"), 0);
    dst.op2 = jsInt(jsValue, QStringLiteral("op2"), 0);
    dst.op3 = jsInt(jsValue, QStringLiteral("op3"), 0);
    dst.op4 = jsInt(jsValue, QStringLiteral("op4"), 0);
    dst.offsets = readOffsets(QVariant::fromValue(jsValue.property(QStringLiteral("offsets"))));
    return true;
}

QVector<Dst> readDsts(const QVariantList& dsts) {
    QVector<Dst> result;
    result.reserve(dsts.size());
    for (const QVariant& entry : dsts) {
        Dst dst;
        if (readDst(entry, dst)) {
            result.append(dst);
        }
    }
    return result;
}

QVector<Dst> readDsts(const QVariant& dsts) {
    return readDsts(readVariantList(dsts));
}

bool readSource(const QVariant& value, Source& source) {
    if (!value.isValid() || value.isNull()) {
        return false;
    }

    if (value.canConvert<Lr2SrcImage>()) {
        const auto parsed = value.value<Lr2SrcImage>();
        source.valid = true;
        source.x = parsed.x;
        source.y = parsed.y;
        source.w = parsed.w;
        source.h = parsed.h;
        source.divX = std::max(1, parsed.div_x);
        source.divY = std::max(1, parsed.div_y);
        source.cycle = parsed.cycle;
        source.timer = parsed.timer;
        source.resultChartType = parsed.resultChartType;
        source.button = parsed.button;
        source.buttonId = parsed.buttonId;
        source.onMouse = parsed.onMouse;
        source.mouseCursor = parsed.mouseCursor;
        source.slider = parsed.slider;
        source.sliderDirection = parsed.sliderDirection;
        source.sliderRange = parsed.sliderRange;
        source.sliderType = parsed.sliderType;
        source.sliderDisabled = parsed.sliderDisabled;
        source.sliderRefNumber = parsed.sliderRefNumber;
        source.imageSet = parsed.imageSet;
        source.side = parsed.side;
        source.specialType = parsed.specialType;
        source.path = parsed.source;
        return true;
    }

    if (value.canConvert<Lr2SrcNumber>()) {
        const auto parsed = value.value<Lr2SrcNumber>();
        source.valid = true;
        source.num = parsed.num;
        source.x = parsed.x;
        source.y = parsed.y;
        source.w = parsed.w;
        source.h = parsed.h;
        source.divX = std::max(1, parsed.div_x);
        source.divY = std::max(1, parsed.div_y);
        source.cycle = parsed.cycle;
        source.timer = parsed.timer;
        source.side = parsed.side;
        source.path = parsed.source;
        return true;
    }

    if (value.canConvert<Lr2SrcBarGraph>()) {
        const auto parsed = value.value<Lr2SrcBarGraph>();
        source.valid = true;
        source.graphType = parsed.graphType;
        source.x = parsed.x;
        source.y = parsed.y;
        source.w = parsed.w;
        source.h = parsed.h;
        source.divX = std::max(1, parsed.div_x);
        source.divY = std::max(1, parsed.div_y);
        source.cycle = parsed.cycle;
        source.timer = parsed.timer;
        source.specialType = parsed.specialType;
        source.path = parsed.source;
        return true;
    }

    if (value.canConvert<Lr2SrcBarImage>()) {
        const auto parsed = value.value<Lr2SrcBarImage>();
        source.valid = true;
        source.hasKind = true;
        source.kind = parsed.kind;
        return true;
    }

    if (value.canConvert<Lr2SrcBarNumber>()) {
        source.valid = true;
        return true;
    }

    if (value.canConvert<Lr2SrcText>()) {
        const auto parsed = value.value<Lr2SrcText>();
        source.valid = true;
        source.font = parsed.font;
        source.st = parsed.st;
        source.align = parsed.align;
        source.edit = parsed.edit;
        source.panel = parsed.panel;
        source.fontPath = parsed.fontPath;
        source.fontFamily = parsed.fontFamily;
        source.fontSize = parsed.fontSize;
        source.fontThickness = parsed.fontThickness;
        source.fontType = parsed.fontType;
        source.bitmapFont = parsed.bitmapFont;
        source.readme = parsed.readme;
        source.readmeId = parsed.readmeId;
        source.readmeLineSpacing = parsed.readmeLineSpacing;
        return true;
    }

    if (value.canConvert<Lr2SrcBarText>()) {
        const auto parsed = value.value<Lr2SrcBarText>();
        source.valid = true;
        source.titleType = parsed.titleType;
        source.font = parsed.font;
        source.st = parsed.st;
        source.align = parsed.align;
        source.edit = parsed.edit;
        source.panel = parsed.panel;
        source.fontPath = parsed.fontPath;
        source.fontFamily = parsed.fontFamily;
        source.fontSize = parsed.fontSize;
        source.fontThickness = parsed.fontThickness;
        source.fontType = parsed.fontType;
        source.bitmapFont = parsed.bitmapFont;
        return true;
    }

    if (value.canConvert<QVariantMap>()) {
        const QVariantMap map = value.toMap();
        source.valid = true;
        source.num = mapInt(map, QStringLiteral("num"), 0);
        source.st = mapInt(map, QStringLiteral("st"), 0);
        source.x = mapInt(map, QStringLiteral("x"), 0);
        source.y = mapInt(map, QStringLiteral("y"), 0);
        source.w = mapInt(map, QStringLiteral("w"), 0);
        source.h = mapInt(map, QStringLiteral("h"), 0);
        source.divX = std::max(1, mapInt(map, QStringLiteral("div_x"), 1));
        source.divY = std::max(1, mapInt(map, QStringLiteral("div_y"), 1));
        source.cycle = mapInt(map, QStringLiteral("cycle"), 0);
        source.timer = mapInt(map, QStringLiteral("timer"), 0);
        source.resultChartType = mapInt(map, QStringLiteral("resultChartType"), 0);
        source.button = mapBool(map, QStringLiteral("button"), false);
        source.buttonId = mapInt(map, QStringLiteral("buttonId"), 0);
        source.onMouse = mapBool(map, QStringLiteral("onMouse"), false);
        source.mouseCursor = mapBool(map, QStringLiteral("mouseCursor"), false);
        source.slider = mapBool(map, QStringLiteral("slider"), false);
        source.sliderDirection = mapInt(map, QStringLiteral("sliderDirection"), 0);
        source.sliderRange = mapInt(map, QStringLiteral("sliderRange"), 0);
        source.sliderType = mapInt(map, QStringLiteral("sliderType"), 0);
        source.sliderDisabled = mapInt(map, QStringLiteral("sliderDisabled"), 0);
        source.sliderRefNumber = mapBool(map, QStringLiteral("sliderRefNumber"), false);
        source.imageSet = mapBool(map, QStringLiteral("imageSet"), false);
        source.side = mapInt(map, QStringLiteral("side"), 0);
        source.hasKind = mapHas(map, QStringLiteral("kind"));
        source.kind = mapInt(map, QStringLiteral("kind"), 0);
        source.graphType = mapInt(map, QStringLiteral("graphType"), 0);
        source.titleType = mapInt(map, QStringLiteral("titleType"), -1);
        source.align = mapInt(map, QStringLiteral("align"), 0);
        source.font = mapInt(map, QStringLiteral("font"), 0);
        source.edit = mapInt(map, QStringLiteral("edit"), 0);
        source.panel = mapInt(map, QStringLiteral("panel"), 0);
        source.fontPath = map.value(QStringLiteral("fontPath")).toString();
        source.fontFamily = map.value(QStringLiteral("fontFamily")).toString();
        source.fontSize = mapInt(map, QStringLiteral("fontSize"), 0);
        source.fontThickness = mapInt(map, QStringLiteral("fontThickness"), 0);
        source.fontType = mapInt(map, QStringLiteral("fontType"), 0);
        source.bitmapFont = mapBool(map, QStringLiteral("bitmapFont"), false);
        source.readme = mapBool(map, QStringLiteral("readme"), false);
        source.readmeId = mapInt(map, QStringLiteral("readmeId"), 0);
        source.readmeLineSpacing = mapInt(map, QStringLiteral("readmeLineSpacing"), 18);
        source.specialType = mapInt(map, QStringLiteral("specialType"), 0);
        source.path = map.value(QStringLiteral("source")).toString();
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
    source.num = jsInt(jsValue, QStringLiteral("num"), 0);
    source.st = jsInt(jsValue, QStringLiteral("st"), 0);
    source.x = jsInt(jsValue, QStringLiteral("x"), 0);
    source.y = jsInt(jsValue, QStringLiteral("y"), 0);
    source.w = jsInt(jsValue, QStringLiteral("w"), 0);
    source.h = jsInt(jsValue, QStringLiteral("h"), 0);
    source.divX = std::max(1, jsInt(jsValue, QStringLiteral("div_x"), 1));
    source.divY = std::max(1, jsInt(jsValue, QStringLiteral("div_y"), 1));
    source.cycle = jsInt(jsValue, QStringLiteral("cycle"), 0);
    source.timer = jsInt(jsValue, QStringLiteral("timer"), 0);
    source.resultChartType = jsInt(jsValue, QStringLiteral("resultChartType"), 0);
    source.button = jsBool(jsValue, QStringLiteral("button"), false);
    source.buttonId = jsInt(jsValue, QStringLiteral("buttonId"), 0);
    source.onMouse = jsBool(jsValue, QStringLiteral("onMouse"), false);
    source.mouseCursor = jsBool(jsValue, QStringLiteral("mouseCursor"), false);
    source.slider = jsBool(jsValue, QStringLiteral("slider"), false);
    source.sliderDirection = jsInt(jsValue, QStringLiteral("sliderDirection"), 0);
    source.sliderRange = jsInt(jsValue, QStringLiteral("sliderRange"), 0);
    source.sliderType = jsInt(jsValue, QStringLiteral("sliderType"), 0);
    source.sliderDisabled = jsInt(jsValue, QStringLiteral("sliderDisabled"), 0);
    source.sliderRefNumber = jsBool(jsValue, QStringLiteral("sliderRefNumber"), false);
    source.imageSet = jsBool(jsValue, QStringLiteral("imageSet"), false);
    source.side = jsInt(jsValue, QStringLiteral("side"), 0);
    source.hasKind = jsHas(jsValue, QStringLiteral("kind"));
    source.kind = jsInt(jsValue, QStringLiteral("kind"), 0);
    source.graphType = jsInt(jsValue, QStringLiteral("graphType"), 0);
    source.titleType = jsInt(jsValue, QStringLiteral("titleType"), -1);
    source.align = jsInt(jsValue, QStringLiteral("align"), 0);
    source.font = jsInt(jsValue, QStringLiteral("font"), 0);
    source.edit = jsInt(jsValue, QStringLiteral("edit"), 0);
    source.panel = jsInt(jsValue, QStringLiteral("panel"), 0);
    source.fontPath = jsString(jsValue, QStringLiteral("fontPath"));
    source.fontFamily = jsString(jsValue, QStringLiteral("fontFamily"));
    source.fontSize = jsInt(jsValue, QStringLiteral("fontSize"), 0);
    source.fontThickness = jsInt(jsValue, QStringLiteral("fontThickness"), 0);
    source.fontType = jsInt(jsValue, QStringLiteral("fontType"), 0);
    source.bitmapFont = jsBool(jsValue, QStringLiteral("bitmapFont"), false);
    source.readme = jsBool(jsValue, QStringLiteral("readme"), false);
    source.readmeId = jsInt(jsValue, QStringLiteral("readmeId"), 0);
    source.readmeLineSpacing = jsInt(jsValue, QStringLiteral("readmeLineSpacing"), 18);
    source.specialType = jsInt(jsValue, QStringLiteral("specialType"), 0);
    const QJSValue sourcePath = jsValue.property(QStringLiteral("source"));
    source.path = sourcePath.isUndefined() || sourcePath.isNull()
        ? QString {}
        : sourcePath.toString();
    return true;
}

bool readState(const QVariant& value, State& state) {
    if (!value.isValid() || value.isNull()) {
        return false;
    }

    if (value.canConvert<State>()) {
        state = value.value<State>();
        return state.valid;
    }

    if (!value.canConvert<QVariantMap>()) {
        return false;
    }

    const QVariantMap map = value.toMap();
    if (!mapHas(map, QStringLiteral("x"))
            && !mapHas(map, QStringLiteral("y"))
            && !mapHas(map, QStringLiteral("w"))
            && !mapHas(map, QStringLiteral("h"))) {
        return false;
    }

    state.valid = true;
    state.x = mapReal(map, QStringLiteral("x"), state.x);
    state.y = mapReal(map, QStringLiteral("y"), state.y);
    state.w = mapReal(map, QStringLiteral("w"), state.w);
    state.h = mapReal(map, QStringLiteral("h"), state.h);
    state.a = mapReal(map, QStringLiteral("a"), state.a);
    state.r = mapReal(map, QStringLiteral("r"), state.r);
    state.g = mapReal(map, QStringLiteral("g"), state.g);
    state.b = mapReal(map, QStringLiteral("b"), state.b);
    state.angle = mapReal(map, QStringLiteral("angle"), state.angle);
    state.center = mapInt(map, QStringLiteral("center"), state.center);
    state.sortId = mapReal(map, QStringLiteral("sortId"), state.sortId);
    state.blend = mapInt(map, QStringLiteral("blend"), state.blend);
    state.filter = mapInt(map, QStringLiteral("filter"), state.filter);
    state.op1 = mapInt(map, QStringLiteral("op1"), state.op1);
    state.op2 = mapInt(map, QStringLiteral("op2"), state.op2);
    state.op3 = mapInt(map, QStringLiteral("op3"), state.op3);
    state.op4 = mapInt(map, QStringLiteral("op4"), state.op4);
    return true;
}

DstAnalysis analyzeDsts(const QVector<Dst>& dsts) {
    DstAnalysis analysis;
    if (dsts.isEmpty() || !dsts.front().valid) {
        return analysis;
    }

    const Dst& first = dsts.front();
    analysis.firstTimer = first.timer;
    analysis.firstSortId = first.sortId;
    analysis.usesDynamicTimer = first.timer != 0;
    analysis.usesActiveOptions = first.op1 != 0 || first.op2 != 0 || first.op3 != 0;
    analysis.scratchRotationSide = first.op4 == 1 || first.op4 == 2 ? first.op4 : 0;
    analysis.canUseStaticState = dsts.size() == 1
        && first.time <= 0
        && first.timer == 0
        && first.op1 == 0
        && first.op2 == 0
        && first.op3 == 0;

    if (dsts.size() >= 2) {
        const Dst& last = dsts.back();
        analysis.loopsContinuously = first.loop >= 0 && first.loop < last.time;
    }
    return analysis;
}

int animationLimitFor(const QVector<Dst>& dsts) {
    if (dsts.isEmpty() || !dsts.front().valid) {
        return 0;
    }

    const Dst& first = dsts.front();
    if (first.timer != 0) {
        return -1;
    }

    const int endTime = dsts.back().time;
    if (dsts.size() == 1) {
        return first.loop < 0 ? endTime + 1 : first.time;
    }

    if (first.loop < 0 || first.loop > endTime) {
        return endTime + 1;
    }
    if (first.loop == endTime) {
        return endTime;
    }
    return -1;
}

QSet<int> activeOptionSet(const QVariant& activeOptions) {
    QSet<int> result;

    const QVariantList list = activeOptions.toList();
    if (!list.isEmpty()) {
        for (const QVariant& option : list) {
            bool ok = false;
            const int value = option.toInt(&ok);
            if (ok) {
                result.insert(std::abs(value));
            }
        }
        return result;
    }

    if (!activeOptions.canConvert<QJSValue>()) {
        return result;
    }

    const QJSValue value = activeOptions.value<QJSValue>();
    if (!value.isObject()) {
        return result;
    }

    const QJSValue lookup = value.property(QStringLiteral("__lookup"));
    if (lookup.isObject()) {
        const QVariantMap map = lookup.toVariant().toMap();
        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            if (it.value().toBool()) {
                result.insert(std::abs(it.key().toInt()));
            }
        }
        if (!result.isEmpty()) {
            return result;
        }
    }

    const int length = value.property(QStringLiteral("length")).toInt();
    for (int i = 0; i < length; ++i) {
        const QJSValue option = value.property(static_cast<quint32>(i));
        if (option.isNumber()) {
            result.insert(std::abs(option.toInt()));
        }
    }
    return result;
}

QVariantList activeOptionsForDsts(const Dst& firstDst, const QVariant& activeOptions) {
    int ids[3] = {0, 0, 0};
    int count = 0;
    const int ops[3] = {firstDst.op1, firstDst.op2, firstDst.op3};
    for (int op : ops) {
        if (op == 0) {
            continue;
        }
        const int id = std::abs(op);
        if (!activeOptionPresent(id, activeOptions)) {
            continue;
        }
        bool duplicate = false;
        for (int i = 0; i < count; ++i) {
            duplicate = duplicate || ids[i] == id;
        }
        if (!duplicate && count < 3) {
            ids[count++] = id;
        }
    }

    if (count == 0) {
        return {};
    }

    std::sort(ids, ids + count);
    QVariantList result;
    result.reserve(count);
    for (int i = 0; i < count; ++i) {
        result.append(ids[i]);
    }
    return result;
}

bool allOpsMatch(const Dst& dst, const QSet<int>& activeOptions) {
    const auto check = [&activeOptions](int op) {
        if (op == 0) {
            return true;
        }
        const bool negate = op < 0;
        const bool present = activeOptions.contains(std::abs(op));
        return negate ? !present : present;
    };
    return check(dst.op1) && check(dst.op2) && check(dst.op3);
}

qreal timerValue(const QVariant& timers, int timerIdx) {
    if (!timers.isValid() || timers.isNull()) {
        return timerIdx == 0 ? 0.0 : -1.0;
    }

    const QString key = QString::number(timerIdx);
    if (timers.canConvert<QVariantMap>()) {
        const QVariantMap map = timers.toMap();
        const auto it = map.constFind(key);
        return it == map.constEnd() || !it->isValid() || it->isNull()
            ? (timerIdx == 0 ? 0.0 : -1.0)
            : it->toDouble();
    }

    if (timers.canConvert<QJSValue>()) {
        const QJSValue value = timers.value<QJSValue>();
        if (!value.isObject()) {
            return timerIdx == 0 ? 0.0 : -1.0;
        }
        const QJSValue field = value.property(key);
        return field.isUndefined() || field.isNull()
            ? (timerIdx == 0 ? 0.0 : -1.0)
            : field.toNumber();
    }

    return timerIdx == 0 ? 0.0 : -1.0;
}

State currentState(const QVector<Dst>& dsts,
                   int globalTime,
                   qreal timerFire,
                   const QSet<int>& activeOptions) {
    if (dsts.isEmpty() || !dsts.front().valid) {
        return {};
    }

    const Dst& first = dsts.front();
    if (!allOpsMatch(first, activeOptions) || timerFire < 0.0) {
        return {};
    }

    int time = globalTime - static_cast<int>(std::floor(timerFire));
    if (time < first.time) {
        return {};
    }

    const Dst& last = dsts.back();
    const int endTime = last.time;
    const int loopTo = first.loop;

    if (dsts.size() == 1) {
        if (loopTo < 0 && time > endTime) {
            return {};
        }
        return copyDstAsState(first, first);
    }

    if (time > endTime) {
        if (loopTo < 0) {
            return {};
        }
        if (loopTo == endTime) {
            time = endTime;
        } else if (loopTo > endTime) {
            time = 0;
        } else {
            const int period = endTime - loopTo;
            if (period <= 0) {
                return {};
            }
            time = loopTo + ((time - loopTo) % period);
        }
    }

    if (time >= endTime) {
        return copyDstAsState(last, first);
    }

    const Dst* d1 = &dsts.front();
    const Dst* d2 = &dsts.front();
    for (qsizetype i = 0; i < dsts.size() - 1; ++i) {
        if (time >= dsts[i].time && time < dsts[i + 1].time) {
            d1 = &dsts[i];
            d2 = &dsts[i + 1];
            break;
        }
    }

    const int segment = d2->time - d1->time;
    if (segment <= 0) {
        return copyDstAsState(*d1, first);
    }

    const qreal progress = applyAccel(static_cast<qreal>(time - d1->time) / segment, d1->acc);
    const auto mix = [progress](qreal a, qreal b) {
        return a + (b - a) * progress;
    };

    State state;
    state.valid = true;
    state.x = mix(d1->x, d2->x);
    state.y = mix(d1->y, d2->y);
    state.w = mix(d1->w, d2->w);
    state.h = mix(d1->h, d2->h);
    state.a = mix(d1->a, d2->a);
    state.r = mix(d1->r, d2->r);
    state.g = mix(d1->g, d2->g);
    state.b = mix(d1->b, d2->b);
    state.angle = mix(d1->angle, d2->angle);
    state.center = d1->center;
    state.sortId = mix(d1->sortId, d2->sortId);
    state.blend = d1->blend;
    state.filter = d1->filter;
    state.op1 = first.op1;
    state.op2 = first.op2;
    state.op3 = first.op3;
    state.op4 = first.op4;
    return state;
}

State copyDstAsState(const Dst& dst, const Dst& controlDst) {
    State state;
    state.valid = true;
    state.x = dst.x;
    state.y = dst.y;
    state.w = dst.w;
    state.h = dst.h;
    state.a = dst.a;
    state.r = dst.r;
    state.g = dst.g;
    state.b = dst.b;
    state.angle = dst.angle;
    state.center = dst.center;
    state.sortId = dst.sortId;
    state.blend = dst.blend;
    state.filter = dst.filter;
    state.op1 = controlDst.op1;
    state.op2 = controlDst.op2;
    state.op3 = controlDst.op3;
    state.op4 = controlDst.op4;
    return state;
}

bool sameState(const State& lhs, const State& rhs) {
    const auto sameReal = [](qreal a, qreal b) {
        return std::abs(a - b) <= 0.0001;
    };

    return lhs.valid == rhs.valid
        && sameReal(lhs.x, rhs.x)
        && sameReal(lhs.y, rhs.y)
        && sameReal(lhs.w, rhs.w)
        && sameReal(lhs.h, rhs.h)
        && sameReal(lhs.a, rhs.a)
        && sameReal(lhs.r, rhs.r)
        && sameReal(lhs.g, rhs.g)
        && sameReal(lhs.b, rhs.b)
        && sameReal(lhs.angle, rhs.angle)
        && lhs.center == rhs.center
        && sameReal(lhs.sortId, rhs.sortId)
        && lhs.blend == rhs.blend
        && lhs.filter == rhs.filter
        && lhs.op1 == rhs.op1
        && lhs.op2 == rhs.op2
        && lhs.op3 == rhs.op3
        && lhs.op4 == rhs.op4;
}

qreal applyAccel(qreal progress, int accType) {
    if (accType == 1) {
        return progress * progress;
    }
    if (accType == 2) {
        const qreal inv = 1.0 - progress;
        return 1.0 - inv * inv;
    }
    if (accType == 3) {
        return 0.0;
    }
    return progress;
}

bool sourceCyclesContinuously(const Source& source) {
    return source.valid
        && source.cycle > 0
        && std::max(1, source.divX) * std::max(1, source.divY) > 1;
}

bool sourceUsesChartAsset(const Source& source) {
    return source.valid && chartAssetSourceType(source) != 0;
}

bool sourceTreeCyclesContinuously(const QVariant& value) {
    return sourceTreeHas(value, sourceCyclesContinuously, 0);
}

bool sourceTreeUsesChartAsset(const QVariant& value) {
    return sourceTreeHas(value, sourceUsesChartAsset, 0);
}

int chartAssetSourceType(const Source& source) {
    switch (source.specialType) {
    case Lr2SrcImage::StageFile:
    case Lr2SrcImage::BackBmp:
    case Lr2SrcImage::Banner:
        return source.specialType;
    default:
        return 0;
    }
}

bool sourceUsesDynamicTimer(const Source& source) {
    return source.valid && source.timer != 0 && source.cycle > 0;
}

bool isSelectBarElement(int type, const Source& source) {
    return type == 4
        || type == 5
        || type == 13
        || (type == 3 && source.hasKind);
}

qreal selectBarElementLayer(int type, const Source& source) {
    if (type == 13) {
        return 0.20;
    }
    if (type == 4) {
        return 0.30;
    }
    if (type == 5) {
        return 0.60;
    }
    if (type != 3 || !source.valid) {
        return 0.0;
    }

    switch (source.kind) {
    case 0:
    case 1:
        return 0.0;
    case 2:
        return 0.10;
    case 3:
    case 4:
    case 5:
        return 0.50;
    case 6:
    case 7:
        return 0.65;
    case 8:
        return 0.70;
    default:
        return 0.40;
    }
}

int staticNoteElementSortId(const QVariantList& noteDsts) {
    int result = -1;
    for (const QVariant& laneDsts : noteDsts) {
        const int sortId = firstSortId(laneDsts);
        result = result < 0 || sortId < result ? sortId : result;
    }
    return result >= 0 ? result : 0;
}

int firstSortId(const QVariantList& dsts) {
    if (dsts.isEmpty()) {
        return 0;
    }
    Dst dst;
    return readDst(dsts.first(), dst) ? dst.sortId : 0;
}

int firstSortId(const QVariant& dsts) {
    return firstSortId(readVariantList(dsts));
}

SpriteStateOverrideKind spriteStateOverrideKind(const QString& screenKey,
                                                bool gameplayScreen,
                                                const Source& source) {
    if (!source.valid || !source.slider || source.sliderRange <= 0) {
        return NoSpriteStateOverride;
    }

    const bool selectScreen = screenKey == QStringLiteral("select");
    if (selectScreen && source.sliderType == 1 && source.sliderDisabled == 0) {
        return SelectScrollSpriteStateOverride;
    }
    if (gameplayScreen && source.sliderType == 6) {
        return GameplayProgressSpriteStateOverride;
    }
    if (gameplayScreen && (source.sliderType == 4 || source.sliderType == 5)) {
        return GameplayLaneCoverSpriteStateOverride;
    }
    if (source.sliderRefNumber) {
        return NumberRefSpriteStateOverride;
    }
    if (selectScreen && source.sliderDisabled == 0) {
        return GenericSliderSpriteStateOverride;
    }
    return NoSpriteStateOverride;
}

State translatedSliderState(State state, qreal position, int range, int direction) {
    if (!state.valid) {
        return state;
    }

    const qreal offset = position * std::max(1, range);
    switch (direction) {
    case 0:
        state.y -= offset;
        break;
    case 1:
        state.x += offset;
        break;
    case 2:
        state.y += offset;
        break;
    case 3:
        state.x -= offset;
        break;
    default:
        state.valid = false;
        break;
    }
    return state;
}

State sliderTrackState(const Source& source, const State& baseState) {
    if (!source.valid || !source.slider || !baseState.valid) {
        return {};
    }

    State track = baseState;
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

    return track;
}

qreal sliderPositionFromPointer(const Source& source,
                                const State& track,
                                qreal pointerX,
                                qreal pointerY) {
    State state = track;
    if (!source.valid || !source.slider || !state.valid) {
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

bool rectContains(const QVariant& state, qreal skinX, qreal skinY) {
    State value;
    if (!readState(state, value)) {
        return false;
    }

    const qreal left = std::min(value.x, value.x + value.w);
    const qreal right = std::max(value.x, value.x + value.w);
    const qreal top = std::min(value.y, value.y + value.h);
    const qreal bottom = std::max(value.y, value.y + value.h);
    return skinX >= left && skinX <= right && skinY >= top && skinY <= bottom;
}

} // namespace lr2skin::runtime
