pragma ValueTypeBehavior: Addressable
import QtQuick
import QtQuick.Controls.FluentWinUI3
import QtQuick.Layouts
import RhythmGameQml

FocusScope {
    id: screen

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
    }
    // [select-controller]

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
