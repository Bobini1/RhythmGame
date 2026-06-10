pragma ValueTypeBehavior: Addressable
import QtQuick
import RhythmGameQml

QtObject {
    id: resolver

    required property var screenRoot
    required property var selectContext
    required property var playContext
    required property var rankingState
    property Lr2ResolvedTextRegistry textRegistry: null
    property bool resolvedTextRefreshQueued: false
    property bool resolvedTextRefreshAllQueued: false
    property var resolvedTextRefreshPendingIds: ({})

    readonly property var root: screenRoot

    function numericValue(value: var, fallback: var) : var {
        const numeric = Number(value);
        return isNaN(numeric) ? fallback : numeric;
    }

    function integerPart(value: var) : var {
        const numeric = Number(value);
        if (!isFinite(numeric)) {
            return 0;
        }
        return numeric < 0 ? Math.ceil(numeric) : Math.floor(numeric);
    }

    function playerTotalStatNumber(num: var) : var {
        let stats = selectContext.playerStats || {};
        switch (num) {
        case 30:
            return stats.playCount || 0;
        case 31:
            return stats.clearCount || 0;
        case 32:
            return stats.failCount || 0;
        case 33:
            return stats.perfectCount || 0;
        case 34:
            return stats.greatCount || 0;
        case 35:
            return stats.goodCount || 0;
        case 36:
            return stats.badCount || 0;
        case 37:
            return stats.poorCount || 0;
        case 333:
            return (stats.perfectCount || 0)
                + (stats.greatCount || 0)
                + (stats.goodCount || 0)
                + (stats.badCount || 0);
        default:
            return undefined;
        }
    }

    function resolveBeatorajaGlobalNumber(num: var) : var {
        switch (num) {
        case 12: {
            let vars = root.mainGeneralVarsRef;
            return vars ? Math.round(vars.offset || 0) : 0;
        }
        case 20:
        case 21:
        case 22:
        case 23:
        case 24:
        case 25:
        case 26:
        case 27:
        case 28:
        case 29:
            return root.dateTimeNumber(num);
        default:
            return resolver.playerTotalStatNumber(num);
        }
    }

    function optionOnlyRankId(id: var) : var {
        return id >= 340 && id <= 347;
    }

    function optionText(labels: var, index: var) : var {
        return index >= 0 && index < labels.length ? labels[index] : "";
    }

    function clearLabelForLamp(lamp: var) : var {
        switch (lamp) {
        case 1:
            return "FAILED";
        case 2:
            return "EASY";
        case 3:
            return "NORMAL";
        case 4:
            return "HARD";
        case 5:
            return "FULLCOMBO";
        default:
            return "NO PLAY";
        }
    }

    function courseStage(index: var) : var {
        return index >= 0 && index < root.selectedCourseStages.length
            ? root.selectedCourseStages[index]
            : null;
    }

    function singleLineText(text: var) : var {
        return String(text || "").replace(/\r\n|\n|\r/g, " ");
    }

    function tableEntryTitleForHash(hash: var) : var {
        let matches = Rg.tables.search(String(hash || ""));
        if (!matches || matches.length <= 0) {
            return "";
        }

        let entry = matches[0].entry || {};
        let title = entry.title || "";
        let subtitle = entry.subtitle || "";
        if (title.length <= 0) {
            return "";
        }
        return resolver.singleLineText(title + (subtitle ? " " + subtitle : ""));
    }

    function missingCourseStageTitle(hash: var) : var {
        let title = resolver.tableEntryTitleForHash(hash);
        if (title.length <= 0) {
            title = String(hash || "----");
        }
        return (root.lr2SkinUsesBeatorajaSemantics ? "(no song) " : "(missing) ") + title;
    }

    function missingTableEntryPrefix(chart: var) : var {
        return !root.lr2SkinUsesBeatorajaSemantics
            && selectContext
            && selectContext.isMissingTableEntry(chart)
            ? "(missing) "
            : "";
    }

    function chartTitle(chart: var) : var {
        if (typeof chart === "string") {
            return resolver.missingCourseStageTitle(chart);
        }
        return chart ? resolver.missingTableEntryPrefix(chart) + (chart.title || "") : "";
    }

    function chartFullTitle(chart: var) : var {
        if (typeof chart === "string") {
            return resolver.missingCourseStageTitle(chart);
        }
        if (!chart) {
            return "";
        }
        let title = chart.title || "";
        let subtitle = chart.subtitle || "";
        let fullTitle = resolver.singleLineText(title + (subtitle ? " " + subtitle : ""));
        return resolver.missingTableEntryPrefix(chart) + fullTitle;
    }

    function chartSubtitle(chart: var) : var {
        if (typeof chart === "string") {
            return "";
        }
        return chart ? (chart.subtitle || "") : "";
    }

    function lr2SelectOptionText(st: var) : var {
        const textId = resolver.numericValue(st, -1);
        switch (textId) {
        case 50:
            return root.lr2SkinPreviewTitle();
        case 51:
            return root.lr2SkinPreviewMaker();
        case 63:
            return root.lr2RandomText(1);
        case 64:
            return root.lr2RandomText(2);
        case 65:
            return root.lr2GaugeText(1);
        case 66:
            return root.lr2GaugeText(2);
        case 67:
            return "OFF";
        case 68:
            return "OFF";
        case 69:
            return resolver.optionText(root.lr2BattleLabels, root.lr2BattleIndex);
        case 70:
            return resolver.optionText(["OFF", "FLIP"], root.lr2FlipIndex);
        case 71:
            return resolver.optionText(["OFF", "ON"], root.lr2ScoreGraphIndex);
        case 72:
            return resolver.optionText(root.lr2GhostLabels, root.lr2GhostIndex);
        case 73:
            return resolver.optionText(["OFF", "ON"], root.lr2LaneCoverIndex);
        case 74:
            return resolver.optionText(root.lr2HiSpeedFixLabels, root.lr2HiSpeedFixIndex);
        case 75:
            return resolver.optionText(root.lr2BgaSizeLabels, root.lr2BgaSizeIndex);
        case 76:
            return resolver.optionText(root.lr2BgaLabels, root.lr2BgaIndex);
        case 77:
            return "32BIT";
        case 78:
            return "OFF";
        case 79:
            return globalRoot.isFullScreen() ? "FULLSCREEN" : "WINDOW";
        case 80:
            return "OFF";
        case 81:
            return resolver.optionText(root.lr2ReplayLabels, root.lr2ReplayType);
        case 82:
            return "MISSION COMPLETED";
        case 83:
            return "";
        case 84:
            return resolver.optionText(root.lr2HidSudLabels, root.lr2HidSudIndexP1);
        case 85:
            return resolver.optionText(root.lr2HidSudLabels, root.lr2HidSudIndexP2);
        case 86:
            return root.lr2SkinUsesBeatorajaSemantics ? "RIVALCHART" : "";
        case 308:
            return resolver.optionText(root.lr2LnModeLabels, root.lr2LnModeIndex);
        case 120:
            return Rg.profileList.mainProfile.vars.generalVars.name || "";
        case 121:
        case 122:
        case 123:
        case 124:
            return "";
        case 130:
            return resolver.clearLabelForLamp(selectContext.entryLamp(selectContext.selectedStateItem));
        case 131:
        case 132:
        case 133:
        case 134:
            return "";
        case 150:
        case 151:
        case 152:
        case 153:
        case 154:
        case 155:
        case 156:
        case 157:
        case 158:
        case 159:
            return resolver.chartTitle(resolver.courseStage(st - 150));
        case 160:
        case 161:
        case 162:
        case 163:
        case 164:
            return resolver.chartSubtitle(resolver.courseStage(st - 160));
        case 170:
            return root.effectiveScreenKey === "select" ? selectContext.entryDisplayName(selectContext.selectedStateItem, true) : "";
        case 171:
            return "TITLE";
        case 172:
            return "ARTIST";
        case 173:
            return "GENRE";
        case 174:
            return "TAG";
        case 183:
            return "";
        case 190:
            return "OFF";
        case 191:
        case 192:
        case 193:
        case 194:
        case 195:
            return "NO LIMIT";
        case 196:
            return "RANDOM";
        case 198:
        case 199:
            return "GROOVE";
        default:
            if (textId >= 200 && textId <= 219) {
                return root.lr2TargetNameText(textId);
            }
            if (textId >= 100 && textId < 110) {
                return root.lr2SkinSettingName(textId - 100);
            }
            if (textId >= 110 && textId < 120) {
                return root.lr2SkinSettingValueText(textId - 110);
            }
            return "";
        }
    }

    function selectedTextEntry() : var {
        return root.effectiveScreenKey === "select" ? selectContext.selectedStateItem : null;
    }

    function courseDisplayName() : var {
        if (root.course) {
            return root.course.name || "";
        }
        return root.chart && root.chart.course ? root.chart.course.name : "";
    }

    function beatorajaRivalText() : var {
        return root.effectiveScreenKey === "select" ? "" : root.lr2TargetText();
    }

    function selectTableLevelText() : var {
        let tableItem = selectContext.currentTableItem();
        return root.tableLevelNameForInfo({
            "levelName": selectContext.currentTableLevelName(),
            "symbol": tableItem ? (tableItem.symbol || "") : ""
        });
    }

    function selectTableFullText() : var {
        return resolver.selectTableLevelText() + selectContext.currentTableName();
    }

    function displayTableChart() : var {
        let chart = root.displayChartData();
        return chart || (root.effectiveScreenKey === "select" ? resolver.selectedTextEntry() : null);
    }

    function displayTableName() : var {
        let chartText = root.chartTableName(resolver.displayTableChart());
        return chartText.length > 0
            ? chartText
            : (root.effectiveScreenKey === "select" ? selectContext.currentTableName() : "");
    }

    function displayTableLevelText() : var {
        let chartText = root.chartTableLevelName(resolver.displayTableChart());
        return chartText.length > 0
            ? chartText
            : (root.effectiveScreenKey === "select" ? resolver.selectTableLevelText() : "");
    }

    function displayTableFullText() : var {
        let chartText = root.chartTableFullName(resolver.displayTableChart());
        return chartText.length > 0
            ? chartText
            : (root.effectiveScreenKey === "select" ? resolver.selectTableFullText() : "");
    }

    function chartHashText(chartData: var, field: var) : var {
        return chartData && chartData[field] ? String(chartData[field]) : "";
    }

    function resolveChartText(st: var, chartData: var, selectedEntry: var) : var {
        switch (st) {
        case 10:
            if (root.effectiveScreenKey === "courseResult" && resolver.courseDisplayName().length > 0) {
                return resolver.courseDisplayName();
            }
            if (chartData) {
                return resolver.singleLineText(resolver.chartTitle(chartData));
            }
            if (root.effectiveScreenKey === "select") {
                return root.lr2SkinUsesBeatorajaSemantics && selectContext.isCourse(selectedEntry)
                    ? ""
                    : selectContext.entryMainTitle(selectedEntry);
            }
            return resolver.courseDisplayName();
        case 11:
            return chartData ? (chartData.subtitle || "") : "";
        case 12:
            if (root.effectiveScreenKey === "courseResult" && resolver.courseDisplayName().length > 0) {
                return resolver.courseDisplayName();
            }
            if (chartData) {
                return resolver.chartFullTitle(chartData);
            }
            return root.lr2SkinUsesBeatorajaSemantics && selectContext.isCourse(selectedEntry)
                ? ""
                : selectContext.entryDisplayName(selectedEntry, true);
        case 13:
            return chartData ? (chartData.genre || "") : (root.lr2SkinUsesBeatorajaSemantics ? "" : "Course");
        case 14:
            return chartData ? (chartData.artist || "") : "";
        case 15:
            return chartData ? (chartData.subartist || "") : "";
        case 16:
            if (!root.lr2SkinUsesBeatorajaSemantics) {
                return chartData ? (chartData.tag || "") : "";
            }
            return chartData
                ? (chartData.artist || "") + (chartData.subartist ? " " + chartData.subartist : "")
                : "";
        case 17:
        case 27:
            if (root.lr2SkinUsesBeatorajaSemantics) {
                return "";
            }
            return chartData ? String(chartData.playLevel || 0) : "";
        case 18:
        case 28:
            if (root.lr2SkinUsesBeatorajaSemantics) {
                return "";
            }
            return chartData ? String(selectContext.entryDifficulty(chartData)) : "";
        case 19:
        case 29:
            if (root.lr2SkinUsesBeatorajaSemantics) {
                return "";
            }
            return chartData ? String(chartData.exlevel || chartData.exLevel || "") : "";
        default:
            return "";
        }
    }

    function resolveSelectedEntryText(st: var, selectedEntry: var) : var {
        if (root.effectiveScreenKey !== "select") {
            return "";
        }
        switch (st) {
        case 20:
            return selectContext.entryMainTitle(selectedEntry);
        case 21:
            return selectContext.entrySubtitle(selectedEntry);
        case 22:
            return selectContext.entryDisplayName(selectedEntry, true);
        case 23:
            return selectContext.entryGenre(selectedEntry);
        case 24:
            return selectContext.entryArtist(selectedEntry);
        case 25:
            return selectContext.entrySubartist(selectedEntry);
        case 26:
            return selectedEntry ? (selectedEntry.tag || "") : "";
        default:
            return "";
        }
    }

    function resolveText(st: var) : var {
        return resolver.computeResolvedText(resolver.numericValue(st, -1));
    }

    function queueResolvedTextRefresh(ids: var) : void {
        if (!resolver.textRegistry) {
            return;
        }

        if (ids === undefined || ids === null) {
            resolver.resolvedTextRefreshAllQueued = true;
            resolver.resolvedTextRefreshPendingIds = ({});
        } else if (!resolver.resolvedTextRefreshAllQueued) {
            let pending = resolver.resolvedTextRefreshPendingIds;
            for (let i = 0; i < ids.length; ++i) {
                pending[String(ids[i])] = true;
            }
        }

        if (resolver.resolvedTextRefreshQueued) {
            return;
        }
        resolver.resolvedTextRefreshQueued = true;
        Qt.callLater(resolver.refreshResolvedTexts);
    }

    function refreshResolvedTexts() : void {
        resolver.resolvedTextRefreshQueued = false;
        let registry = resolver.textRegistry;
        if (!registry) {
            return;
        }

        const refreshAll = resolver.resolvedTextRefreshAllQueued;
        const pendingIds = resolver.resolvedTextRefreshPendingIds;
        resolver.resolvedTextRefreshAllQueued = false;
        resolver.resolvedTextRefreshPendingIds = ({});

        if (refreshAll) {
            let count = registry.activeTextIdCount;
            for (let i = 0; i < count; ++i) {
                let id = registry.activeTextIdAt(i);
                registry.setText(id, resolver.computeResolvedText(id));
            }
            return;
        }

        const keys = Object.keys(pendingIds);
        for (let i = 0; i < keys.length; ++i) {
            let id = Number(keys[i]);
            if (registry.isTextIdActive(id)) {
                registry.setText(id, resolver.computeResolvedText(id));
            }
        }
    }

    function computeResolvedText(st: var) : var {
        switch (st) {
        case 1:
            return root.lr2SkinUsesBeatorajaSemantics ? resolver.beatorajaRivalText() : root.lr2TargetText();
        case 3:
            return root.lr2SkinUsesBeatorajaSemantics ? root.lr2TargetText() : resolver.lr2SelectOptionText(st);
        case 2:
            return Rg.profileList.mainProfile.vars.generalVars.name || "";
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
        case 17:
        case 18:
        case 19:
            return resolver.resolveChartText(st, root.displayChartData(), resolver.selectedTextEntry());
        case 20:
        case 21:
        case 22:
        case 23:
        case 24:
        case 25:
        case 26:
            if (root.lr2SkinUsesBeatorajaSemantics) {
                return "";
            }
            return resolver.resolveSelectedEntryText(st, resolver.selectedTextEntry());
        case 27:
        case 28:
        case 29:
            return resolver.resolveChartText(st, root.displayChartData(), resolver.selectedTextEntry());
        case 30:
            if (root.effectiveScreenKey !== "select") {
                return "";
            }
            if (selectContext.searchText.length > 0) {
                return selectContext.searchText;
            }
            let folderName = selectContext.currentFolderDisplayName();
            return folderName.length > 0 ? folderName : root.lr2SearchPlaceholderText;
        case 60:
            return root.effectiveScreenKey === "select" ? selectContext.keyFilterLabel() : "ALL";
        case 61:
            return root.effectiveScreenKey === "select" ? selectContext.sortLabel() : "DIRECTORY";
        case 62:
            return root.effectiveScreenKey === "select" ? selectContext.difficultyFilterLabel() : "ALL";
        case 86:
            return root.lr2SkinUsesBeatorajaSemantics ? "RIVALCHART" : "";
        case 120:
        case 121:
        case 122:
        case 123:
        case 124:
        case 125:
        case 126:
        case 127:
        case 128:
        case 129: {
            let rankingName = rankingState.entryName(st - 120);
            return rankingName.length > 0 ? rankingName : resolver.lr2SelectOptionText(st);
        }
        case 150:
        case 151:
        case 152:
        case 153:
        case 154:
        case 155:
        case 156:
        case 157:
        case 158:
        case 159:
            return resolver.chartTitle(resolver.courseStage(st - 150));
        case 160:
        case 161:
        case 162:
        case 163:
        case 164:
        case 165:
        case 166:
        case 167:
        case 168:
        case 169:
            return resolver.chartSubtitle(resolver.courseStage(st - 160));
        case 1000:
            return root.effectiveScreenKey === "select"
                ? (root.lr2SkinUsesBeatorajaSemantics
                    ? selectContext.currentDirectoryBreadcrumb()
                    : selectContext.currentFolderDisplayName())
                : "";
        case 1001:
            return resolver.displayTableName();
        case 1002:
            return resolver.displayTableLevelText();
        case 1003:
            return resolver.displayTableFullText();
        case 1010:
            return root.lr2SkinUsesBeatorajaSemantics ? (Qt.application.version || "") : "";
        case 1020:
        case 1021:
            return "";
        case 1030:
            return resolver.chartHashText(root.displayChartData(), "md5");
        case 1031:
            return resolver.chartHashText(root.displayChartData(), "sha256");
        default:
            return root.effectiveScreenKey === "select" ? resolver.lr2SelectOptionText(st) : "";
        }
    }

    onTextRegistryChanged: queueResolvedTextRefresh()

    property Connections textRegistryConnections: Connections {
        target: resolver.textRegistry
        function onActiveTextIdsChanged() : void {
            resolver.queueResolvedTextRefresh();
        }
    }

    Component.onCompleted: queueResolvedTextRefresh()

    function resolveGameplayNumber(num: var) : var {
        if ((num >= 1510 && num <= 1599) || (num >= 1610 && num <= 1699)) {
            return root.gameplayJudgeValueForId(num);
        }
        if (num >= 271 && num <= 289) {
            return 0;
        }

        switch (num) {
        case 20:
            return root.lr2CurrentFps;
        case 10:
            return root.lr2HiSpeedP1;
        case 11:
            return root.lr2HiSpeedP2;
        case 310:
            return root.hiSpeedInteger(1);
        case 311:
            return root.hiSpeedAfterDot(1);
        case 312:
            return root.durationNumber(1, false);
        case 313:
            return root.durationNumber(1, true);
        case 314:
            return root.liftNumber(1);
        case 315:
            return root.hiddenNumber(1);
        case 12: {
            let vars = root.mainGeneralVarsRef;
            return vars ? Math.round(vars.offset || 0) : 0;
        }
        case 13:
            return root.lr2TargetPercent;
        case 14:
            return root.laneCoverNumber(1);
        case 15:
            return root.laneCoverNumber(2);
        case 160: {
            let p1 = root.gameplayPlayer(1);
            return p1 && (p1.bpm || 0) > 0 ? Math.round(p1.bpm) : 1;
        }
        case 161:
            return Math.floor(root.gameplayTimeSeconds(1, false) / 60);
        case 162:
            return root.gameplayTimeSeconds(1, false) % 60;
        case 163:
            return Math.floor(root.gameplayTimeSeconds(1, true) / 60);
        case 164:
            return root.gameplayTimeSeconds(1, true) % 60;
        }

        let chartData = root.gameplayChartData();
        let s1 = root.gameplayScore(1);
        let s2 = root.gameplayScore(2);

        switch (num) {
        case 1312:
        case 1313:
        case 1314:
        case 1315:
        case 1316:
        case 1317:
        case 1318:
        case 1319:
        case 1320:
        case 1321:
        case 1322:
        case 1323:
        case 1324:
        case 1325:
        case 1326:
        case 1327:
            return root.bpmDurationNumber(num, chartData);
        case 100:
            return root.gameplayDisplayedScorePrint(1);
        case 101:
            return root.gameplayExScore(s1);
        case 102:
            return root.gameplayRateInteger(s1, true);
        case 103:
            return root.gameplayRateDecimal(s1, true);
        case 104:
            return root.gameplayCombo(1, false);
        case 105:
            return root.gameplayCombo(1, true);
        case 106:
            return root.gameplayTotalNotes(s1);
        case 107:
            return Math.floor(root.gameplayGaugeValue(s1) / 2) * 2;
        case 407:
            return root.gaugeAfterDot(root.gameplayGaugeValue(s1));
        case 108:
            return root.gameplayExScore(s1) - root.gameplayExScore(s2);
        case 109:
            return root.gameplayRankDelta(s1);
        case 110:
            return root.gameplayJudgementCount(s1, 5);
        case 111:
            return root.gameplayJudgementCount(s1, 4);
        case 112:
            return root.gameplayJudgementCount(s1, 3);
        case 113:
            return root.gameplayJudgementCount(s1, 2);
        case 114:
            return root.gameplayPoorCount(s1);
        case 115:
            return root.gameplayRateInteger(s1, false);
        case 116:
            return root.gameplayRateDecimal(s1, false);
        case 410:
        case 411:
        case 412:
        case 413:
        case 414:
        case 415:
        case 416:
        case 417:
        case 418:
        case 419:
            return root.gameplayJudgeTimingNumber(num, 1);
        case 420:
            return root.gameplayJudgementCount(s1, Judgement.EmptyPoor);
        case 421:
        case 422:
        case 423:
        case 424:
            return root.gameplayJudgeTimingNumber(num, 1);
        case 425:
            return root.gameplayJudgementCount(s1, Judgement.Bad)
                + root.gameplayJudgementCount(s1, Judgement.Poor)
                + root.gameplayJudgementCount(s1, Judgement.EmptyPoor);
        case 426:
            return root.gameplayPoorCount(s1);
        case 427:
            return root.gameplayJudgementCount(s1, Judgement.Bad) + root.gameplayPoorCount(s1);
        case 525:
            return root.gameplayLastJudgeTiming1;
        case 526:
            return root.gameplayLastJudgeTiming2;
        case 527:
            return root.gameplayLastJudgeTiming1;
        case 500:
        case 501:
        case 502:
        case 503:
        case 504:
        case 505:
        case 506:
        case 507:
        case 508:
        case 509:
        case 510:
        case 511:
        case 512:
        case 513:
        case 514:
        case 515:
        case 516:
        case 517:
        case 518:
        case 519:
        case 520:
        case 521:
        case 522:
            return root.gameplayJudgeValueForId(num);
        case 120:
            return root.gameplayDisplayedScorePrint(2);
        case 121:
            return root.gameplayExScore(s2);
        case 122:
            return root.gameplayRateInteger(s2, true);
        case 123:
            return root.gameplayRateDecimal(s2, true);
        case 124:
            return root.gameplayCombo(2, false);
        case 125:
            return root.gameplayCombo(2, true);
        case 126:
            return root.gameplayTotalNotes(s2);
        case 127:
            return Math.floor(root.gameplayGaugeValue(s2) / 2) * 2;
        case 128:
            return root.gameplayExScore(s2) - root.gameplayExScore(s1);
        case 129:
            return root.gameplayRankDelta(s2);
        case 130:
            return root.gameplayJudgementCount(s2, 5);
        case 131:
            return root.gameplayJudgementCount(s2, 4);
        case 132:
            return root.gameplayJudgementCount(s2, 3);
        case 133:
            return root.gameplayJudgementCount(s2, 2);
        case 134:
            return root.gameplayPoorCount(s2);
        case 135:
            return root.gameplayRateInteger(s2, false);
        case 136:
            return root.gameplayRateDecimal(s2, false);
        case 150:
            return root.gameplayHighScorePoints();
        case 151:
            return root.gameplayTargetScorePoints(1);
        case 152:
            return root.gameplayExScore(s1) - root.gameplayHighScorePoints();
        case 153:
            return root.gameplayExScore(s1) - root.gameplayTargetScorePoints(1);
        case 154:
            return root.gameplayRankDelta(s1);
        case 155:
            return root.gameplayScoreRateInteger(root.gameplayHighScorePoints(), s1);
        case 156:
            return root.gameplayScoreRateDecimal(root.gameplayHighScorePoints(), s1);
        case 157:
            return root.gameplayScoreRateInteger(root.gameplayTargetScorePoints(1), s1);
        case 158:
            return root.gameplayScoreRateDecimal(root.gameplayTargetScorePoints(1), s1);
        case 1163: {
            let seconds = root.chartLengthSeconds(chartData);
            return seconds >= 0 ? Math.floor(seconds / 60) % 60 : -1;
        }
        case 1164: {
            let seconds = root.chartLengthSeconds(chartData);
            return seconds >= 0 ? seconds % 60 : -1;
        }
        case 165:
            return root.chart && root.chart.status !== undefined ? 100 : 0;
        case 350:
            return chartData ? (chartData.normalNoteCount || 0) : -1;
        case 351:
            return chartData ? (chartData.lnCount || 0) : -1;
        case 352:
            return chartData ? (chartData.scratchCount || 0) : -1;
        case 353:
            return chartData ? (chartData.bssCount || 0) : -1;
        case 354:
            return chartData ? (chartData.mineCount || 0) : -1;
        case 360:
            return root.chartDensityNumber(chartData, "peakDensity", false);
        case 361:
            return root.chartDensityNumber(chartData, "peakDensity", true);
        case 362:
            return root.chartDensityNumber(chartData, "endDensity", false);
        case 363:
            return root.chartDensityNumber(chartData, "endDensity", true);
        case 364:
            return root.chartDensityNumber(chartData, "avgDensity", false);
        case 365:
            return root.chartDensityNumber(chartData, "avgDensity", true);
        case 368:
            return chartData ? Math.floor(chartData.total || 0) : -1;
        case 42:
        case 96:
            return chartData ? (chartData.playLevel || 0) : 0;
        case 90:
        case 290:
            return chartData && (chartData.maxBpm || chartData.mainBpm)
                ? Math.round(chartData.maxBpm || chartData.mainBpm)
                : -1;
        case 91:
        case 291:
            return chartData && (chartData.minBpm || chartData.mainBpm)
                ? Math.round(chartData.minBpm || chartData.mainBpm)
                : -1;
        case 92:
            return chartData && chartData.mainBpm ? Math.round(chartData.mainBpm) : -1;
        default:
            return 0;
        }
    }

    function resultCompareScore() : var {
        return root.resultScore(2) || root.resultTargetSavedScore(1);
    }

    function resultCompareResult() : var {
        let score = resolver.resultCompareScore();
        return score && score.result ? score.result : null;
    }

    function resultGaugeValueFromScore(score: var, fallbackSide: var) : var {
        if (score === root.resultScore(1)) {
            return root.resultGaugeValue(1);
        }
        if (score === root.resultScore(2)) {
            return root.resultGaugeValue(2);
        }

        let infos = score && score.gaugeHistory ? score.gaugeHistory.gaugeInfo : [];
        if (!infos || infos.length === 0) {
            return root.resultGaugeValue(fallbackSide);
        }

        let best = null;
        let fallback = infos[infos.length - 1];
        for (let i = 0; i < infos.length; ++i) {
            let info = infos[i];
            let history = info && info.gaugeHistory ? info.gaugeHistory : [];
            let last = history && history.length > 0 ? history[history.length - 1].gauge : 0;
            if (!best && root.gaugeAboveThreshold(last, info.threshold)) {
                best = info;
            }
            if (info && !info.courseGauge) {
                fallback = info;
            }
        }

        let selected = best || fallback;
        let history = selected && selected.gaugeHistory ? selected.gaugeHistory : [];
        return history && history.length > 0 ? (history[history.length - 1].gauge || 0) : 0;
    }

    function resultScoreBarGraphMaximum(result: var) : var {
        let chartData = root.resultChartData();
        let keymode = chartData ? chartData.keymode : (result ? result.keymode : 0);
        return keymode === 7 || keymode === 14 ? 20000 : 10000;
    }

    function resultScorePrintFromLr2Score(score: var, result: var) : var {
        let chartData = root.resultChartData();
        let keymode = chartData ? chartData.keymode : (result ? result.keymode : 0);
        return keymode === 7 || keymode === 14 ? score : Math.floor(score / 20) * 10;
    }

    function resultScorePrintFromTargetPoints(points: var, totalNotes: var, result: var) : var {
        let score = totalNotes > 0 ? Math.floor(points * 100000 / totalNotes) : 0;
        return resolver.resultScorePrintFromLr2Score(score, result);
    }

    function resultRankDeltaFromPoints(points: var, totalNotes: var) : var {
        let perfectScore = totalNotes * 2;
        if (totalNotes <= 0 || points === perfectScore) {
            return 0;
        }
        let rank = Math.floor(points * 9 / perfectScore);
        rank = Math.max(1, Math.min(8, rank));
        return points - Math.floor(perfectScore * (rank + 1) / 9);
    }

    function resultFinalCombo(score: var, result: var) : var {
        let events = score && score.replayData ? (score.replayData.hitEvents || []) : [];
        if (!events || events.length === 0) {
            return result ? (result.maxCombo || 0) : 0;
        }

        let combo = 0;
        for (let hit of events) {
            let judgement = root.gameplayJudgementFromHit(hit);
            if (judgement === Judgement.Poor || judgement === Judgement.Bad) {
                combo = 0;
            } else if (judgement >= Judgement.Good && judgement <= Judgement.Perfect) {
                ++combo;
            }
        }
        return combo;
    }

    function targetTotalNotes(side: var) : var {
        return Math.floor(root.resultTargetMaxPoints(side) / 2);
    }

    function targetClampedPoints(side: var) : var {
        let totalNotes = resolver.targetTotalNotes(side);
        return Math.max(0, Math.min(totalNotes * 2, Math.floor(root.resultTargetPoints(side))));
    }

    function hasSyntheticTarget(side: var) : var {
        return resolver.targetClampedPoints(side) > 0;
    }

    function targetMaxCombo(side: var) : var {
        return resolver.hasSyntheticTarget(side) ? resolver.targetTotalNotes(side) : 0;
    }

    function targetPerfectCount(side: var) : var {
        if (!resolver.hasSyntheticTarget(side)) {
            return 0;
        }
        let totalNotes = resolver.targetTotalNotes(side);
        let points = resolver.targetClampedPoints(side);
        return Math.max(0, points - totalNotes);
    }

    function targetGreatCount(side: var) : var {
        if (!resolver.hasSyntheticTarget(side)) {
            return 0;
        }
        let totalNotes = resolver.targetTotalNotes(side);
        let points = resolver.targetClampedPoints(side);
        if (points <= totalNotes) {
            return points;
        }
        return totalNotes - resolver.targetPerfectCount(side);
    }

    function targetGoodCount(side: var) : var {
        if (!resolver.hasSyntheticTarget(side)) {
            return 0;
        }
        let totalNotes = resolver.targetTotalNotes(side);
        let points = resolver.targetClampedPoints(side);
        return points < totalNotes ? totalNotes - points : 0;
    }

    function resolveResultSideNumber(num: var, result: var, score: var, otherResult: var, otherPoints: var) : var {
        switch (num) {
        case 0:
            return root.resultScorePrint(result);
        case 1:
            return root.resultExScore(result);
        case 2:
            return root.resultRateInteger(result);
        case 3:
            return root.resultRateDecimal(result);
        case 4:
            return resolver.resultFinalCombo(score, result);
        case 5:
            return result ? (result.maxCombo || 0) : 0;
        case 6:
            return root.resultTotalNotes(result);
        case 7:
            return Math.floor(resolver.resultGaugeValueFromScore(
                score,
                result === root.resultData(2) ? 2 : 1));
        case 8:
            return root.resultExScore(result)
                - (otherResult ? root.resultExScore(otherResult) : Math.floor(otherPoints || 0));
        case 9:
            return root.resultRankDelta(result);
        case 10:
            return root.resultJudgementCount(result, Judgement.Perfect);
        case 11:
            return root.resultJudgementCount(result, Judgement.Great);
        case 12:
            return root.resultJudgementCount(result, Judgement.Good);
        case 13:
            return root.resultJudgementCount(result, Judgement.Bad);
        case 14:
            return root.resultPoorCount(result);
        case 15:
            return root.resultRateInteger(result);
        case 16:
            return root.resultRateDecimal(result);
        default:
            return 0;
        }
    }

    function resolveResultTargetSideNumber(num: var, side: var) : var {
        let targetScore = root.resultTargetSavedScore(side);
        let targetResult = targetScore && targetScore.result ? targetScore.result : null;
        if (targetResult) {
            return resolver.resolveResultSideNumber(
                num,
                targetResult,
                targetScore,
                root.resultData(side),
                root.resultExScore(root.resultData(side)));
        }

        let points = resolver.targetClampedPoints(side);
        let maxPoints = root.resultTargetMaxPoints(side);
        let totalNotes = Math.floor(maxPoints / 2);
        let current = root.resultData(side);
        let hasTarget = resolver.hasSyntheticTarget(side);
        switch (num) {
        case 0:
            return hasTarget ? resolver.resultScorePrintFromTargetPoints(points, totalNotes, current) : 0;
        case 1:
            return points;
        case 2:
            return Math.floor(points * 100 / maxPoints);
        case 3:
            return Math.floor(points * 10000 / maxPoints) % 100;
        case 4:
            return hasTarget ? totalNotes : 0;
        case 5:
            return resolver.targetMaxCombo(side);
        case 6:
            return hasTarget ? totalNotes : 0;
        case 7:
            return 0;
        case 8:
            return hasTarget ? points - root.resultExScore(current) : 0;
        case 9:
            return hasTarget ? resolver.resultRankDeltaFromPoints(points, totalNotes) : 0;
        case 10:
            return resolver.targetPerfectCount(side);
        case 11:
            return resolver.targetGreatCount(side);
        case 12:
            return resolver.targetGoodCount(side);
        case 13:
        case 14:
            return 0;
        case 15:
            return Math.floor(points * 100 / maxPoints);
        case 16:
            return Math.floor(points * 10000 / maxPoints) % 100;
        default:
            return 0;
        }
    }

    function resolveResultNumber(num: var) : var {
        let current = root.resultData(1);
        let old = root.resultOldBestResult(1);
        let chartData = root.resultChartData();

        if (num >= 271 && num <= 289) {
            return 0;
        }

        if (num >= 100 && num <= 116) {
            return resolver.resolveResultSideNumber(
                num - 100,
                current,
                root.resultScore(1),
                resolver.resultCompareResult(),
                root.resultTargetPoints(1));
        }
        if (num >= 120 && num <= 136) {
            let compareScore = resolver.resultCompareScore();
            if (compareScore && compareScore.result && compareScore === root.resultScore(2)) {
                return resolver.resolveResultSideNumber(
                    num - 120,
                    compareScore.result,
                    compareScore,
                    current,
                    root.resultExScore(current));
            }
            return resolver.resolveResultTargetSideNumber(num - 120, 1);
        }

        switch (num) {
        case 20:
        case 21:
        case 22:
        case 23:
        case 24:
        case 25:
        case 26:
        case 27:
        case 28:
        case 29:
            return root.dateTimeNumber(num);
        case 71:
            return root.resultExScore(current);
        case 75:
            return current ? (current.maxCombo || 0) : 0;
        case 76:
            return root.resultBadPoor(current);
        case 310:
            return root.hiSpeedInteger(1);
        case 311:
            return root.hiSpeedAfterDot(1);
        case 312:
            return root.durationNumber(1, false);
        case 313:
            return root.durationNumber(1, true);
        case 314:
            return root.liftNumber(1);
        case 315:
            return root.hiddenNumber(1);
        case 1312:
        case 1313:
        case 1314:
        case 1315:
        case 1316:
        case 1317:
        case 1318:
        case 1319:
        case 1320:
        case 1321:
        case 1322:
        case 1323:
        case 1324:
        case 1325:
        case 1326:
        case 1327:
            return root.bpmDurationNumber(num, chartData);
        case 42:
        case 96: {
            return chartData ? (chartData.playLevel || 0) : 0;
        }
        case 90:
        case 290: {
            return chartData && (chartData.maxBpm || chartData.mainBpm)
                ? Math.round(chartData.maxBpm || chartData.mainBpm)
                : -1;
        }
        case 91:
        case 291: {
            return chartData && (chartData.minBpm || chartData.mainBpm)
                ? Math.round(chartData.minBpm || chartData.mainBpm)
                : -1;
        }
        case 92:
            return chartData && chartData.mainBpm ? Math.round(chartData.mainBpm) : -1;
        case 350:
            return current ? (current.normalNoteCount || 0) : (chartData ? (chartData.normalNoteCount || 0) : -1);
        case 351:
            return current ? (current.lnCount || 0) : (chartData ? (chartData.lnCount || 0) : -1);
        case 352:
            return current ? (current.scratchCount || 0) : (chartData ? (chartData.scratchCount || 0) : -1);
        case 353:
            return current ? (current.bssCount || 0) : (chartData ? (chartData.bssCount || 0) : -1);
        case 354:
            return current ? (current.mineCount || 0) : (chartData ? (chartData.mineCount || 0) : -1);
        case 360:
            return root.chartDensityNumber(chartData, "peakDensity", false);
        case 361:
            return root.chartDensityNumber(chartData, "peakDensity", true);
        case 362:
            return root.chartDensityNumber(chartData, "endDensity", false);
        case 363:
            return root.chartDensityNumber(chartData, "endDensity", true);
        case 364:
            return root.chartDensityNumber(chartData, "avgDensity", false);
        case 365:
            return root.chartDensityNumber(chartData, "avgDensity", true);
        case 368:
            return chartData ? Math.floor(chartData.total || 0) : -1;
        case 370:
            return root.clearTypeValue(current ? current.clearType : "NOPLAY");
        case 371:
            return root.clearTypeValue(old ? old.clearType : "NOPLAY");
        case 372: {
            let stats = root.resultTimingStats(1);
            return resolver.integerPart(stats.averageDuration || 0);
        }
        case 373: {
            let stats = root.resultTimingStats(1);
            return Math.floor((stats.averageDuration || 0) * 100) % 100;
        }
        case 374:
            if (root.effectiveScreenKey === "courseResult") {
                return 0;
            }
            {
                let stats = root.resultTimingStats(1);
                return resolver.integerPart(stats.average || 0);
            }
        case 375:
            if (root.effectiveScreenKey === "courseResult") {
                return 0;
            }
            {
                let stats = root.resultTimingStats(1);
                return root.signedAfterDot(stats.average || 0);
            }
        case 376:
            if (root.effectiveScreenKey === "courseResult") {
                return 0;
            }
            {
                let stats = root.resultTimingStats(1);
                return resolver.integerPart(stats.stddev || 0);
            }
        case 377:
            if (root.effectiveScreenKey === "courseResult") {
                return 0;
            }
            {
                let stats = root.resultTimingStats(1);
                return Math.floor((stats.stddev || 0) * 100) % 100;
            }
        case 407:
            return root.gaugeAfterDot(root.resultGaugeValue(1));
        case 410:
        case 411:
        case 412:
        case 413:
        case 414:
        case 415:
        case 416:
        case 417:
        case 418:
        case 419:
            return root.resultJudgeTimingNumber(num, 1);
        case 420:
            return root.resultJudgementCount(current, Judgement.EmptyPoor);
        case 421:
        case 422:
        case 423:
        case 424:
            return root.resultJudgeTimingNumber(num, 1);
        case 425:
            return root.resultJudgementCount(current, Judgement.Bad)
                + root.resultJudgementCount(current, Judgement.Poor)
                + root.resultJudgementCount(current, Judgement.EmptyPoor);
        case 426:
            return root.resultPoorCount(current);
        case 427:
            return root.resultJudgementCount(current, Judgement.Bad) + root.resultPoorCount(current);
        case 1163: {
            let seconds = root.chartLengthSeconds(chartData || current);
            return seconds >= 0 ? Math.floor(seconds / 60) % 60 : -1;
        }
        case 1164: {
            let seconds = root.chartLengthSeconds(chartData || current);
            return seconds >= 0 ? seconds % 60 : -1;
        }
        case 150:
            return root.resultHighScorePoints(1);
        case 151:
            return root.resultTargetPoints(1);
        case 152:
            return root.resultExScore(current) - root.resultHighScorePoints(1);
        case 153:
            return root.resultExScore(current) - root.resultTargetPoints(1);
        case 154:
            return root.resultRankDelta(current);
        case 155:
            return root.resultScoreRateInteger(root.resultHighScorePoints(1), current);
        case 156:
            return root.resultScoreRateDecimal(root.resultHighScorePoints(1), current);
        case 157:
            return root.resultScoreRateInteger(root.resultTargetPoints(1), current);
        case 158:
            return root.resultScoreRateDecimal(root.resultTargetPoints(1), current);
        case 170:
            return root.resultExScore(old);
        case 171:
            return root.resultExScore(current);
        case 172:
            return root.resultExScore(current) - root.resultExScore(old);
        case 173:
            return old ? (old.maxCombo || 0) : 0;
        case 174:
            return current ? (current.maxCombo || 0) : 0;
        case 175:
            return (current ? (current.maxCombo || 0) : 0) - (old ? (old.maxCombo || 0) : 0);
        case 176:
            return root.resultBadPoor(old);
        case 177:
            return root.resultBadPoor(current);
        case 178:
            return root.resultBadPoor(current) - root.resultBadPoor(old);
        case 179:
            return rankingState.playerRank();
        case 180:
            return rankingState.currentPlayerCount;
        case 181:
            return rankingState.clearPercentValue(false, "AEASY", "EASY", "NORMAL", "HARD", "EXHARD", "FC", "PERFECT", "MAX");
        case 182:
            return 0;
        case 183:
            return root.resultRateInteger(old);
        case 184:
            return root.resultRateDecimal(old);
        default:
            if (num === 200) {
                return rankingState.currentPlayerCount;
            }
            if (num === 201) {
                return rankingState.totalPlayCount(rankingState.entries());
            }
            if (num >= 202 && num <= 242) {
                switch (num) {
                case 202:
                    return rankingState.clearCount("NOPLAY");
                case 203:
                    return rankingState.clearPercentValue(false, "NOPLAY");
                case 204:
                    return rankingState.clearCount("AEASY");
                case 205:
                    return rankingState.clearPercentValue(false, "AEASY");
                case 206:
                    return rankingState.clearCount("LIGHTASSIST");
                case 207:
                    return rankingState.clearPercentValue(false, "LIGHTASSIST");
                case 208:
                    return rankingState.clearCount("EXHARD");
                case 209:
                    return rankingState.clearPercentValue(false, "EXHARD");
                case 210:
                    return rankingState.clearCount("FAILED");
                case 211:
                    return rankingState.clearPercentValue(false, "FAILED");
                case 212:
                    return rankingState.clearCount("EASY");
                case 213:
                    return rankingState.clearPercentValue(false, "EASY");
                case 214:
                    return rankingState.clearCount("NORMAL");
                case 215:
                    return rankingState.clearPercentValue(false, "NORMAL");
                case 216:
                    return rankingState.clearCount("HARD");
                case 217:
                    return rankingState.clearPercentValue(false, "HARD");
                case 218:
                    return rankingState.clearCount("FC");
                case 219:
                    return rankingState.clearPercentValue(false, "FC");
                case 222:
                    return rankingState.clearCount("PERFECT");
                case 223:
                    return rankingState.clearPercentValue(false, "PERFECT");
                case 224:
                    return rankingState.clearCount("MAX");
                case 225:
                    return rankingState.clearPercentValue(false, "MAX");
                case 226:
                    return rankingState.clearCount("AEASY", "EASY", "NORMAL", "HARD", "EXHARD", "FC", "PERFECT", "MAX");
                case 227:
                    return rankingState.clearPercentValue(false, "AEASY", "EASY", "NORMAL", "HARD", "EXHARD", "FC", "PERFECT", "MAX");
                case 228:
                    return rankingState.clearCount("FC", "PERFECT", "MAX");
                case 229:
                    return rankingState.clearPercentValue(false, "FC", "PERFECT", "MAX");
                case 230:
                    return rankingState.clearPercentValue(true, "NOPLAY");
                case 231:
                    return rankingState.clearPercentValue(true, "AEASY");
                case 232:
                    return rankingState.clearPercentValue(true, "LIGHTASSIST");
                case 233:
                    return rankingState.clearPercentValue(true, "EXHARD");
                case 234:
                    return rankingState.clearPercentValue(true, "FAILED");
                case 235:
                    return rankingState.clearPercentValue(true, "EASY");
                case 236:
                    return rankingState.clearPercentValue(true, "NORMAL");
                case 237:
                    return rankingState.clearPercentValue(true, "HARD");
                case 238:
                    return rankingState.clearPercentValue(true, "FC");
                case 239:
                    return rankingState.clearPercentValue(true, "PERFECT");
                case 240:
                    return rankingState.clearPercentValue(true, "MAX");
                case 241:
                    return rankingState.clearPercentValue(true, "AEASY", "EASY", "NORMAL", "HARD", "EXHARD", "FC", "PERFECT", "MAX");
                case 242:
                    return rankingState.clearPercentValue(true, "FC", "PERFECT", "MAX");
                default:
                    return 0;
                }
            }
            if (num >= 380 && num <= 389) {
                return rankingState.entryExScore(num - 380);
            }
            if (num >= 390 && num <= 399) {
                return rankingState.entryClearValue(num - 390);
            }
            return 0;
        }
    }

    function resultButtonFrameOverride(src: var) : var {
        if (!root.resultScreenActive || !src || !src.button) {
            return -1;
        }

        let buttonId = Math.floor(resolver.numericValue(src.buttonId, -1));
        if (buttonId !== 370 && buttonId !== 371) {
            return -1;
        }

        let value = Math.max(0, Math.floor(resolver.resolveResultNumber(buttonId) || 0));
        let divX = Math.max(1, Math.floor(src.div_x || 1));
        let divY = Math.max(1, Math.floor(src.div_y || 1));
        return Math.min(value, divY - 1) * divX;
    }

    function resolveNumber(num: var) : var {
        let globalValue = resolver.resolveBeatorajaGlobalNumber(num);
        if (globalValue !== undefined) {
            return globalValue;
        }
        if (root.resultScreenActive) {
            return resolver.resolveResultNumber(num);
        }
        if (root.gameplayScreenActive) {
            return resolver.resolveGameplayNumber(num);
        }
        if (root.effectiveScreenKey === "select" || root.effectiveScreenKey === "decide") {
            switch (num) {
            case 10:
                return root.lr2HiSpeedP1;
            case 11:
                return root.lr2HiSpeedP2;
            case 12: {
                let vars = root.mainGeneralVarsRef;
                return vars ? Math.round(vars.offset || 0) : 0;
            }
            case 13:
                return root.lr2TargetPercent;
            case 14:
                return root.laneCoverNumber(1);
            case 15:
                return root.laneCoverNumber(2);
            case 20:
            case 21:
            case 22:
            case 23:
            case 24:
            case 25:
            case 26:
            case 27:
            case 28:
            case 29:
                return root.dateTimeNumber(num);
            case 308:
                return root.lr2LnModeIndex;
            case 340:
                return root.lr2JudgeAlgorithmIndex;
            case 341:
                return root.lr2BottomShiftableGaugeIndex;
            case 310:
                return root.hiSpeedInteger(1);
            case 311:
                return root.hiSpeedAfterDot(1);
            case 312:
                return root.durationNumber(1, false);
            case 313:
                return root.durationNumber(1, true);
            case 314:
                return root.liftNumber(1);
            case 315:
                return root.hiddenNumber(1);
            case 1312:
            case 1313:
            case 1314:
            case 1315:
            case 1316:
            case 1317:
            case 1318:
            case 1319:
            case 1320:
            case 1321:
            case 1322:
            case 1323:
            case 1324:
            case 1325:
            case 1326:
            case 1327:
                return root.bpmDurationNumber(num, selectContext.selectedStateChartData);
            }
            if ((num >= 50 && num <= 66) || num === 8) {
                return root.lr2SliderNumber(num);
            }
            if (num >= 250 && num <= 259) {
                let stage = resolver.courseStage(num - 250);
                return selectContext.entryPlayLevel(stage);
            }
            if (num === 220) {
                return -1;
            }
            if (num >= 380 && num <= 389) {
                return rankingState.entryExScore(num - 380);
            }
            if (num >= 390 && num <= 399) {
                return rankingState.entryClearValue(num - 390);
            }
            if ((num >= 271 && num <= 289)
                    || num === 292 || num === 293) {
                return 0;
            }
            let selectValue = selectContext.numberValue(num);
            if (root.effectiveScreenKey === "select" || selectValue !== 0) {
                return selectValue;
            }
            let chartData = root.chart && root.chart.chartData ? root.chart.chartData : null;
            switch (num) {
            case 42:
            case 96:
                return chartData ? (chartData.playLevel || 0) : 0;
            case 90:
            case 290:
                return chartData && (chartData.maxBpm || chartData.mainBpm)
                    ? Math.round(chartData.maxBpm || chartData.mainBpm)
                    : -1;
            case 91:
            case 291:
                return chartData && (chartData.minBpm || chartData.mainBpm)
                    ? Math.round(chartData.minBpm || chartData.mainBpm)
                    : -1;
            default:
                return 0;
            }
        }
        return 0;
    }

    function dependOnGameplayNumberState(src: var) : void {
        if (!root.gameplayScreenActive) {
            return;
        }
        if (src && src.nowCombo) {
            if ((src.side || (src.timer === 47 ? 2 : 1)) === 2) {
                root.gameplayJudgeRevision2;
            } else {
                root.gameplayJudgeRevision1;
            }
            return;
        }

        let num = src ? (src.num || 0) : 0;
        if (num === 20 || (num >= 160 && num <= 164)) {
            root.renderSkinTime;
            return;
        }
        if (num === 11 || num === 15
                || (num >= 120 && num <= 136) || num === 526 || num === 521
                || (num >= 510 && num <= 519) || (num >= 1610 && num <= 1699)) {
            root.gameplayNumberRevision2;
            root.gameplayJudgeRevision2;
            return;
        }
        if (num === 10 || num === 12 || num === 13 || num === 14
                || (num >= 310 && num <= 315)
                || (num >= 100 && num <= 116) || num === 407
                || (num >= 410 && num <= 427)
                || (num >= 500 && num <= 509) || num === 520 || num === 522
                || num === 525 || num === 527 || (num >= 1510 && num <= 1599)) {
            root.gameplayNumberRevision1;
            root.gameplayJudgeRevision1;
            return;
        }
        if (num === 42 || num === 90 || num === 91 || num === 92
                || num === 106 || num === 126 || num === 165
                || num === 290 || num === 291
                || (num >= 350 && num <= 365) || num === 368
                || num === 1163 || num === 1164) {
            root.gameplayStaticNumberRevision;
            return;
        }
        root.gameplayNumberRevision1;
        root.gameplayNumberRevision2;
        root.gameplayScoresRevision;
    }

    function numberValue(src: var) : var {
        resolver.dependOnGameplayNumberState(src);
        if (src && src.nowCombo) {
            return (src.side || (src.timer === 47 ? 2 : 1)) === 2
                ? root.gameplayJudgeCombo2
                : root.gameplayJudgeCombo1;
        }
        let num = src ? (src.num || 0) : 0;
        return resolver.resolveNumber(num);
    }

    function imageSetValue(imageSetRef: var, sourceCount: var) : var {
        let id = Math.floor(imageSetRef || 0);
        switch (id) {
        case 12:
            return root.effectiveScreenKey === "select" && sourceCount >= 5
                ? selectContext.sortFrameForSourceCount(sourceCount)
                : resolver.resolveNumber(id);
        case 40:
            return root.lr2GaugeButtonFrame(1, sourceCount);
        case 41:
            return root.lr2GaugeButtonFrame(2, sourceCount);
        case 42:
            return root.lr2RandomButtonFrame(1, sourceCount);
        case 43:
            return root.lr2RandomButtonFrame(2, sourceCount);
        case 54:
            return root.lr2DpOptionIndex;
        case 55:
            return root.lr2HiSpeedFixIndex;
        case 72:
            return sourceCount >= 3 ? root.lr2BeatorajaBgaIndex : root.lr2BgaIndex;
        case 77:
            return root.lr2TargetButtonFrame(sourceCount);
        case 78:
            return root.lr2GaugeAutoShiftIndex;
        case 340:
            return root.lr2JudgeAlgorithmIndex;
        case 341:
            return root.lr2BottomShiftableGaugeIndex;
        case 342:
        case 343:
        case 350:
        case 351:
        case 352:
        case 353:
        case 360:
        case 361:
        case 400:
            return 0;
        case 301:
        case 302:
        case 303:
        case 304:
        case 305:
        case 306:
        case 307:
            return root.beatorajaAssistOptionFrame(id);
        case 308:
            return root.lr2LnModeIndex;
        default:
            return resolver.resolveNumber(id);
        }
    }

    function imageSetSourceFor(src: var) : var {
        if (!src || !src.imageSet || !src.imageSetSources || src.imageSetSources.length <= 0) {
            return src;
        }
        let value = Math.floor(resolver.imageSetValue(src.imageSetRef || 0, src.imageSetSources.length));
        if (isFinite(value) && value < -1) {
            let hidden = root.copyObject(src);
            hidden.source = "";
            hidden.specialType = 0;
            return hidden;
        }
        if (!isFinite(value) || value < 0 || value >= src.imageSetSources.length) {
            value = 0;
        }
        let selected = src.imageSetSources[value] || src.imageSetSources[0];
        if (!selected) {
            return src;
        }
        let source = selected.source !== undefined ? selected.source : src.source;
        let specialType = selected.specialType !== undefined ? selected.specialType : src.specialType;
        if (!source && !specialType) {
            source = src.source || "";
            specialType = src.specialType || 0;
        }
        return {
            gr: selected.gr !== undefined ? selected.gr : (src.gr || 0),
            x: selected.x !== undefined ? selected.x : (src.x || 0),
            y: selected.y !== undefined ? selected.y : (src.y || 0),
            w: selected.w !== undefined ? selected.w : (src.w || 0),
            h: selected.h !== undefined ? selected.h : (src.h || 0),
            div_x: Math.max(1, selected.div_x || 1),
            div_y: Math.max(1, selected.div_y || 1),
            cycle: src.cycle || 0,
            timer: src.timer || 0,
            zeropadding: src.zeropadding !== undefined ? src.zeropadding : -1,
            op1: src.op1 || 0,
            op2: src.op2 || 0,
            op3: src.op3 || 0,
            op4: src.op4 || 0,
            resultChartType: selected.resultChartType || 0,
            resultChartIndex: selected.resultChartIndex || 0,
            button: false,
            onMouse: false,
            mouseCursor: false,
            slider: false,
            specialType: specialType || 0,
            side: selected.side || 0,
            source: source || ""
        };
    }

    function numberSourceFrameGroupSize(src: var) : var {
        if (!src) {
            return 0;
        }
        let divX = Math.max(1, src.div_x || 1);
        let divY = Math.max(1, src.div_y || 1);
        let frames = divX * divY;
        if (frames % 24 === 0) return 24;
        if (frames % 11 === 0) return 11;
        if (frames % 10 === 0) return 10;
        return 0;
    }

    function numberForceHidden(src: var) : var {
        if (!src) {
            return false;
        }
        if (resolver.optionOnlyRankId(src.num || 0)) {
            return true;
        }
        if (root.effectiveScreenKey === "select"
                && resolver.numberSourceFrameGroupSize(src) !== 24
                && resolver.resolveNumber(src.num || 0) < 0) {
            return true;
        }
        if (!src.nowCombo || !root.gameplayScreenActive) {
            return false;
        }
        let side = src.side || (src.timer === 47 ? 2 : 1);
        let judgement = side === 2 ? root.gameplayLastJudgement2 : root.gameplayLastJudgement1;
        let combo = side === 2 ? root.gameplayJudgeCombo2 : root.gameplayJudgeCombo1;
        return combo <= 0
            || (src.judgementIndex >= 0 && judgement !== src.judgementIndex);
    }

    function resolveBarGraph(type: var) : var {
        if (root.effectiveScreenKey === "select") {
            return selectContext.barGraphValue(type);
        }
        if (root.gameplayScreenActive) {
            root.gameplayRevision;
            return playContext.barGraphValue(type);
        }
        if (root.resultScreenActive) {
            return resolver.resultBarGraphValue(type);
        }
        return 0;
    }

    function normalizedBarValue(value: var, maximum: var) : var {
        return maximum > 0 ? Math.max(0, Math.min(1, value / maximum)) : 0;
    }

    function resultBarGraphValue(type: var) : var {
        let current = root.resultData(1);
        let old = root.resultOldBestResult(1);
        let maxPoints = current ? Math.max(0, current.maxPoints || 0) : 0;
        let totalNotes = root.resultTotalNotes(current);
        let compareScore = resolver.resultCompareScore();
        let compare = compareScore && compareScore.result ? compareScore.result : null;
        let comparePoints = compare
            ? root.resultExScore(compare)
            : resolver.targetClampedPoints(1);
        let compareMaxPoints = maxPoints || (compare ? (compare.maxPoints || 0) : 0);
        let compareTotalNotes = compare
            ? root.resultTotalNotes(compare)
            : resolver.targetTotalNotes(1);
        let compareMaxCombo = compare ? (compare.maxCombo || 0) : resolver.targetMaxCombo(1);
        let oldTotalNotes = root.resultTotalNotes(old);
        switch (type) {
        case 1:
        case 101:
        case 2:
        case 102:
            return 1;
        case 10:
        case 110:
        case 11:
        case 111:
            return resolver.normalizedBarValue(root.resultExScore(current), maxPoints);
        case 12:
        case 112:
        case 13:
        case 113:
            return resolver.normalizedBarValue(root.resultExScore(old),
                                               maxPoints || (old ? (old.maxPoints || 0) : 0));
        case 14:
        case 114:
        case 15:
        case 115:
            return resolver.normalizedBarValue(root.resultTargetPoints(1), maxPoints);
        case 20:
        case 140:
            return resolver.normalizedBarValue(root.resultJudgementCount(current, Judgement.Perfect), totalNotes);
        case 21:
        case 141:
            return resolver.normalizedBarValue(root.resultJudgementCount(current, Judgement.Great), totalNotes);
        case 22:
        case 142:
            return resolver.normalizedBarValue(root.resultJudgementCount(current, Judgement.Good), totalNotes);
        case 23:
        case 143:
            return resolver.normalizedBarValue(root.resultJudgementCount(current, Judgement.Bad), totalNotes);
        case 24:
        case 144:
            return resolver.normalizedBarValue(root.resultPoorCount(current), totalNotes);
        case 25:
        case 145:
            return resolver.normalizedBarValue(current ? (current.maxCombo || 0) : 0, totalNotes);
        case 26:
        case 146:
            return resolver.normalizedBarValue(root.resultScorePrint(current),
                                               resolver.resultScoreBarGraphMaximum(current));
        case 27:
        case 147:
            return resolver.normalizedBarValue(root.resultExScore(current), maxPoints);
        case 30:
            return resolver.normalizedBarValue(
                compare ? root.resultJudgementCount(compare, Judgement.Perfect) : resolver.targetPerfectCount(1),
                compareTotalNotes);
        case 31:
            return resolver.normalizedBarValue(
                compare ? root.resultJudgementCount(compare, Judgement.Great) : resolver.targetGreatCount(1),
                compareTotalNotes);
        case 32:
            return resolver.normalizedBarValue(
                compare ? root.resultJudgementCount(compare, Judgement.Good) : resolver.targetGoodCount(1),
                compareTotalNotes);
        case 33:
            return resolver.normalizedBarValue(root.resultJudgementCount(compare, Judgement.Bad), compareTotalNotes);
        case 34:
            return resolver.normalizedBarValue(compare ? root.resultPoorCount(compare) : 0, compareTotalNotes);
        case 35:
            return resolver.normalizedBarValue(compareMaxCombo, compareTotalNotes);
        case 36:
            return resolver.normalizedBarValue(
                compare
                    ? root.resultScorePrint(compare)
                    : resolver.resultScorePrintFromTargetPoints(comparePoints, compareTotalNotes, current),
                resolver.resultScoreBarGraphMaximum(compare || current));
        case 37:
            return resolver.normalizedBarValue(comparePoints, compareMaxPoints);
        case 40:
            return resolver.normalizedBarValue(root.resultJudgementCount(old, Judgement.Perfect), oldTotalNotes);
        case 41:
            return resolver.normalizedBarValue(root.resultJudgementCount(old, Judgement.Great), oldTotalNotes);
        case 42:
            return resolver.normalizedBarValue(root.resultJudgementCount(old, Judgement.Good), oldTotalNotes);
        case 43:
            return resolver.normalizedBarValue(root.resultJudgementCount(old, Judgement.Bad), oldTotalNotes);
        case 44:
            return resolver.normalizedBarValue(root.resultPoorCount(old), oldTotalNotes);
        case 45:
            return resolver.normalizedBarValue(old ? (old.maxCombo || 0) : 0, oldTotalNotes);
        case 46:
            return resolver.normalizedBarValue(root.resultScorePrint(old), resolver.resultScoreBarGraphMaximum(old));
        case 47:
            return resolver.normalizedBarValue(root.resultExScore(old), old ? (old.maxPoints || 0) : 0);
        default:
            return 0;
        }
    }

}
