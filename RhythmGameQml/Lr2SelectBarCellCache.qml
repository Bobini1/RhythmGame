pragma ValueTypeBehavior: Addressable

import QtQml

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

    function cacheKey(entry) {
        if (!entry) {
            return "empty";
        }
        if (context.isRankingEntry(entry)) {
            return "ranking:" + (entry.sourceMd5 || "") + ":" + (entry.rankingIndex || 0);
        }
        if (context.isChart(entry)) {
            return "chart:" + (entry.md5 || entry.path || entry.title || "");
        }
        if (context.isEntry(entry)) {
            return "entry:" + (entry.md5 || entry.path || entry.title || entry.level || "");
        }
        if (context.isCourse(entry)) {
            return "course:" + (entry.identifier || entry.name || "");
        }
        if (context.isTable(entry)) {
            return "table:" + (entry.url || entry.name || "");
        }
        if (context.isLevel(entry)) {
            return "level:" + context.folderLampKey(entry);
        }
        if (typeof entry === "string") {
            return "folder:" + entry;
        }
        return "item:" + context.entryDisplayName(entry, true);
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

    function buildCore(entry) {
        let ranking = context.isRankingEntry(entry);
        let chartLike = context.isChart(entry);
        let entryLike = context.isEntry(entry);
        let folderLike = context.isFolderLikeForLamp(entry);
        let lamp = 0;
        let rank = 0;

        if (ranking) {
            lamp = context.clearTypeLamp(entry.bestClearType);
            rank = rankingEntryRank(entry);
        } else if (folderLike) {
            lamp = context.entryLamp(entry);
        } else if (entry) {
            let summary = context.scoreSummaryForItem(entry);
            lamp = summary.lamp;
            rank = summary.rank;
        }

        return {
            valid: !!entry,
            text: context.entryDisplayName(entry, true),
            titleType: context.entryTitleType(entry),
            bodyType: context.entryBodyType(entry),
            playLevel: context.entryPlayLevel(entry),
            difficulty: context.entryDifficulty(entry),
            keymode: entry ? (entry.keymode || 0) : 0,
            ranking: ranking,
            chartLike: chartLike,
            entryLike: entryLike,
            folderLike: folderLike,
            lamp: lamp,
            rank: rank
        };
    }

    function core(entry) {
        ensureCurrent();
        let key = cacheKey(entry);
        let cached = cache[key];
        if (cached !== undefined) {
            return cached;
        }

        cached = buildCore(entry);
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
