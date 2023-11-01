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