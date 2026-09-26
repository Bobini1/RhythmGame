pragma ComponentBehavior: Bound
pragma ValueTypeBehavior: Addressable
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import RhythmGameQml

FocusScope {
    id: screen

    function labelFor(item) {
        if (typeof item === "string") {
            const path = item.replace(/\\/g, "/").replace(/\/$/, "");
            return path.substring(path.lastIndexOf("/") + 1) || path;
        }
        return item ? (item.title || item.name || qsTr("Unnamed item")) : "";
    }

    function activateCurrent() {
        if (selection.focusedItem !== null) {
            selection.goForward(selection.focusedItem);
        }
    }

    // [select-controller]
    StandardSelectController {
        id: selection

        enabled: screen.enabled
        autoInitialize: false
        openInternetRankingShortcutEnabled: false

        onFocusRequested: index => songList.currentIndex = index
        onMoveRequested: steps => {
            if (songList.count > 0) {
                const next = songList.currentIndex + steps;
                songList.currentIndex =
                    ((next % songList.count) + songList.count) % songList.count;
            }
        }
    }

    Component.onCompleted: selection.initialize()
    // [select-controller]

    Pane {
        anchors.fill: parent

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

            ListView {
                id: songList

                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                focus: true
                keyNavigationEnabled: false
                model: selection.entries
                currentIndex: -1
                ScrollBar.vertical: ScrollBar {}

                // [select-focus]
                onCurrentIndexChanged: {
                    if (currentIndex >= 0 && currentIndex < count) {
                        selection.setFocused(selection.entries[currentIndex]);
                    }
                }

                Keys.onUpPressed: event => selection.handleUpPressed(event)
                Keys.onDownPressed: event => selection.handleDownPressed(event)
                Keys.onReleased: event => selection.handleReleased(event)
                Keys.onReturnPressed: screen.activateCurrent()
                Keys.onEnterPressed: screen.activateCurrent()
                Keys.onRightPressed: screen.activateCurrent()
                Keys.onLeftPressed: selection.goBack()
                // [select-focus]

                delegate: ItemDelegate {
                    id: row

                    required property int index
                    required property var modelData

                    width: ListView.view.width
                    text: screen.labelFor(modelData)
                    highlighted: ListView.isCurrentItem

                    onClicked: {
                        songList.forceActiveFocus();
                        songList.currentIndex = row.index;
                        selection.setFocused(row.modelData);
                        screen.activateCurrent();
                    }
                }

                Label {
                    anchors.centerIn: parent
                    visible: songList.count === 0
                    text: qsTr("No entries. Check song folders and filters.")
                }
            }
        }
    }

    // [arena-select]
    StandardArenaSelectOverlay {
        readyShortcutDescription: selection.readyEnabled
            ? selection.readyShortcutDescription : ""
        navigationFocusTarget: songList
        z: 10
    }
    // [arena-select]

}
