import QtQuick
import RhythmGameQml 1.0

Item {
    id: root

    property var screenRoot
    property int renderSkinTime: 0

    property real scratchAngle1: 0
    property real scratchAngle2: 0
    property int lastScratchSkinTime: renderSkinTime

    function normalized(value, maximum) {
        if (maximum <= 0) {
            return 0;
        }
        return Math.max(0, Math.min(1, value / maximum));
    }

    function score(side) {
        return screenRoot ? screenRoot.gameplayScore(side) : null;
    }

    function totalNotes(side) {
        return screenRoot ? screenRoot.gameplayTotalNotes(score(side)) : 0;
    }

    function totalPoints(side) {
        return totalNotes(side) * 2;
    }

    function currentNotes(side) {
        return screenRoot ? screenRoot.gameplayCurrentNotes(score(side)) : 0;
    }

    function exScore(side) {
        return screenRoot ? screenRoot.gameplayExScore(score(side)) : 0;
    }

    function judgementCount(side, judgement) {
        return screenRoot ? screenRoot.gameplayJudgementCount(score(side), judgement) : 0;
    }

    function poorCount(side) {
        return screenRoot ? screenRoot.gameplayPoorCount(score(side)) : 0;
    }

    function scorePrint(side) {
        if (!screenRoot) {
            return 0;
        }
        return screenRoot.gameplayScorePrint(score(side), screenRoot.gameplayChartData());
    }

    function scorePrintMaximum() {
        let chartData = screenRoot ? screenRoot.gameplayChartData() : null;
        return chartData && (chartData.keymode === 7 || chartData.keymode === 14)
            ? 20000
            : 10000;
    }

    function resultScorePrint(result) {
        if (!result || !result.judgementCounts || (result.maxHits || 0) <= 0) {
            return 0;
        }
        let counts = result.judgementCounts;
        let value = Math.floor(((counts[3] || 0) + ((counts[4] || 0) + (counts[5] || 0) * 2) * 2) * 50000 / result.maxHits);
        let chartData = screenRoot ? screenRoot.gameplayChartData() : null;
        let keymode = chartData ? chartData.keymode : 0;
        return keymode === 7 || keymode === 14 ? value : Math.floor(value / 20) * 10;
    }

    function savedScorePoints(savedScore) {
        return screenRoot ? screenRoot.gameplaySavedScorePoints(savedScore) : 0;
    }

    function scoreGraphValue(type) {
        switch (type) {
        case 10:
            return normalized(exScore(1), totalPoints(1));
        case 11: {
            let now = currentNotes(1);
            return now > 0
                ? normalized(exScore(1) * totalNotes(1) / now, totalPoints(1))
                : 0;
        }
        case 12:
            return normalized(screenRoot ? screenRoot.gameplayHighScorePoints() : 0,
                              totalPoints(1));
        case 13:
            return normalized(screenRoot ? screenRoot.gameplayHighScorePoints() : 0,
                              totalPoints(1));
        case 14:
            return normalized(
                screenRoot && screenRoot.battleModeActive()
                    ? exScore(2)
                    : (screenRoot ? screenRoot.gameplayTargetScorePoints() : 0),
                totalPoints(1));
        case 15:
            return normalized(screenRoot ? screenRoot.gameplayTargetFinalPoints() : 0,
                              totalPoints(1));
        default:
            return 0;
        }
    }

    function playerStatGraphValue(type, side) {
        let groupOffset = side === 2 ? 30 : 20;
        switch (type - groupOffset) {
        case 0:
            return normalized(judgementCount(side, 5), totalNotes(side));
        case 1:
            return normalized(judgementCount(side, 4), totalNotes(side));
        case 2:
            return normalized(judgementCount(side, 3), totalNotes(side));
        case 3:
            return normalized(judgementCount(side, 2), totalNotes(side));
        case 4:
            return normalized(poorCount(side), totalNotes(side));
        case 5:
            return normalized(screenRoot ? screenRoot.gameplayCombo(side, true) : 0,
                              totalNotes(side));
        case 6:
            return normalized(scorePrint(side), scorePrintMaximum());
        case 7:
            return normalized(exScore(side), totalPoints(side));
        default:
            return 0;
        }
    }

    function bestStatGraphValue(type) {
        let best = screenRoot ? screenRoot.gameplayBestSavedScore() : null;
        let result = best ? best.result : null;
        let counts = result && result.judgementCounts ? result.judgementCounts : [];
        if (!result) {
            return 0;
        }
        switch (type - 40) {
        case 0:
            return normalized(counts[5] || 0, totalNotes(1));
        case 1:
            return normalized(counts[4] || 0, totalNotes(1));
        case 2:
            return normalized(counts[3] || 0, totalNotes(1));
        case 3:
            return normalized(counts[2] || 0, totalNotes(1));
        case 4:
            return normalized((counts[0] || 0) + (counts[1] || 0),
                              totalNotes(1));
        case 5:
            return normalized(result.maxCombo || 0, totalNotes(1));
        case 6:
            return normalized(resultScorePrint(result), scorePrintMaximum());
        case 7:
            return normalized(savedScorePoints(best), totalPoints(1));
        default:
            return 0;
        }
    }

    function barGraphValue(type) {
        if (!screenRoot || !screenRoot.isGameplayScreen()) {
            return 0;
        }
        if (type === 1 || type === 101) {
            let player = screenRoot.gameplayPlayer(1);
            let elapsed = player ? Math.max(0, player.elapsed || 0) : 0;
            let length = player ? Math.max(0, player.chartLength || 0) : 0;
            return normalized(elapsed, length);
        }
        if (type === 2 || type === 102) {
            return screenRoot.chart && screenRoot.chart.status !== undefined ? 1 : 0;
        }
        if (type >= 110 && type <= 115) {
            return scoreGraphValue(type - 100);
        }
        if (type >= 140 && type <= 147) {
            return playerStatGraphValue(type - 120, 1);
        }
        if (type >= 10 && type <= 15) {
            return scoreGraphValue(type);
        }
        if (type >= 20 && type <= 27) {
            return playerStatGraphValue(type, 1);
        }
        if (type >= 30 && type <= 37) {
            return playerStatGraphValue(type, 2);
        }
        if (type >= 40 && type <= 47) {
            return bestStatGraphValue(type);
        }
        return 0;
    }

    function scratchDirection(side) {
        if (side === 2) {
            if (Input.col2sUp) {
                return -2;
            }
            if (Input.col2sDown) {
                return 2;
            }
        } else {
            if (Input.col1sUp) {
                return -2;
            }
            if (Input.col1sDown) {
                return 2;
            }
        }
        return screenRoot && screenRoot.isGameplayScreen()
            && screenRoot.gameplayStartSkinTime >= 0
            ? 1
            : 0;
    }

    function advanceScratchAngles() {
        let dt = Math.max(0, renderSkinTime - lastScratchSkinTime);
        lastScratchSkinTime = renderSkinTime;
        if (dt <= 0) {
            return;
        }
        scratchAngle1 = (scratchAngle1 + dt * scratchDirection(1) * 360 / 1000) % 360;
        scratchAngle2 = (scratchAngle2 + dt * scratchDirection(2) * 360 / 1000) % 360;
    }

    onRenderSkinTimeChanged: advanceScratchAngles()
    onScreenRootChanged: {
        lastScratchSkinTime = renderSkinTime;
        scratchAngle1 = 0;
        scratchAngle2 = 0;
    }
}
