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
        dst.timerCallback = parsed.timerCallback;
        dst.op1 = parsed.op1;
        dst.op2 = parsed.op2;
        dst.op3 = parsed.op3;
        dst.op4 = parsed.op4;
        dst.offsets = readOffsets(parsed.offsets);
        dst.stretch = parsed.stretch;
        dst.hasMouseRect = parsed.hasMouseRect;
        dst.mouseRectX = parsed.mouseRectX;
        dst.mouseRectY = parsed.mouseRectY;
        dst.mouseRectW = parsed.mouseRectW;
        dst.mouseRectH = parsed.mouseRectH;
        dst.drawCallback = parsed.drawCallback;
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
        dst.timerCallback = mapInt(map, QStringLiteral("timerCallback"), 0);
        dst.op1 = mapInt(map, QStringLiteral("op1"), 0);
        dst.op2 = mapInt(map, QStringLiteral("op2"), 0);
        dst.op3 = mapInt(map, QStringLiteral("op3"), 0);
        dst.op4 = mapInt(map, QStringLiteral("op4"), 0);
        const auto offsetsIt = map.constFind(QStringLiteral("offsets"));
        if (offsetsIt != map.constEnd()) {
            dst.offsets = readOffsets(*offsetsIt);
        }
        dst.stretch = mapInt(map, QStringLiteral("stretch"), -1);
        dst.hasMouseRect = mapBool(map, QStringLiteral("hasMouseRect"), false);
        if (dst.hasMouseRect || mapHas(map, QStringLiteral("mouseRectX"))) {
            dst.hasMouseRect = true;
            dst.mouseRectX = mapInt(map, QStringLiteral("mouseRectX"), 0);
            dst.mouseRectY = mapInt(map, QStringLiteral("mouseRectY"), 0);
            dst.mouseRectW = mapInt(map, QStringLiteral("mouseRectW"), 0);
            dst.mouseRectH = mapInt(map, QStringLiteral("mouseRectH"), 0);
        }
        dst.drawCallback = mapInt(map, QStringLiteral("drawCallback"), 0);
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
    dst.timerCallback = jsInt(jsValue, QStringLiteral("timerCallback"), 0);
    dst.op1 = jsInt(jsValue, QStringLiteral("op1"), 0);
    dst.op2 = jsInt(jsValue, QStringLiteral("op2"), 0);
    dst.op3 = jsInt(jsValue, QStringLiteral("op3"), 0);
    dst.op4 = jsInt(jsValue, QStringLiteral("op4"), 0);
    dst.offsets = readOffsets(QVariant::fromValue(jsValue.property(QStringLiteral("offsets"))));
    dst.stretch = jsInt(jsValue, QStringLiteral("stretch"), -1);
    dst.hasMouseRect = jsBool(jsValue, QStringLiteral("hasMouseRect"), false);
    if (dst.hasMouseRect || jsHas(jsValue, QStringLiteral("mouseRectX"))) {
        dst.hasMouseRect = true;
        dst.mouseRectX = jsInt(jsValue, QStringLiteral("mouseRectX"), 0);
        dst.mouseRectY = jsInt(jsValue, QStringLiteral("mouseRectY"), 0);
        dst.mouseRectW = jsInt(jsValue, QStringLiteral("mouseRectW"), 0);
        dst.mouseRectH = jsInt(jsValue, QStringLiteral("mouseRectH"), 0);
    }
    dst.drawCallback = jsInt(jsValue, QStringLiteral("drawCallback"), 0);
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
        source.timerCallback = parsed.timerCallback;
        source.resultChartType = parsed.resultChartType;
        source.button = parsed.button;
        source.buttonId = parsed.buttonId;
        source.buttonActionCallback = parsed.buttonActionCallback;
        source.onMouse = parsed.onMouse;
        source.mouseCursor = parsed.mouseCursor;
        source.slider = parsed.slider;
        source.sliderDirection = parsed.sliderDirection;
        source.sliderRange = parsed.sliderRange;
        source.sliderType = parsed.sliderType;
        source.sliderDisabled = parsed.sliderDisabled;
        source.sliderRefNumber = parsed.sliderRefNumber;
        source.sliderMinValue = parsed.sliderMinValue;
        source.sliderMaxValue = parsed.sliderMaxValue;
        source.sliderValueCallback = parsed.sliderValueCallback;
        source.sliderEventCallback = parsed.sliderEventCallback;
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
        source.timerCallback = parsed.timerCallback;
        source.side = parsed.side;
        source.constantValueEnabled = parsed.constantValueEnabled;
        source.constantValue = parsed.constantValue;
        source.valueCallback = parsed.valueCallback;
        source.path = parsed.source;
        return true;
    }

    if (value.canConvert<Lr2SrcBarGraph>()) {
        const auto parsed = value.value<Lr2SrcBarGraph>();
        source.valid = true;
        source.x = parsed.x;
        source.y = parsed.y;
        source.w = parsed.w;
        source.h = parsed.h;
        source.divX = std::max(1, parsed.div_x);
        source.divY = std::max(1, parsed.div_y);
        source.cycle = parsed.cycle;
        source.timer = parsed.timer;
        source.timerCallback = parsed.timerCallback;
        source.graphType = parsed.graphType;
        source.graphDirection = parsed.direction;
        source.graphRefNumber = parsed.graphRefNumber;
        source.graphMinValue = parsed.graphMinValue;
        source.graphMaxValue = parsed.graphMaxValue;
        source.valueCallback = parsed.valueCallback;
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
        source.st = parsed.st;
        source.valueCallback = parsed.valueCallback;
        source.constantText = parsed.constantText;
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
        source.timerCallback = mapInt(map, QStringLiteral("timerCallback"), 0);
        source.resultChartType = mapInt(map, QStringLiteral("resultChartType"), 0);
        source.button = mapBool(map, QStringLiteral("button"), false);
        source.buttonId = mapInt(map, QStringLiteral("buttonId"), 0);
        source.buttonActionCallback = mapInt(map, QStringLiteral("buttonActionCallback"), 0);
        source.onMouse = mapBool(map, QStringLiteral("onMouse"), false);
        source.mouseCursor = mapBool(map, QStringLiteral("mouseCursor"), false);
        source.slider = mapBool(map, QStringLiteral("slider"), false);
        source.sliderDirection = mapInt(map, QStringLiteral("sliderDirection"), 0);
        source.sliderRange = mapInt(map, QStringLiteral("sliderRange"), 0);
        source.sliderType = mapInt(map, QStringLiteral("sliderType"), 0);
        source.sliderDisabled = mapInt(map, QStringLiteral("sliderDisabled"), 0);
        source.sliderRefNumber = mapBool(map, QStringLiteral("sliderRefNumber"), false);
        source.sliderMinValue = mapInt(map, QStringLiteral("sliderMinValue"), 0);
        source.sliderMaxValue = mapInt(map, QStringLiteral("sliderMaxValue"), 0);
        source.sliderValueCallback = mapInt(map, QStringLiteral("sliderValueCallback"), 0);
        source.sliderEventCallback = mapInt(map, QStringLiteral("sliderEventCallback"), 0);
        source.imageSet = mapBool(map, QStringLiteral("imageSet"), false);
        source.side = mapInt(map, QStringLiteral("side"), 0);
        source.hasKind = mapHas(map, QStringLiteral("kind"));
        source.kind = mapInt(map, QStringLiteral("kind"), 0);
        source.graphType = mapInt(map, QStringLiteral("graphType"), 0);
        source.graphDirection = mapInt(map, QStringLiteral("direction"), 0);
        source.graphRefNumber = mapBool(map, QStringLiteral("graphRefNumber"), false);
        source.graphMinValue = mapInt(map, QStringLiteral("graphMinValue"), 0);
        source.graphMaxValue = mapInt(map, QStringLiteral("graphMaxValue"), 0);
        source.specialType = mapInt(map, QStringLiteral("specialType"), 0);
        source.constantValueEnabled =
            mapBool(map, QStringLiteral("constantValueEnabled"), false);
        source.constantValue = mapInt(map, QStringLiteral("constantValue"), 0);
        source.valueCallback = mapInt(map, QStringLiteral("valueCallback"), 0);
        source.constantText = map.value(QStringLiteral("constantText")).toString();
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
    source.timerCallback = jsInt(jsValue, QStringLiteral("timerCallback"), 0);
    source.resultChartType = jsInt(jsValue, QStringLiteral("resultChartType"), 0);
    source.button = jsBool(jsValue, QStringLiteral("button"), false);
    source.buttonId = jsInt(jsValue, QStringLiteral("buttonId"), 0);
    source.buttonActionCallback = jsInt(jsValue, QStringLiteral("buttonActionCallback"), 0);
    source.onMouse = jsBool(jsValue, QStringLiteral("onMouse"), false);
    source.mouseCursor = jsBool(jsValue, QStringLiteral("mouseCursor"), false);
    source.slider = jsBool(jsValue, QStringLiteral("slider"), false);
    source.sliderDirection = jsInt(jsValue, QStringLiteral("sliderDirection"), 0);
    source.sliderRange = jsInt(jsValue, QStringLiteral("sliderRange"), 0);
    source.sliderType = jsInt(jsValue, QStringLiteral("sliderType"), 0);
    source.sliderDisabled = jsInt(jsValue, QStringLiteral("sliderDisabled"), 0);
    source.sliderRefNumber = jsBool(jsValue, QStringLiteral("sliderRefNumber"), false);
    source.sliderMinValue = jsInt(jsValue, QStringLiteral("sliderMinValue"), 0);
    source.sliderMaxValue = jsInt(jsValue, QStringLiteral("sliderMaxValue"), 0);
    source.sliderValueCallback = jsInt(jsValue, QStringLiteral("sliderValueCallback"), 0);
    source.sliderEventCallback = jsInt(jsValue, QStringLiteral("sliderEventCallback"), 0);
    source.imageSet = jsBool(jsValue, QStringLiteral("imageSet"), false);
    source.side = jsInt(jsValue, QStringLiteral("side"), 0);
    source.hasKind = jsHas(jsValue, QStringLiteral("kind"));
    source.kind = jsInt(jsValue, QStringLiteral("kind"), 0);
    source.graphType = jsInt(jsValue, QStringLiteral("graphType"), 0);
    source.graphDirection = jsInt(jsValue, QStringLiteral("direction"), 0);
    source.graphRefNumber = jsBool(jsValue, QStringLiteral("graphRefNumber"), false);
    source.graphMinValue = jsInt(jsValue, QStringLiteral("graphMinValue"), 0);
    source.graphMaxValue = jsInt(jsValue, QStringLiteral("graphMaxValue"), 0);
    source.specialType = jsInt(jsValue, QStringLiteral("specialType"), 0);
    source.constantValueEnabled =
        jsBool(jsValue, QStringLiteral("constantValueEnabled"), false);
    source.constantValue = jsInt(jsValue, QStringLiteral("constantValue"), 0);
    source.valueCallback = jsInt(jsValue, QStringLiteral("valueCallback"), 0);
    const QJSValue constantText = jsValue.property(QStringLiteral("constantText"));
    source.constantText = constantText.isUndefined() || constantText.isNull()
        ? QString {}
        : constantText.toString();
    const QJSValue sourcePath = jsValue.property(QStringLiteral("source"));
    source.path = sourcePath.isUndefined() || sourcePath.isNull()
        ? QString {}
        : sourcePath.toString();
    return true;
}

DstAnalysis analyzeDsts(const QVector<Dst>& dsts) {
    DstAnalysis analysis;
    if (dsts.isEmpty() || !dsts.front().valid) {
        return analysis;
    }

    const Dst& first = dsts.front();
    analysis.firstTimer = first.timer;
    analysis.firstTimerCallback = first.timerCallback;
    analysis.firstSortId = first.sortId;
    analysis.usesDynamicTimer = first.timer != 0 || first.timerCallback > 0;
    analysis.usesActiveOptions = first.op1 != 0 || first.op2 != 0 || first.op3 != 0;
    analysis.scratchRotationSide = first.op4 == 1 || first.op4 == 2 ? first.op4 : 0;
    analysis.canUseStaticState = dsts.size() == 1
        && first.time <= 0
        && first.timer == 0
        && first.timerCallback == 0
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
    if (first.timer != 0 || first.timerCallback > 0) {
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
    state.stretch = d1->stretch;
    state.hasMouseRect = first.hasMouseRect;
    state.mouseRectX = first.mouseRectX;
    state.mouseRectY = first.mouseRectY;
    state.mouseRectW = first.mouseRectW;
    state.mouseRectH = first.mouseRectH;
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
    state.stretch = dst.stretch;
    state.hasMouseRect = controlDst.hasMouseRect;
    state.mouseRectX = controlDst.mouseRectX;
    state.mouseRectY = controlDst.mouseRectY;
    state.mouseRectW = controlDst.mouseRectW;
    state.mouseRectH = controlDst.mouseRectH;
    return state;
}

QVariant stateToVariant(const State& state) {
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
    value.insert(QStringLiteral("stretch"), state.stretch);
    value.insert(QStringLiteral("hasMouseRect"), state.hasMouseRect);
    value.insert(QStringLiteral("mouseRectX"), state.mouseRectX);
    value.insert(QStringLiteral("mouseRectY"), state.mouseRectY);
    value.insert(QStringLiteral("mouseRectW"), state.mouseRectW);
    value.insert(QStringLiteral("mouseRectH"), state.mouseRectH);
    return value;
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
        && lhs.op4 == rhs.op4
        && lhs.stretch == rhs.stretch
        && lhs.hasMouseRect == rhs.hasMouseRect
        && lhs.mouseRectX == rhs.mouseRectX
        && lhs.mouseRectY == rhs.mouseRectY
        && lhs.mouseRectW == rhs.mouseRectW
        && lhs.mouseRectH == rhs.mouseRectH;
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

bool sourceUsesDynamicTimer(const Source& source) {
    return source.valid
        && (source.timer != 0 || source.timerCallback > 0)
        && source.cycle > 0;
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

    if (source.sliderValueCallback > 0) {
        return LuaValueSliderSpriteStateOverride;
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

QVariant sliderTrackState(const Source& source, const State& baseState) {
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

    QVariantMap value;
    value.insert(QStringLiteral("x"), track.x);
    value.insert(QStringLiteral("y"), track.y);
    value.insert(QStringLiteral("w"), track.w);
    value.insert(QStringLiteral("h"), track.h);
    return value;
}

qreal sliderPositionFromPointer(const Source& source,
                                const QVariant& track,
                                qreal pointerX,
                                qreal pointerY) {
    if (!source.valid || !source.slider || !track.canConvert<QVariantMap>()) {
        return 0.0;
    }

    const QVariantMap state = track.toMap();
    const qreal x = state.value(QStringLiteral("x")).toReal();
    const qreal y = state.value(QStringLiteral("y")).toReal();
    const qreal w = state.value(QStringLiteral("w")).toReal();
    const qreal h = state.value(QStringLiteral("h")).toReal();
    const qreal range = std::max(1, source.sliderRange);
    const qreal moveX = (w - range) / 2.0;
    const qreal moveY = (h - range) / 2.0;
    qreal position = 0.0;

    switch (source.sliderDirection) {
    case 0: {
        const qreal start = y + moveY;
        const qreal end = y + h - moveY;
        position = end != start ? (end - pointerY) / (end - start) : 0.0;
        break;
    }
    case 1: {
        const qreal start = x + moveX;
        const qreal end = x + w - moveX;
        position = end != start ? (pointerX - start) / (end - start) : 0.0;
        break;
    }
    case 2: {
        const qreal start = y + moveY;
        const qreal end = y + h - moveY;
        position = end != start ? (pointerY - start) / (end - start) : 0.0;
        break;
    }
    case 3: {
        const qreal start = x + moveX;
        const qreal end = x + w - moveX;
        position = end != start ? (end - pointerX) / (end - start) : 0.0;
        break;
    }
    default:
        return 0.0;
    }

    return std::clamp(position, 0.0, 1.0);
}

bool rectContains(const QVariant& state, qreal skinX, qreal skinY) {
    if (!state.isValid() || state.isNull() || !state.canConvert<QVariantMap>()) {
        return false;
    }

    const QVariantMap map = state.toMap();
    const qreal x = map.value(QStringLiteral("x")).toReal();
    const qreal y = map.value(QStringLiteral("y")).toReal();
    const qreal w = map.value(QStringLiteral("w")).toReal();
    const qreal h = map.value(QStringLiteral("h")).toReal();
    const qreal left = std::min(x, x + w);
    const qreal right = std::max(x, x + w);
    const qreal top = std::min(y, y + h);
    const qreal bottom = std::max(y, y + h);
    return skinX >= left && skinX <= right && skinY >= top && skinY <= bottom;
}

} // namespace lr2skin::runtime
