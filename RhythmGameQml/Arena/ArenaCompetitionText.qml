import QtQml

QtObject {
    function stateText(connected, state): string {
        switch (String(state || "")) {
        case "finished":
            return qsTr("Finished");
        case "dnf":
            return qsTr("DNF");
        default:
            break;
        }
        if (!connected) {
            return qsTr("Disconnected");
        }
        switch (String(state || "")) {
        case "loading":
            return qsTr("Loading");
        case "playing":
            return qsTr("Playing");
        default:
            return qsTr("Waiting");
        }
    }

    function gaugeTypeText(value): string {
        switch (String(value || "")) {
        case "fc":
            return qsTr("Full combo");
        case "exhard":
            return qsTr("EX Hard");
        case "hard":
            return qsTr("Hard");
        case "normal":
            return qsTr("Normal");
        case "easy":
            return qsTr("Easy");
        case "aeasy":
            return qsTr("Assist Easy");
        default:
            return qsTr("Unknown");
        }
    }

    function clearTypeText(value): string {
        switch (String(value || "")) {
        case "max":
            return qsTr("MAX");
        case "perfect":
            return qsTr("Perfect");
        case "fc":
            return qsTr("Full combo");
        case "exhard":
            return qsTr("EX Hard");
        case "hard":
            return qsTr("Hard");
        case "normal":
            return qsTr("Normal");
        case "easy":
            return qsTr("Easy");
        case "aeasy":
            return qsTr("Assist Easy");
        case "failed":
            return qsTr("Failed");
        default:
            return qsTr("Unknown");
        }
    }

    function dnfReasonText(value): string {
        switch (String(value || "")) {
        case "aborted":
            return qsTr("Aborted");
        case "result_unavailable":
            return qsTr("Result unavailable");
        case "left":
            return qsTr("Left the room");
        case "kicked":
            return qsTr("Kicked");
        case "grace_expired":
            return qsTr("Reconnect grace expired");
        case "play_deadline":
            return qsTr("Play deadline expired");
        default:
            return qsTr("Unknown");
        }
    }

    function winsText(wins): string {
        const count = Number(wins);
        return count >= 0 ? qsTr("%n win(s)", "Arena lobby wins", count) : qsTr("Wins —");
    }

    function gaugeText(gaugeType, gaugeValueMilli): string {
        return qsTr("%1 · %2%").arg(gaugeTypeText(gaugeType)).arg((Number(gaugeValueMilli) / 1000).toFixed(1));
    }

    function gaugeValueText(gaugeValueMilli): string {
        return (Number(gaugeValueMilli) / 1000).toFixed(1) + "%";
    }

    function gaugePasses(gaugeType, gaugeValueMilli): bool {
        const value = Number(gaugeValueMilli);
        switch (String(gaugeType || "")) {
        case "fc":
            return value > 2000;
        case "exhard":
            return value > 0;
        case "hard":
            return value > 2000;
        case "normal":
        case "easy":
            return value > 80000;
        case "aeasy":
            return value > 60000;
        default:
            return false;
        }
    }

    function currentClearText(maxCombo, perfect, great, good, bad, poor, emptyPoor, gaugeType, gaugeValueMilli): string {
        const pg = Math.max(0, Number(perfect));
        const gr = Math.max(0, Number(great));
        const gd = Math.max(0, Number(good));
        const bd = Math.max(0, Number(bad));
        const pr = Math.max(0, Number(poor));
        const ep = Math.max(0, Number(emptyPoor));
        if (gr + gd + bd + pr + ep === 0) {
            return qsTr("MAX");
        }
        if (gd + bd + pr + ep === 0) {
            return qsTr("Perfect");
        }
        if (String(gaugeType || "") === "fc" && gaugePasses(gaugeType, gaugeValueMilli)) {
            return qsTr("FC");
        }
        const judgedNotes = pg + gr + gd + bd + pr + ep;
        if (Number(maxCombo) === judgedNotes) {
            return qsTr("FC");
        }
        if (!gaugePasses(gaugeType, gaugeValueMilli)) {
            return qsTr("Failed");
        }
        return qsTr("%1 clear").arg(gaugeTypeText(gaugeType));
    }

    function outcomeText(clearType, lobbyWinsAfter, dnfReason): string {
        const parts = [];
        if (String(dnfReason || "").length > 0) {
            parts.push(dnfReasonText(dnfReason));
        } else if (String(clearType || "").length > 0) {
            parts.push(qsTr("%1 clear").arg(clearTypeText(clearType)));
        }
        if (Number(lobbyWinsAfter) >= 0) {
            parts.push(winsText(lobbyWinsAfter));
        }
        return parts.join(qsTr(" · "));
    }

    function resultDetailsText(competitionState, dnfReason, badPoorCount, maxCombo, clearType): string {
        if (competitionState === "dnf") {
            return qsTr("Did not finish · %1").arg(dnfReasonText(dnfReason));
        }
        return qsTr("BP %1 · Combo %2 · %3").arg(badPoorCount).arg(maxCombo).arg(clearTypeText(clearType));
    }

    function nativeResultDetailsText(competitionState, dnfReason, badPoorCount, maxCombo, clearType, gaugeValueMilli): string {
        if (competitionState === "dnf") {
            return qsTr("Did not finish · %1").arg(dnfReasonText(dnfReason));
        }
        return qsTr("BP %1 · Combo %2 · %3 · %4").arg(badPoorCount).arg(maxCombo).arg(clearTypeText(clearType)).arg(gaugeValueText(gaugeValueMilli));
    }
}
