pragma ValueTypeBehavior: Addressable

import QtQml
import RhythmGameQml 1.0

QtObject {
    id: root

    required property var context
    property var items: []
    property int logicalCount: 0
    property int visualBaseIndex: 0
    property int barCenter: 0
    property int scoreRevision: 0
    property int folderLampRevision: 0
    property int listRevision: 0

    property var cache: ({})
    property int cacheScoreRevision: -1
    property int cacheFolderLampRevision: -1
    property int cacheListRevision: -1

    function entryForRow(row) {
        if (logicalCount <= 0) {
            return null;
        }
        return items[context.normalizeIndex(visualBaseIndex + row - barCenter)];
    }

    function ensureCurrent() {
        if (cacheScoreRevision === scoreRevision
                && cacheFolderLampRevision === folderLampRevision
                && cacheListRevision === listRevision) {
            return;
        }
        cache = {};
        cacheScoreRevision = scoreRevision;
        cacheFolderLampRevision = folderLampRevision;
        cacheListRevision = listRevision;
    }

    function rankingEntryRank(entry) {
        let points = Number(entry.bestPoints || 0);
        let maxPoints = Number(entry.maxPoints || 0);
        if (maxPoints <= 0) {
            return 0;
        }

        let rank = Math.floor(points * 9 / maxPoints);
        if (rank > 7) {
            rank = 8;
        }
        if (rank < 2 && points > 0) {
            rank = 1;
        }
        return rank;
    }

    function folderDisplayName(path) {
        let normalized = path.replace(/\\/g, "/").replace(/\/$/, "");
        let slash = normalized.lastIndexOf("/");
        return slash >= 0 ? normalized.slice(slash + 1) : normalized;
    }

    function chartDifficulty(item) {
        let cached = context.chartDifficultyCache[item.path || ""];
        if (cached !== undefined) {
            return cached;
        }
        return Math.max(0, item.difficulty || 0);
    }

    function buildCore(item) {
        let valid = !!item;
        let key = "empty";
        let text = "";
        let bodyType = 1;
        let playLevel = 0;
        let difficulty = 0;
        let keymode = 0;
        let ranking = false;
        let chartLike = false;
        let entryLike = false;
        let folderLike = false;
        let lamp = 0;
        let rank = 0;

        if (!item) {
            return {
                key: key,
                valid: false,
                text: text,
                titleType: 0,
                bodyType: bodyType,
                playLevel: playLevel,
                difficulty: difficulty,
                keymode: keymode,
                ranking: false,
                chartLike: false,
                entryLike: false,
                folderLike: false,
                lamp: 0,
                rank: 0
            };
        }

        if (item.__lr2RankingEntry === true) {
            ranking = true;
            key = "ranking:" + (item.sourceMd5 || "") + ":" + (item.rankingIndex || 0);
            text = item.title || "";
            bodyType = 0;
            playLevel = item.level || 0;
            keymode = item.keymode || 0;
            lamp = context.clearTypeLamp(item.bestClearType);
            rank = rankingEntryRank(item);
        } else if (item instanceof ChartData) {
            chartLike = true;
            key = "chart:" + (item.md5 || item.path || item.title || "");
            text = item.subtitle ? (item.title || "") + " " + item.subtitle : (item.title || "");
            bodyType = 0;
            playLevel = item.playLevel || 0;
            difficulty = chartDifficulty(item);
            keymode = item.keymode || 0;
            let summary = context.scoreLampRankForItem(item);
            lamp = summary.lamp;
            rank = summary.rank;
        } else if (item instanceof entry) {
            entryLike = true;
            key = "entry:" + (item.md5 || item.path || item.title || item.level || "");
            text = item.subtitle ? (item.title || "") + " " + item.subtitle : (item.title || "");
            bodyType = 0;
            let parsedLevel = parseInt(item.level);
            playLevel = isNaN(parsedLevel) ? 0 : parsedLevel;
            keymode = item.keymode || 0;
            let summary = context.scoreLampRankForItem(item);
            lamp = summary.lamp;
            rank = summary.rank;
        } else if (item instanceof table) {
            key = "table:" + (item.url || item.name || "");
            text = item.name || "";
            bodyType = 2;
            folderLike = true;
            lamp = context.entryLamp(item);
        } else if (item instanceof level) {
            key = "level:" + context.folderLampKey(item);
            text = context.entryDisplayName(item, true);
            bodyType = 2;
            folderLike = true;
            lamp = context.entryLamp(item);
        } else if (item instanceof course) {
            key = "course:" + (item.identifier || item.name || "");
            text = item.name || "";
            bodyType = 8;
            let summary = context.scoreLampRankForItem(item);
            lamp = summary.lamp;
            rank = summary.rank;
        } else if (typeof item === "string") {
            key = "folder:" + item;
            text = folderDisplayName(item);
            folderLike = true;
            lamp = context.entryLamp(item);
        } else {
            text = context.entryDisplayName(item, true);
            key = "item:" + text;
            keymode = item.keymode || 0;
            let summary = context.scoreLampRankForItem(item);
            lamp = summary.lamp;
            rank = summary.rank;
        }

        return {
            key: key,
            valid: valid,
            text: text,
            titleType: 0,
            bodyType: bodyType,
            playLevel: playLevel,
            difficulty: difficulty,
            keymode: keymode,
            ranking: ranking,
            chartLike: chartLike,
            entryLike: entryLike,
            folderLike: folderLike,
            lamp: lamp,
            rank: rank
        };
    }

    function core(item) {
        ensureCurrent();
        let key = "";
        if (!item) {
            key = "empty";
        } else if (item.__lr2RankingEntry === true) {
            key = "ranking:" + (item.sourceMd5 || "") + ":" + (item.rankingIndex || 0);
        } else if (item instanceof ChartData) {
            key = "chart:" + (item.md5 || item.path || item.title || "");
        } else if (item instanceof entry) {
            key = "entry:" + (item.md5 || item.path || item.title || item.level || "");
        } else if (item instanceof table) {
            key = "table:" + (item.url || item.name || "");
        } else if (item instanceof level) {
            key = "level:" + context.folderLampKey(item);
        } else if (item instanceof course) {
            key = "course:" + (item.identifier || item.name || "");
        } else if (typeof item === "string") {
            key = "folder:" + item;
        } else {
            key = "item:" + context.entryDisplayName(item, true);
        }
        let cached = cache[key];
        if (cached !== undefined) {
            return cached;
        }

        cached = buildCore(item);
        cache[key] = cached;
        return cached;
    }

    function assignCore(cell, row, entry, cachedCore) {
        cell.setCore(
            row,
            !!entry,
            cachedCore.text,
            cachedCore.titleType,
            cachedCore.bodyType,
            cachedCore.playLevel,
            cachedCore.difficulty,
            cachedCore.keymode,
            cachedCore.ranking,
            cachedCore.chartLike,
            cachedCore.entryLike,
            cachedCore.folderLike,
            cachedCore.lamp,
            cachedCore.rank);
    }

    function cellForRow(row) {
        let entry = entryForRow(row);
        let cachedCore = core(entry);
        return {
            row: row,
            entry: entry,
            valid: !!entry,
            text: cachedCore.text,
            titleType: cachedCore.titleType,
            bodyType: cachedCore.bodyType,
            playLevel: cachedCore.playLevel,
            difficulty: cachedCore.difficulty,
            keymode: cachedCore.keymode,
            ranking: cachedCore.ranking,
            chartLike: cachedCore.chartLike,
            entryLike: cachedCore.entryLike,
            folderLike: cachedCore.folderLike,
            lamp: cachedCore.lamp,
            rank: cachedCore.rank
        };
    }

    function updateCellForRow(cell, row) {
        let entry = entryForRow(row);
        assignCore(cell, row, entry, core(entry));
    }

    function updateCell(cell, row, entry) {
        assignCore(cell, row, entry, core(entry));
    }
}
