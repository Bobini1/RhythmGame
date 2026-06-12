.pragma library

var fileUrlCache = Object.create(null);
var fileUrlCacheSize = 0;
var fileUrlCacheLimit = 4096;
var chartAssetVideoExtensionPattern = /\.(mpg|mpeg|mp4|avi|wmv|mov|mkv|webm|m4v)$/i;

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

function chartAssetLookupFileName(fileName) {
    if (!fileName) {
        return "";
    }

    const normalized = String(fileName).replace(/\\/g, "/");
    const suffixIndex = normalized.search(/[?#]/);
    const pathPart = suffixIndex >= 0 ? normalized.slice(0, suffixIndex) : normalized;
    const suffix = suffixIndex >= 0 ? normalized.slice(suffixIndex) : "";
    const lastSlash = pathPart.lastIndexOf("/");
    const lastDot = pathPart.lastIndexOf(".");
    if (lastDot <= lastSlash + 1 || lastDot >= pathPart.length - 1) {
        return normalized;
    }
    if (chartAssetVideoExtensionPattern.test(pathPart)) {
        return normalized;
    }
    return pathPart.slice(0, lastDot) + suffix;
}

function chartAssetPath(chartData, fileName) {
    if (!chartData || !fileName || !chartData.chartDirectory) {
        return "";
    }

    const filePath = chartAssetLookupFileName(fileName);
    if (/^[A-Za-z]:\//.test(filePath) || filePath.startsWith("/")) {
        return filePath;
    }

    let dir = String(chartData.chartDirectory).replace(/\\/g, "/");
    if (dir.length > 0 && !dir.endsWith("/")) {
        dir += "/";
    }
    return dir + filePath;
}

function chartAssetUrl(chartData, fileName) {
    return fileUrlForPath(chartAssetPath(chartData, fileName));
}

function fileUrlForPath(path) {
    if (!path) {
        return "";
    }
    const pathKey = String(path);
    const cached = fileUrlCache[pathKey];
    if (cached !== undefined) {
        return cached;
    }

    const normalized = pathKey.replace(/\\/g, "/");
    const encoded = normalized.split("/").map((part, index) => {
        if (index === 0 && /^[A-Za-z]:$/.test(part)) {
            return part;
        }
        return encodeURIComponent(part);
    }).join("/");
    let result = "";
    if (/^[A-Za-z]:\//.test(normalized)) {
        result = "file:///" + encoded;
    } else if (normalized.startsWith("/")) {
        result = "file://" + encoded;
    } else {
        result = normalized.replace(/#/g, "%23");
    }

    if (fileUrlCacheSize >= fileUrlCacheLimit) {
        fileUrlCache = Object.create(null);
        fileUrlCacheSize = 0;
    }
    fileUrlCache[pathKey] = result;
    ++fileUrlCacheSize;
    return result;
}

function sanitizeFilename(value) {
    return String(value || "").replace(/[/\\:*?"<>|]/g, "_");
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
        return chartAssetUrl(chartData, fileName);
    }
    return fileUrlForPath(source.source);
}

function normalizedBlendMode(rawBlendMode, colorKeyEnabled, supportsInvertedBlend) {
    let raw = rawBlendMode === undefined || rawBlendMode === null ? 1 : rawBlendMode;
    if (raw === 0 && !colorKeyEnabled) {
        return 1;
    }
    if (raw === 10 && !supportsInvertedBlend) {
        return 1;
    }
    if (raw === 3 || raw === 4 || raw === 5 || raw === 6 || raw === 9 || raw === 11) {
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

function hexColor(value, fallback) {
    let raw = value === undefined || value === null ? "" : String(value);
    raw = raw.replace(/[^0-9a-fA-F]/g, "");
    if (raw.length < 6) {
        return fallback;
    }
    return "#" + raw.substring(0, 6);
}

function skinTimeForClock(clock, clockMode, fallbackSkinTime) {
    if (!clock || clockMode === 0) {
        return fallbackSkinTime;
    }
    switch (clockMode) {
    case 1: return clock.renderSkinTime;
    case 2: return clock.selectSourceSkinTime;
    case 3: return clock.barSkinTime;
    case 4: return clock.globalSkinTime;
    case 5: return clock.selectLiveSkinTime;
    case 6: return clock.selectInfoElapsed;
    default: return fallbackSkinTime;
    }
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
