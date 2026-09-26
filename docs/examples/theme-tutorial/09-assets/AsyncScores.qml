import QtQuick
import RhythmGameQml

// [async-scores]
Item {
    id: root

    required property Profile profile
    property ChartData chartData: null
    property var scores: []
    property bool initialized: false

    PendingReplyGroup {
        id: replies
    }

    function refresh() {
        if (!root.initialized) {
            return;
        }
        replies.cancelAll();
        root.scores = [];
        if (!root.enabled || !root.profile || !root.chartData) {
            return;
        }
        const md5 = root.chartData.md5;
        replies.track(root.profile.scoreDb.getScoresForMd5([md5])).then(result => {
            root.scores = result.scores[md5] || [];
        });
    }

    onChartDataChanged: refresh()
    onProfileChanged: refresh()
    onEnabledChanged: refresh()
    Component.onCompleted: {
        root.initialized = true;
        root.refresh();
    }
}
// [async-scores]
