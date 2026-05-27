.pragma library

function isChartAssetSource(source) {
    return !!source
        && (source.specialType === 1
            || source.specialType === 3
            || source.specialType === 4);
}

function sourceFrameCount(source) {
    if (!source) {
        return 1;
    }
    return Math.max(1, source.div_x || 1) * Math.max(1, source.div_y || 1);
}

function sourceCyclesContinuously(source) {
    return !!source
        && (source.cycle || 0) > 0
        && sourceFrameCount(source) > 1;
}

function sourceTreeHas(value, predicate, depth) {
    if (!value || depth > 4) {
        return false;
    }
    if (predicate(value)) {
        return true;
    }
    if (value.length !== undefined && typeof value !== "string") {
        for (let i = 0; i < value.length; ++i) {
            if (sourceTreeHas(value[i], predicate, depth + 1)) {
                return true;
            }
        }
    }
    if (value.source && sourceTreeHas(value.source, predicate, depth + 1)) {
        return true;
    }
    if (value.sources && sourceTreeHas(value.sources, predicate, depth + 1)) {
        return true;
    }
    return false;
}

function sourceTreeCyclesContinuously(value) {
    return sourceTreeHas(value, sourceCyclesContinuously, 0);
}

function sourceTreeUsesChartAsset(value) {
    return sourceTreeHas(value, isChartAssetSource, 0);
}

function chartAssetSourceFor(source, stageFileSource, backBmpSource, bannerSource) {
    if (!source) {
        return "";
    }
    if (source.specialType === 1) {
        return stageFileSource;
    }
    if (source.specialType === 3) {
        return backBmpSource;
    }
    if (source.specialType === 4) {
        return bannerSource;
    }
    return "";
}

function chartAssetFileName(source, chartData) {
    if (!source || !chartData) {
        return "";
    }
    if (source.specialType === 1) {
        return chartData.stageFile || "";
    }
    if (source.specialType === 3) {
        return chartData.backBmp || "";
    }
    if (source.specialType === 4) {
        return chartData.banner || "";
    }
    return "";
}

function fileUrlForPath(path) {
    if (!path) {
        return "";
    }
    const normalized = String(path).replace(/\\/g, "/");
    if (/^[A-Za-z]:\//.test(normalized)) {
        return "file:///" + normalized;
    }
    if (normalized.startsWith("/")) {
        return "file://" + normalized;
    }
    return normalized;
}

function resolvedSource(source, chart, chartAssetSource) {
    if (!source) {
        return "";
    }
    if (isChartAssetSource(source)) {
        if (chartAssetSource && chartAssetSource.length > 0) {
            return chartAssetSource;
        }
        const chartData = chart ? (chart.chartData || chart) : null;
        const fileName = chartAssetFileName(source, chartData);
        if (!chartData || !fileName || !chartData.chartDirectory) {
            return "";
        }
        let dir = chartData.chartDirectory;
        if (dir[0] !== "/") {
            dir = "/" + dir;
        }
        return "file://" + dir + fileName.replace(/\.[^/.]+$/, "");
    }
    return fileUrlForPath(source.source);
}

function normalizedBlendMode(rawBlendMode, colorKeyEnabled, supportsInvertedBlend) {
    let raw = rawBlendMode === undefined || rawBlendMode === null ? 1 : rawBlendMode;
    if (raw === 0 && !colorKeyEnabled) {
        return 1;
    }
    if (raw === 5 || raw === 6) {
        return 2;
    }
    if (raw === 10 && !supportsInvertedBlend) {
        return 1;
    }
    if (raw === 3 || raw === 4 || raw === 9 || raw === 11) {
        return 1;
    }
    return raw;
}

function colorComponent(value) {
    if (value === undefined || value === null) {
        return 1.0;
    }
    return Math.max(0, Math.min(255, value)) / 255.0;
}

function centerAnchor(index) {
    switch (index) {
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

function stateValue(state, name, fallback) {
    if (!state || state[name] === undefined || state[name] === null) {
        return fallback;
    }
    return state[name];
}
