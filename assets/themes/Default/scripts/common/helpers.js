const clearTypePriorities = ["NOPLAY", "FAILED", "AEASY", "EASY", "NORMAL", "HARD", "EXHARD", "FC", "PERFECT", "MAX"]

function getClearType(scores) {
    let clearType = "NOPLAY";
    for (let i = 0; i < scores.length; i++) {
        let score = scores[i];
        if (clearTypePriorities.indexOf(score.clearType) > clearTypePriorities.indexOf(clearType)) {
            clearType = score.clearType;
        }
    }
    return clearType;
}

function getScoreWithBestPoints(scores) {
    let bestPoints = 0;
    let bestScore = null;
    for (let score of scores) {
        if (score.maxPoints === 0) {
            continue;
        }
        let percent = score.points / score.maxPoints;
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
        if (score.maxCombo > best.maxCombo || best.maxCombo === undefined) {
            best.maxCombo = score.maxCombo;
        }
        let missCount = score.judgementCounts[Judgement.Poor] + score.judgementCounts[Judgement.EmptyPoor] + score.judgementCounts[Judgement.Bad];
        if (missCount < best.missCount || best.missCount === undefined) {
            best.missCount = missCount;
        }
        let comboBreak = score.judgementCounts[Judgement.Poor] + score.judgementCounts[Judgement.Bad];
        if (comboBreak < best.comboBreak || best.comboBreak === undefined) {
            best.comboBreak = comboBreak;
        }
    }
    return best;
}


function getGrade(points, maxPoints) {
    if (points === maxPoints) {
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

function getEarlyLate(replayData, missCount) {
    // array with a value for each judgement
    let early = [];
    let late = [];
    for (let i = 0; i < Judgement.Perfect + 1; i++) {
        early.push(0);
        late.push(0);
    }
    let hitsWithPoints = replayData.hitsWithPoints;
    for (let hit of hitsWithPoints) {
        let delta = hit.points.deviation;
        if (delta < 0) {
            early[hit.points.judgement]++;
        } else {
            late[hit.points.judgement]++;
        }
    }
    late[Judgement.Poor] = missCount;

    return {
        early: early,
        late: late
    };
}

function capitalizeFirstLetter(string) {
    return string.charAt(0).toUpperCase() + string.slice(1);
}