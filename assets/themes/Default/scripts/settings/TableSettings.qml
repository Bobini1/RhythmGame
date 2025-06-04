pragma ValueTypeBehavior: Addressable
import QtQuick
import QtQuick.Controls.Basic
import RhythmGameQml
import QtQuick.Layouts
import QtQuick.Shapes

Item {
    id: tableSettings
    Component {
        id: dragDelegate

        MouseArea {
            id: dragArea

            property bool held: false

            required property int index
            required property var display

            anchors {
                left: parent
                ?.
                left
                right: parent
                ?.
                right
            }
            height: content.height

            drag.target: held ? content : undefined
            drag.axis: Drag.YAxis

            onPressed: held = true
            onReleased: held = false


            Rectangle {
                id: content
                anchors {
                    horizontalCenter: parent.horizontalCenter
                    verticalCenter: parent.verticalCenter
                }
                width: dragArea.width
                height: Math.max(64, row.implicitHeight + 8)

                border.width: 1
                border.color: "lightsteelblue"

                color: dragArea.held ? "lightsteelblue" : "white"
                Behavior on color {
                    ColorAnimation {
                        duration: 100
                    }
                }

                radius: 2
                Drag.active: dragArea.held
                Drag.source: dragArea
                Drag.hotSpot.x: width / 2
                Drag.hotSpot.y: height / 2
                states: State {
                    when: dragArea.held

                    ParentChange {
                        target: content
                        parent: tableSettings
                    }
                    AnchorChanges {
                        target: content
                        anchors {
                            horizontalCenter: undefined
                            verticalCenter: undefined
                        }
                    }
                }

                RowLayout {
                    id: row

                    anchors.fill: parent
                    spacing: 10
                    anchors.margins: 5
                    Item {
                        implicitWidth: 300
                        implicitHeight: tableUrl.implicitHeight
                        Layout.fillWidth: true
                        TextEdit {
                            id: tableUrl
                            readOnly: true
                            wrapMode: TextEdit.Wrap
                            text: dragArea.display.url
                            width: Math.min(implicitWidth, parent.width)
                        }
                    }
                    Item {
                        implicitWidth: 100
                        implicitHeight: tableName.implicitHeight
                        Layout.fillWidth: true
                        TextEdit {
                            id: tableName
                            readOnly: true
                            wrapMode: TextEdit.Wrap
                            text: dragArea.display.name
                            width: Math.min(implicitWidth, parent.width)
                        }
                    }
                    Component {
                        id: defaultItem
                        Item {
                        }
                    }
                    Loader {
                        Component {
                            id: errorItem
                            Shape {
                                id: errorIcon
                                visible: dragArea.display.status === table.Error
                                ShapePath {
                                    strokeColor: "red"
                                    strokeWidth: 4
                                    fillColor: "transparent"
                                    startX: 0
                                    startY: 0
                                    PathLine {
                                        x: 32; y: 32
                                    }
                                }
                                ShapePath {
                                    strokeColor: "red"
                                    strokeWidth: 4
                                    fillColor: "transparent"
                                    startX: 32
                                    startY: 0
                                    PathLine {
                                        x: 0; y: 32
                                    }
                                }
                            }
                        }
                        Component {
                            id: loadingItem
                            Item {
                                BusyIndicator {
                                    running: dragArea.display.status === table.Loading
                                    anchors.fill: parent
                                    anchors.margins: -8
                                }
                            }
                        }
                        width: 32
                        height: 32

                        sourceComponent: {
                            if (dragArea.display.status === table.Error) {
                                return errorItem;
                            } else if (dragArea.display.status === table.Loading) {
                                return loadingItem;
                            }
                            return defaultItem;
                        }
                    }
                    Button {
                        text: qsTr("Reload")

                        onClicked: {
                            Rg.tables.reload(dragArea.index);
                        }
                    }
                    Button {
                        text: qsTr("Remove")

                        onClicked: {
                            Rg.tables.removeAt(dragArea.index);
                        }
                    }
                }
            }
            DropArea {
                anchors {
                    fill: parent
                    margins: 10
                }

                onEntered: (drag) => {
                    Rg.tables.reorder(
                        drag.source.index,
                        dragArea.index)
                }
            }
        }
    }

    ScrollView {
        id: rootScrollView
        anchors {
            top: parent.top
            bottom: parent.bottom
            horizontalCenter: parent.horizontalCenter
        }
        width: Math.min(1200, parent.width)
        contentWidth: Math.max(600, width)
        contentHeight: Math.max(rootFrame.implicitHeight, parent.height)

        Frame {
            id: rootFrame
            anchors.fill: parent

            ColumnLayout {
                anchors.fill: parent
                ScrollView {
                    Layout.fillHeight: true
                    Layout.fillWidth: true

                    ListView {
                        id: songList

                        clip: true
                        spacing: 5
                        model: Rg.tables
                        delegate: dragDelegate
                    }
                }
                RowLayout {
                    Layout.fillWidth: true

                    TextField {
                        id: textField
                        Layout.fillWidth: true
                        Layout.preferredWidth: 3
                        placeholderText: qsTr("Add table")

                        onAccepted: {
                            Rg.tables.add(text);
                            text = "";
                        }
                    }
                    Button {
                        Layout.fillWidth: true
                        Layout.preferredWidth: 1
                        text: qsTr("Add")

                        onClicked: {
                            Rg.tables.add(textField.text);
                            textField.text = "";
                        }
                    }
                }
            }
        }
    }
}