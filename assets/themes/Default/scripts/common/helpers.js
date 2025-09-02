const clearTypePriorities = ["NOPLAY", "FAILED", "AEASY", "EASY", "NORMAL", "HARD", "EXHARD", "FC", "PERFECT", "MAX"]

function getClearType(scores) {
    let clearType = "NOPLAY";
    for (let score of scores) {
        if (clearTypePriorities.indexOf(score.result.clearType) > clearTypePriorities.indexOf(clearType)) {
            clearType = score.result.clearType;
        }
    }
    return clearType;
}

function getScoreWithBestPoints(scores) {
    let bestPoints = 0;
    let bestScore = null;
    for (let score of scores) {
        if (score.result.maxPoints === 0) {
            continue;
        }
        let percent = score.result.points / score.result.maxPoints;
        if (percent > bestPoints) {
            bestPoints = percent;
            bestScore = score;
        }
    }
    return bestScore;
}

function getBestStats(scores) {
    // create an object with the clear types as keys and the scores as values
    let best = {};
    if (!scores || scores.length === 0) {
        return null;
    }
    for (let score of scores) {
        if (score.result.maxCombo > best.maxCombo || best.maxCombo === undefined) {
            best.maxCombo = score.result.maxCombo;
        }
        let missCount = score.result.judgementCounts[Judgement.Poor] + score.result.judgementCounts[Judgement.EmptyPoor] + score.result.judgementCounts[Judgement.Bad];
        if (missCount < best.missCount || best.missCount === undefined) {
            best.missCount = missCount;
        }
        let comboBreak = score.result.judgementCounts[Judgement.Poor] + score.result.judgementCounts[Judgement.Bad];
        if (comboBreak < best.comboBreak || best.comboBreak === undefined) {
            best.comboBreak = comboBreak;
        }
    }
    return best;
}


function getGrade(points, maxPoints) {
    if (points === maxPoints || maxPoints === 0) {
        return "max";
    }
    let percent = points / maxPoints;
    if (percent >= 0.88) {
        return "aaa";
    } else if (percent >= 0.77) {
        return "aa";
    } else if (percent >= 0.66) {
        return "a";
    } else if (percent >= 0.55) {
        return "b";
    } else if (percent >= 0.44) {
        return "c";
    } else if (percent >= 0.33) {
        return "d";
    } else if (percent >= 0.22) {
        return "e";
    } else {
        return "f";
    }
}

function pad(num, size) {
    num = num.toString();
    while (num.length < size) num = "0" + num;
    return num;
}

function getEarlyLate(replayData) {
    // array with a value for each judgement
    let early = [];
    let late = [];
    for (let i = 0; i <= Judgement.Perfect; i++) {
        early.push(0);
        late.push(0);
    }
    for (let hit of replayData.hitEvents) {
        if (!hit.points) {
            continue;
        }
        let delta = hit.points.deviation;
        if (delta < 0) {
            early[hit.points.judgement]++;
        } else {
            late[hit.points.judgement]++;
        }
    }

    return {
        early: early,
        late: late
    };
}

function capitalizeFirstLetter(string) {
    return string.charAt(0).toUpperCase() + string.slice(1);
}

function getIndex(array, elem, currentIndex) {
    let index = 0;
    let proposition = null;
    for (let choice of array) {
        if (elem == choice) {
            // special handling for duplicated elements
            if (proposition === null || Math.abs(currentIndex - index) < Math.abs(currentIndex - proposition)) {
                proposition = index
            }
        }
        index++;
    }
    return proposition;
}

function getFormattedNumber(locale, num) {
    if (isNaN(num)) {
        return "";
    }
    let longNum = locale.toString(num, "f", -128);
    let shortNum = locale.toString(num, "f", 3);
    if (longNum.length > shortNum.length) {
        return longNum;
    }
    return shortNum;
}