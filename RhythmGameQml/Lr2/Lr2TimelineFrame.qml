pragma ValueTypeBehavior: Addressable

import RhythmGameQml.Lr2

Lr2TimelineFrameState {
    id: frame

    property var stateOverride: null
    readonly property bool hasStateOverride: !!stateOverride && stateOverride.valid !== false

    stateOverrideEnabled: hasStateOverride
    stateOverrideValue.valid: hasStateOverride
    stateOverrideValue.x: hasStateOverride ? (stateOverride.x || 0) : 0
    stateOverrideValue.y: hasStateOverride ? (stateOverride.y || 0) : 0
    stateOverrideValue.w: hasStateOverride ? (stateOverride.w || 0) : 0
    stateOverrideValue.h: hasStateOverride ? (stateOverride.h || 0) : 0
    stateOverrideValue.a: hasStateOverride
        ? (stateOverride.a === undefined || stateOverride.a === null ? 255 : stateOverride.a)
        : 255
    stateOverrideValue.r: hasStateOverride
        ? (stateOverride.r === undefined || stateOverride.r === null ? 255 : stateOverride.r)
        : 255
    stateOverrideValue.g: hasStateOverride
        ? (stateOverride.g === undefined || stateOverride.g === null ? 255 : stateOverride.g)
        : 255
    stateOverrideValue.b: hasStateOverride
        ? (stateOverride.b === undefined || stateOverride.b === null ? 255 : stateOverride.b)
        : 255
    stateOverrideValue.angle: hasStateOverride ? (stateOverride.angle || 0) : 0
    stateOverrideValue.center: hasStateOverride ? (stateOverride.center || 0) : 0
    stateOverrideValue.blend: hasStateOverride ? (stateOverride.blend || 0) : 0
    stateOverrideValue.filter: hasStateOverride ? (stateOverride.filter || 0) : 0
    stateOverrideValue.op4: hasStateOverride ? (stateOverride.op4 || 0) : 0
}
