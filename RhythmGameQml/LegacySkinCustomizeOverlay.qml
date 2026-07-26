pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

FocusScope {
    id: root

    required property var screen

    function choiceIndex(item) {
        if (!item || !item.choices || item.choices.length === 0) {
            return -1;
        }
        const value = root.screen.legacySkinCustomizeValue(item);
        const index = item.choices.indexOf(value);
        return index >= 0 ? index : 0;
    }

    anchors.fill: parent
    focus: true

    Component.onCompleted: forceActiveFocus()

    Keys.onEscapePressed: event => {
        root.screen.customizeMode = false;
        event.accepted = true;
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.AllButtons

        onPressed: mouse => mouse.accepted = true
        onWheel: wheel => wheel.accepted = true
    }

    Rectangle {
        id: editor

        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.margins: 16
        border.color: "#63707d"
        border.width: 1
        color: "#f21a1d21"
        height: Math.max(240, parent.height - 32)
        radius: 6
        width: Math.min(440, Math.max(320, parent.width * 0.32))

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 8
                Layout.topMargin: 8
                Layout.bottomMargin: 8
                spacing: 8

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 0
                    spacing: 2

                    Label {
                        Layout.fillWidth: true
                        color: "#f5f7fa"
                        elide: Text.ElideRight
                        font.bold: true
                        font.pixelSize: 18
                        text: root.screen.legacySkinCustomizeTitle
                    }

                    Label {
                        Layout.fillWidth: true
                        color: "#b6c0ca"
                        elide: Text.ElideRight
                        font.pixelSize: 12
                        text: root.screen.legacySkinCustomizeSubtitle
                    }
                }

                ToolButton {
                    Accessible.name: qsTr("Close customize mode")
                    display: AbstractButton.TextOnly
                    font.pixelSize: 22
                    text: "\u00d7"

                    onClicked: root.screen.customizeMode = false

                    ToolTip.text: Accessible.name
                    ToolTip.visible: hovered
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: "#46505a"
            }

            ScrollView {
                Layout.fillHeight: true
                Layout.fillWidth: true
                clip: true
                contentWidth: availableWidth

                ColumnLayout {
                    width: Math.max(0, parent.width)
                    spacing: 0

                    CheckBox {
                        Layout.fillWidth: true
                        Layout.leftMargin: 12
                        Layout.rightMargin: 12
                        Layout.topMargin: 10
                        Layout.bottomMargin: 10
                        checked: root.screen.arenaPanelVisible
                        text: qsTr("Arena panel")
                        visible: root.screen.arenaPanelVisibilitySupported

                        onToggled: {
                            root.screen.setArenaPanelVisible(checked);
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.leftMargin: 12
                        Layout.rightMargin: 12
                        Layout.preferredHeight: 1
                        color: "#3d464f"
                        visible: root.screen.arenaPanelVisibilitySupported
                    }

                    Label {
                        Layout.fillWidth: true
                        Layout.margins: 16
                        color: "#aeb8c2"
                        horizontalAlignment: Text.AlignHCenter
                        text: qsTr("No options are exposed by this skin.")
                        visible: root.screen.legacySkinCustomizeItems.length === 0
                        wrapMode: Text.Wrap
                    }

                    Repeater {
                        model: root.screen.legacySkinCustomizeItems

                        delegate: ColumnLayout {
                            id: optionRow

                            required property var modelData

                            Layout.fillWidth: true
                            Layout.leftMargin: 12
                            Layout.rightMargin: 12
                            Layout.topMargin: 10
                            Layout.bottomMargin: 10
                            spacing: 6

                            Label {
                                Layout.fillWidth: true
                                color: "#edf1f5"
                                elide: Text.ElideRight
                                font.bold: true
                                font.pixelSize: 14
                                text: optionRow.modelData.name
                            }

                            ComboBox {
                                Layout.fillWidth: true
                                currentIndex: root.choiceIndex(
                                                  optionRow.modelData)
                                model: optionRow.modelData.labels

                                onActivated: index => {
                                    if (index >= 0
                                            && index
                                            < optionRow.modelData.choices.length) {
                                        root.screen.setLegacySkinCustomizeValue(
                                                    optionRow.modelData,
                                                    optionRow.modelData.choices[
                                                        index]);
                                    }
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.topMargin: 4
                                Layout.preferredHeight: 1
                                color: "#343c44"
                            }
                        }
                    }
                }
            }
        }
    }
}
