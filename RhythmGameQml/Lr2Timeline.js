.pragma library

// LR2 skin timeline interpolation.
// Ported from Lunatic Vibes f's SpriteBase::updateMotion() in src/game/graphics/sprite.cpp.
//
// Semantics (matching LR2):
//   * dsts[0] is authoritative for `timer`, `loop`, and `op1..op4`.
//     Subsequent dsts inherit those fields; only their time/rect/color/blend/etc. matter.
//   * `timer` selects a stopwatch; animation clock = globalSkinTime - timerFireTime[timer].
//     If the timer has not fired yet (fireTime < 0 or undefined), the sprite does not render.
//   * `loop` is a loopTo point in animation time, not a period:
//       loop <  0           -> play once, then disappear
//       loop >= endTime     -> play once, freeze at last frame
//       0 <= loop < endTime -> play once, then loop the [loop, endTime] segment
//   * `op1..op3` gate the entire sprite's visibility. op=0 means no condition.
//     Negative op means "option must NOT be active". OpenLR2 leaves op4 out
//     of the gate check for images and uses it for scratch-disc rotation.
//   * Before first keyframe: not drawn.
//   * Interpolation uses acc from the *earlier* keyframe of the pair.

function checkSingleOp(op, activeOptions) {
    if (!op || op === 0) return true;
    var negate = op < 0;
    var idx = Math.abs(op);
    var present = activeOptions && activeOptions.indexOf(idx) !== -1;
    return negate ? !present : present;
}

function allOpsMatch(firstDst, activeOptions) {
    if (!firstDst) return false;
    return checkSingleOp(firstDst.op1, activeOptions)
        && checkSingleOp(firstDst.op2, activeOptions)
        && checkSingleOp(firstDst.op3, activeOptions);
}

function applyAccel(progress, accType) {
    // LR2 acc types: 0=linear, 1=accel(ease-in), 2=decel(ease-out), 3=discontinuous
    if (accType === 1) return progress * progress;
    if (accType === 2) {
        var inv = 1.0 - progress;
        return 1.0 - inv * inv;
    }
    if (accType === 3) return 0.0; // hold at start frame until next keyframe
    return progress;
}

function copyDstAsState(dst, controlDst) {
    var control = controlDst || dst;
    return {
        x: dst.x, y: dst.y, w: dst.w, h: dst.h,
        a: dst.a, r: dst.r, g: dst.g, b: dst.b,
        angle: dst.angle || 0,
        center: dst.center || 0,
        sortId: dst.sortId || 0,
        blend: dst.blend || 0,
        filter: dst.filter || 0,
        op1: control.op1 || 0,
        op2: control.op2 || 0,
        op3: control.op3 || 0,
        op4: control.op4 || 0
    };
}

function canUseStaticState(dsts) {
    if (!dsts || dsts.length !== 1 || !dsts[0]) return false;
    var first = dsts[0];
    return (first.time || 0) <= 0
        && (first.timer || 0) === 0
        && (first.op1 || 0) === 0
        && (first.op2 || 0) === 0
        && (first.op3 || 0) === 0;
}

function dstsUseActiveOptions(dsts) {
    if (!dsts || dsts.length === 0 || !dsts[0]) return false;
    var first = dsts[0];
    return (first.op1 || 0) !== 0
        || (first.op2 || 0) !== 0
        || (first.op3 || 0) !== 0;
}

function dstsUseDynamicTimer(dsts) {
    if (!dsts || dsts.length === 0 || !dsts[0]) return false;
    return (dsts[0].timer || 0) !== 0;
}

function srcUsesDynamicTimer(src) {
    if (!src) return false;
    return (src.timer || 0) !== 0 && (src.cycle || 0) > 0;
}

// Map a timer index to the time it fired (ms since app start / some global clock).
// `timers` is expected to be an object like { 0: 0 } meaning "timer 0 fired at t=0".
// Returns -1 if the timer has not fired.
function getTimerFire(timers, timerIdx) {
    if (!timers) {
        // Fallback: treat all timers as firing at 0.
        return 0;
    }
    var t = timers[timerIdx];
    if (t === undefined || t === null) return -1;
    return t;
}

// Returns an object describing the sprite's current visual state, or null if invisible.
// `globalTime` is the monotonic skin clock (ms).
// `timers` is a map of { timerIdx: fireTimeMs }.
// `activeOptions` is an array of active LR2 option IDs.
function getCurrentState(dsts, globalTime, timers, activeOptions) {
    if (!dsts || dsts.length === 0) return null;
    var first = dsts[0];
    if (!first) return null;

    if (!allOpsMatch(first, activeOptions)) return null;

    var timerFire = getTimerFire(timers, first.timer || 0);
    if (timerFire < 0) return null;

    var time = globalTime - timerFire;

    // Sprite is not drawn before its first keyframe.
    if (time < first.time) return null;

    var last = dsts[dsts.length - 1];
    var endTime = last.time;
    var loopTo = first.loop;

    if (dsts.length === 1) {
        // Single keyframe: hold it forever (loop<0 would hide, but that's degenerate here).
        return copyDstAsState(first, first);
    }

    if (time >= endTime) {
        if (loopTo < 0) return null;
        if (loopTo >= endTime) {
            time = endTime;
        } else {
            var period = endTime - loopTo;
            time = loopTo + ((time - loopTo) % period);
        }
    }

    if (time >= endTime) {
        return copyDstAsState(last, first);
    }

    // Find the surrounding keyframe pair.
    var d1 = dsts[0], d2 = dsts[0];
    for (var i = 0; i < dsts.length - 1; ++i) {
        if (time >= dsts[i].time && time < dsts[i + 1].time) {
            d1 = dsts[i];
            d2 = dsts[i + 1];
            break;
        }
    }

    var segment = d2.time - d1.time;
    if (segment <= 0) return copyDstAsState(d1, first);

    var progress = applyAccel((time - d1.time) / segment, d1.acc || 0);
    function mix(a, b) { return a + (b - a) * progress; }

    return {
        x: mix(d1.x, d2.x),
        y: mix(d1.y, d2.y),
        w: mix(d1.w, d2.w),
        h: mix(d1.h, d2.h),
        a: mix(d1.a, d2.a),
        r: mix(d1.r, d2.r),
        g: mix(d1.g, d2.g),
        b: mix(d1.b, d2.b),
        angle: mix(d1.angle || 0, d2.angle || 0),
        center: d1.center || 0,
        sortId: mix(d1.sortId || 0, d2.sortId || 0),
        blend: d1.blend || 0,
        filter: d1.filter || 0,
        op1: first.op1 || 0,
        op2: first.op2 || 0,
        op3: first.op3 || 0,
        op4: first.op4 || 0
    };
}

// Computes the current animation frame index for a sprite sheet (div_x * div_y cells cycling over `cycle` ms).
// `srcTimerFire` is the fire time of src.timer; if negative, the animation is paused at frame 0.
function getAnimationFrame(src, globalTime, srcTimerFire) {
    if (!src) return 0;
    var divX = Math.max(1, src.div_x || 1);
    var divY = Math.max(1, src.div_y || 1);
    var frames = divX * divY;
    if (frames <= 1 || !src.cycle || src.cycle <= 0) return 0;
    if (srcTimerFire === undefined || srcTimerFire < 0) return 0;
    var animTime = globalTime - srcTimerFire;
    if (animTime < 0) return 0;
    var msPerFrame = src.cycle / frames;
    if (msPerFrame < 1) return 0;
    return Math.floor((animTime % src.cycle) / msPerFrame);
}

// LR2 center index -> (anchorX, anchorY) in [0,1] within the sprite's own rect.
// OpenLR2/DxLib uses bottom-row first, and treats 0, 5, and out-of-range as center.
//   1 = bottom-left, 2 = bottom-center, 3 = bottom-right
//   4 = middle-left, default/0/5 = middle-center, 6 = middle-right
//   7 = top-left, 8 = top-center, 9 = top-right
function centerAnchor(idx) {
    switch (idx) {
    case 1: return { x: 0.0, y: 1.0 };
    case 2: return { x: 0.5, y: 1.0 };
    case 3: return { x: 1.0, y: 1.0 };
    case 4: return { x: 0.0, y: 0.5 };
    case 6: return { x: 1.0, y: 0.5 };
    case 7: return { x: 0.0, y: 0.0 };
    case 8: return { x: 0.5, y: 0.0 };
    case 9: return { x: 1.0, y: 0.0 };
    default: return { x: 0.5, y: 0.5 };
    }
}
