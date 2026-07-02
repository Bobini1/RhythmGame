.pragma library

function clamp(value, minValue, maxValue) {
    return Math.max(minValue, Math.min(maxValue, value));
}

function luminance(color) {
    function channel(value) {
        return value <= 0.03928 ? value / 12.92 : Math.pow((value + 0.055) / 1.055, 2.4);
    }

    return 0.2126 * channel(color.r) + 0.7152 * channel(color.g) + 0.0722 * channel(color.b);
}

function isLight(color) {
    return luminance(color) > 0.5;
}

function contrastText(color) {
    return isLight(color) ? Qt.rgba(0.06, 0.06, 0.06, 1) : Qt.rgba(1, 1, 1, 1);
}

function alpha(color, opacity) {
    return Qt.rgba(color.r, color.g, color.b, clamp(opacity, 0, 1));
}

function blend(foreground, background, amount) {
    var ratio = clamp(amount, 0, 1);
    var inverse = 1 - ratio;
    return Qt.rgba(
        foreground.r * ratio + background.r * inverse,
        foreground.g * ratio + background.g * inverse,
        foreground.b * ratio + background.b * inverse,
        foreground.a * ratio + background.a * inverse
    );
}

function panel(palette) {
    return blend(palette.base, palette.window, isLight(palette.window) ? 0.78 : 0.62);
}

function panelBorder(palette) {
    return alpha(palette.mid, isLight(palette.window) ? 0.55 : 0.5);
}

function rowFill(palette, selected, hovered) {
    if (selected) {
        return alpha(palette.highlight, isLight(palette.window) ? 0.18 : 0.28);
    }
    if (hovered) {
        return blend(palette.button, palette.base, isLight(palette.window) ? 0.7 : 0.52);
    }
    return alpha(palette.base, 0);
}

function subtleFill(palette) {
    return blend(palette.button, palette.window, isLight(palette.window) ? 0.62 : 0.46);
}

function chipFill(palette) {
    return blend(palette.button, palette.base, isLight(palette.window) ? 0.64 : 0.48);
}

function primaryFill(palette, pressed, hovered) {
    var base = palette.highlight;
    if (pressed) {
        return blend(palette.shadow, base, isLight(base) ? 0.18 : 0.28);
    }
    if (hovered) {
        return blend(palette.light, base, isLight(base) ? 0.12 : 0.18);
    }
    return base;
}

function secondaryFill(palette, pressed, hovered) {
    var base = blend(palette.highlight, palette.button, isLight(palette.window) ? 0.16 : 0.24);
    if (pressed) {
        return blend(palette.shadow, base, 0.12);
    }
    if (hovered) {
        return blend(palette.highlight, base, 0.12);
    }
    return base;
}

function tertiaryFill(palette, pressed, hovered) {
    if (pressed) {
        return blend(palette.mid, palette.button, isLight(palette.window) ? 0.22 : 0.3);
    }
    if (hovered) {
        return subtleFill(palette);
    }
    return alpha(palette.button, 0);
}

function dangerFill(palette, pressed, hovered) {
    var danger = isLight(palette.window) ? Qt.rgba(0.78, 0.12, 0.16, 1) : Qt.rgba(1, 0.38, 0.42, 1);
    var base = blend(danger, palette.button, isLight(palette.window) ? 0.14 : 0.2);
    if (pressed) {
        return blend(danger, base, 0.18);
    }
    if (hovered) {
        return blend(danger, base, 0.1);
    }
    return base;
}

function dangerText(palette) {
    return isLight(palette.window) ? Qt.rgba(0.62, 0.04, 0.08, 1) : Qt.rgba(1, 0.58, 0.6, 1);
}
