import QtQuick
import QtQuick.Controls.FluentWinUI3
import QtQuick.Layouts
import RhythmGameQml

ColumnLayout {
    id: root

    required property var results

    Button {
        id: detailsButton
        checkable: true
        text: detailsButton.checked ? qsTr("Hide details") : qsTr("Show details")
    }

    // [lazy-panel]
    Loader {
        Layout.fillWidth: true
        active: root.enabled && detailsButton.checked
        sourceComponent: ColumnLayout {
            // [score-details]
            ScoreDetails {
                id: details
                result: root.results[0] || null
            }

            Label {
                text: qsTr("%1 saved scores for the selected chart.").arg(root.results.length)
            }
            Label {
                Layout.fillWidth: true
                wrapMode: Text.Wrap
                text: !details.result ? qsTr("No saved score")
                    : details.loading ? qsTr("Loading latest score...")
                    : details.status === ScoreDetails.Error ? qsTr("Could not load this score")
                    : !details.score?.replayData ? qsTr("This score has no recording")
                    : qsTr("Early hits in the latest score: %1").arg(root.countEarly(details.score))
            }
            // [score-details]
        }
    }
    // [lazy-panel]

    // [score-statistics]
    function countEarly(score) {
        let count = 0;
        for (const hit of score.replayData.hitEvents) {
            if (hit.noteRemoved && hit.points && hit.points.deviation < 0)
                ++count;
        }
        return count;
    }
    // [score-statistics]

    // [profile-connection]
    Connections {
        target: Rg.profileList
        enabled: root.enabled

        function onMainProfileChanged() {
            detailsButton.checked = false;
        }
    }
    // [profile-connection]

    // [screen-shortcut]
    Shortcut {
        sequence: "F7"
        enabled: root.enabled
        onActivated: detailsButton.toggle()
    }
    // [screen-shortcut]
}
