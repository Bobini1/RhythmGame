pragma ValueTypeBehavior: Addressable
import QtQuick
import QtQuick.Controls.FluentWinUI3
import QtQuick.Layouts
import RhythmGameQml

FocusScope {
    id: screen

    // [assets-state]
    readonly property ChartData chartData: selection.focusedItem instanceof ChartData
        ? selection.focusedItem : null
    readonly property var chartResults: selection.results[selection.focusedItem?.md5] || []

    // [assets-state]

    // [judge-rank]
    readonly property string judgeRankText: {
        const rank = screen.chartData?.rank;
        const names = {
            25: qsTr("Very hard"),
            50: qsTr("Hard"),
            75: qsTr("Normal"),
            100: qsTr("Easy")
        };
        return rank === undefined ? qsTr("Unknown")
            : names[rank] ?? qsTr("Custom (%1)").arg(rank);
    }
    // [judge-rank]

    // [entry-labels]
    function labelFor(item) {
        if (typeof item === "string") {
            const path = item.replace(/\\/g, "/").replace(/\/$/, "");
            const name = path.substring(path.lastIndexOf("/") + 1) || path;
            return qsTr("Folder: %1").arg(name);
        }
        if (item instanceof ChartData)
            return item.title || qsTr("Untitled chart");
        if (item instanceof entry)
            return qsTr("%1 (not installed)").arg(item.title);
        if (item instanceof table)
            return qsTr("Table: %1").arg(item.name);
        if (item instanceof level)
            return qsTr("Level: %1").arg(item.name);
        if (item instanceof course)
            return qsTr("Course: %1").arg(item.name);
        return "";
    }
    // [entry-labels]

    // [select-controller]
    StandardSelectController {
        id: selection

        // [preview]
        // [preview]

    }
    // [select-controller]

    // [score-details]
    ScoreDetails {
        id: scoreDetails
        result: screen.chartResults[0] || null
    }
    // [score-details]

    Pane {
        anchors.fill: parent
        focus: true

        ColumnLayout {
            anchors.fill: parent
            spacing: 12

            RowLayout {
                Layout.fillWidth: true

                Button {
                    text: qsTr("Back")
                    onClicked: selection.goBack()
                }

                Button {
                    text: qsTr("Leave selection")
                    onClicked: selection.exit()
                }

                Label {
                    Layout.fillWidth: true
                    text: qsTr("Choose a folder or chart")
                    elide: Text.ElideRight
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.maximumHeight: 180

                ColumnLayout {
                    Layout.fillHeight: true
                    Layout.fillWidth: true
                    Layout.preferredWidth: 320

                    // [chart-image]
                    Image {
                        id: stageImage
                        Layout.preferredWidth: 160
                        Layout.preferredHeight: 90
                        Layout.fillHeight: true
                        source: screen.chartData ? screen.chartData.stageFileSource : ""
                        sourceSize: Qt.size(320, 180)
                        fillMode: Image.PreserveAspectFit
                        asynchronous: true

                        Label {
                            anchors.centerIn: parent
                            visible: stageImage.status !== Image.Ready
                            text: qsTr("No image")
                        }
                    }
                    // [chart-image]

                    // [saved-results]
                    Label {
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                        text: qsTr("%1 saved scores for the selected chart.")
                            .arg(screen.chartResults.length)
                    }
                    Label {
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                        text: qsTr("Best clear: %1, best score: %2")
                            .arg(selection.ranking.bestClearTypeResult?.clearType ?? "NOPLAY")
                            .arg(selection.ranking.bestPointsResult?.points ?? 0)
                    }
                    // [saved-results]
                }

                // [recording-graph]
                HitDistribution {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.preferredWidth: 320
                    Layout.preferredHeight: 180
                    title: qsTr("Last score, rank: %1").arg(screen.judgeRankText)
                    replayData: scoreDetails.score?.replayData ?? null
                    unavailableText: !scoreDetails.result ? qsTr("No saved score")
                        : scoreDetails.loading ? qsTr("Loading recording...")
                        : scoreDetails.status === ScoreDetails.Error
                            ? qsTr("Could not load this recording")
                            : qsTr("This score has no recording")
                }
                // [recording-graph]
            }

            // [select-ranking]
            GridLayout {
                id: rankingControls
                Layout.fillWidth: true
                columns: width < 400 ? 2 : 3
                visible: selection.ranking.available

                ComboBox {
                    model: [
                        { value: OnlineRankingModel.RhythmGame, text: "RhythmGame" },
                        { value: OnlineRankingModel.Tachi, text: "Bokutachi" },
                        { value: OnlineRankingModel.LR2IR, text: "LR2IR" }
                    ]
                    textRole: "text"
                    valueRole: "value"
                    currentValue: selection.ranking.provider
                    onActivated: selection.ranking.profile.vars.generalVars.rankingProvider = currentValue
                    Accessible.name: qsTr("Ranking provider")
                }
                Label {
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                    text: selection.ranking.loading ? qsTr("Loading ranking...")
                        : selection.ranking.position > 0
                            ? qsTr("Rank %1 of %2").arg(selection.ranking.position)
                                .arg(selection.ranking.playerCount)
                            : qsTr("%1 ranked players").arg(selection.ranking.playerCount)
                }
                Button {
                    Layout.columnSpan: rankingControls.columns === 2 ? 2 : 1
                    Layout.alignment: Qt.AlignRight
                    text: qsTr("Open ranking")
                    enabled: !selection.ranking.opening
                    onClicked: selection.openInternetRanking()
                }
            }
            // [select-ranking]
            // [select-keymode]
            Flow {
                Layout.fillWidth: true
                spacing: 6

                Repeater {
                    model: [
                        { value: SelectKeymodeFilter.All, text: qsTr("All") },
                        { value: SelectKeymodeFilter.Single, text: qsTr("SP") },
                        { value: SelectKeymodeFilter.Double, text: qsTr("DP") },
                        { value: SelectKeymodeFilter.K5, text: qsTr("5K") },
                        { value: SelectKeymodeFilter.K7, text: qsTr("7K") },
                        { value: SelectKeymodeFilter.K10, text: qsTr("10K") },
                        { value: SelectKeymodeFilter.K14, text: qsTr("14K") }
                    ]

                    delegate: Button {
                        id: keymodeButton

                        required property var modelData

                        text: modelData.text
                        checkable: true
                        autoExclusive: true
                        checked: selection.keymodeFilter === modelData.value
                        onClicked: {
                            selection.setKeymodeFilter(keymodeButton.modelData.value);
                            songList.forceActiveFocus();
                        }
                    }
                }
            }
            // [select-keymode]

            // [select-empty]
            Label {
                Layout.fillWidth: true
                visible: selection.entries.length === 0
                text: qsTr("No entries. Check song folders and filters.")
                wrapMode: Text.WordWrap
            }
            // [select-empty]

            // [select-list]
            StandardSelectListView {
                id: songList

                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumHeight: 60
                clip: true
                focus: true
                controller: selection
                ScrollBar.vertical: ScrollBar {}

                // [select-delegate]
                delegate: ItemDelegate {
                    id: row

                    required property int index
                    required property var modelData

                    width: ListView.view.width
                    text: screen.labelFor(modelData)
                    highlighted: ListView.isCurrentItem

                    onClicked: songList.activate(row.index)
                }
                // [select-delegate]
            }
            // [select-list]

            // [course-stages]
            CourseStages {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredHeight: 140
                Layout.maximumHeight: 140
                courseData: selection.focusedItem instanceof course
                    ? selection.focusedItem : null
                visible: courseData !== null
            }
            // [course-stages]

            // [select-actions]
            RowLayout {
                Button {
                    text: qsTr("Sort: %1").arg(selection.sortModeText)
                    onClicked: selection.cycleSortMode()
                }
                Button {
                    text: qsTr("Autoplay")
                    enabled: selection.autoplayAvailable
                    onClicked: selection.openAutoplay()
                }
            }
            // [select-actions]

            // [select-replay]
            RowLayout {
                ComboBox {
                    model: selection.replayOptions
                    textRole: "text"
                    valueRole: "value"
                    currentValue: selection.replayType
                    onActivated: selection.setReplayType(currentValue)
                }

                Button {
                    text: qsTr("Replay")
                    enabled: selection.replayAvailable
                    onClicked: selection.openReplay()
                }
            }
            // [select-replay]
        }
    }

    // [arena-select]
    StandardArenaSelectOverlay {
        navigationFocusTarget: songList
    }
    // [arena-select]

}
