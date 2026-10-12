import QtQuick
import QtQuick.Controls.FluentWinUI3
import QtQuick.Layouts
import RhythmGameQml

ColumnLayout {
    id: distribution
    required property var replayData
    property string title: qsTr("Hit timing")
    property string unavailableText: qsTr("Replay data unavailable")

    // [timing-buckets]
    readonly property var histogram: {
        const offsets = [];
        const judgements = [Judgement.Perfect, Judgement.Great, Judgement.Good, Judgement.Bad];
        let largest = 0;
        for (const hit of replayData?.hitEvents ?? []) {
            if (!judgements.includes(hit.judgement)) continue;
            const millis = hit.deviation / 1000000;
            offsets.push(millis);
            largest = Math.max(largest, Math.abs(millis));
        }
        const limit = Math.max(100, Math.ceil(largest / 50) * 50);
        const step = limit / 20;
        const counts = Array(41).fill(0);
        for (const millis of offsets)
            counts[Math.round(millis / step) + 20]++;
        return { counts: counts, limit: limit, step: step,
            total: offsets.length, peak: Math.max(1, ...counts) };
    }
    // [timing-buckets]

    Label {
        Layout.fillWidth: true
        text: distribution.title
        font.bold: true
        wrapMode: Text.Wrap
    }
    Label {
        Layout.fillWidth: true
        text: qsTr("%1 hits, %2 ms per bin").arg(distribution.histogram.total)
            .arg(distribution.histogram.step)
        wrapMode: Text.Wrap
    }

    // [timing-bars]
    Rectangle {
        id: plot
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.preferredHeight: 160
        color: "#18232e"

        Repeater {
            model: distribution.histogram.counts
            Rectangle {
                required property int index
                required property int modelData
                x: index * plot.width / 41
                y: plot.height - height
                width: Math.max(0, plot.width / 41 - 1)
                height: modelData / distribution.histogram.peak * plot.height
                color: index < 20 ? "#9de6ff" : index > 20 ? "#e8bc89" : "#ffffff"
            }
        }
        Rectangle {
            x: (plot.width - width) / 2
            width: 1
            height: plot.height
            color: "#708090"
        }
        Label {
            anchors.fill: parent
            anchors.margins: 8
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.Wrap
            visible: distribution.histogram.total === 0
            text: distribution.replayData ? qsTr("No replay data") : distribution.unavailableText
        }
    }
    // [timing-bars]

    RowLayout {
        Layout.fillWidth: true
        Label { text: qsTr("−%1 ms\nEarly").arg(distribution.histogram.limit) }
        Label { Layout.fillWidth: true; text: qsTr("0 ms"); horizontalAlignment: Text.AlignHCenter }
        Label {
            text: qsTr("+%1 ms\nLate").arg(distribution.histogram.limit)
            horizontalAlignment: Text.AlignRight
        }
    }
}
