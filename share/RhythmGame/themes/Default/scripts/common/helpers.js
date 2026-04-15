const clearTypePriorities = ["NOPLAY", "FAILED", "AEASY", "EASY", "NORMAL", "HARD", "EXHARD", "FC", "PERFECT", "MAX"]

function getClearType(scores) {
    let clearType = "NOPLAY";
    for (let score of scores) {
        if (clearTypePriorities.indexOf(score?.result?.clearType) > clearTypePriorities.indexOf(clearType)) {
            clearType = score.result.clearType;
        }
    }
    return clearType;
}

function getScoreWithBestPoints(scores) {
    let bestPoints = -1;
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

function getScoreWithBestClear(scores) {
    let bestClear = "";
    let bestScore = null;
    let bestPoints = 0;
    for (let score of scores) {
        let oldIndex = clearTypePriorities.indexOf(bestClear);
        let newIndex = clearTypePriorities.indexOf(score.result.clearType);
        if (newIndex >= oldIndex || bestScore === null) {
            if (newIndex === oldIndex && score.result.points / (score.result.maxPoints || 1) < bestPoints) {
                continue;
            }
            bestClear = score.result.clearType;
            bestScore = score;
            bestPoints = score.result.points / (score.result.maxPoints || 1);
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
    if (percent >= 8.0 / 9.0) {
        return "aaa";
    } else if (percent >= 7.0 / 9.0) {
        return "aa";
    } else if (percent >= 6.0 / 9.0) {
        return "a";
    } else if (percent >= 5.0 / 9.0) {
        return "b";
    } else if (percent >= 4.0 / 9.0) {
        return "c";
    } else if (percent >= 3.0 / 9.0) {
        return "d";
    } else if (percent >= 2.0 / 9.0) {
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

function getStddevAndMean(replayData) {
    if (!replayData || !replayData.hitEvents) {
        return {mean: 0, stddev: 0};
    }
    const vals = [];
    for (let hit of replayData.hitEvents) {
        if (!hit.points) continue;
        if (hit.points.judgement > Judgement.Perfect || hit.points.judgement < Judgement.Bad) continue;
        const d = hit.points.deviation;
        vals.push(d);
    }
    if (vals.length === 0) return {mean: 0, stddev: 0};
    let sum = 0;
    for (let v of vals) sum += v;
    const mean = sum / vals.length;
    let sq = 0;
    for (let v of vals) {
        const diff = v - mean;
        sq += diff * diff;
    }
    const variance = sq / vals.length; // population variance
    const stddev = Math.sqrt(variance);
    return {mean: mean, stddev: stddev};
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

function getFormattedNumber(locale, num, decimals = 3) {
    if (isNaN(num)) {
        return "";
    }
    let longNum = locale.toString(num, "f", -128);
    let shortNum = locale.toString(num, "f", decimals);
    if (longNum.length < shortNum.length) {
        return longNum;
    }
    return shortNum;
}

/**
 * Returns the full difficulty name for BMS difficulty integers 1–5.
 * @param {number} diff - ChartData.difficulty
 * @returns {string}
 */
function difficultyName(diff) {
    switch (diff) {
        case 1:
            return "BEGINNER";
        case 2:
            return "NORMAL";
        case 3:
            return "HYPER";
        case 4:
            return "ANOTHER";
        case 5:
            return "INSANE";
        default:
            return "";
    }
}

/**
 * Replaces characters forbidden in filenames on Windows / macOS / Linux
 * with underscores.
 * @param {string} str
 * @returns {string}
 */
function sanitizeFilename(str) {
    return str.replace(/[/\\:*?"<>|]/g, "_");
}
