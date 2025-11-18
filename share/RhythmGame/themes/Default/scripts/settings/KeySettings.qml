pragma ValueTypeBehavior: Addressable
import QtQuick 2.5
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Window 2.0
import RhythmGameQml
import "../common/helpers.js" as Helpers

Item {
    property Button checkedButton: null

    function pressButton(button) {
        if (checkedButton !== null && button !== checkedButton)
            checkedButton.checked = false;

        checkedButton = button;
    }
    Loader {
        id: analogAxisSettings1
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignTop
        Layout.preferredWidth: implicitWidth || analogAxisSettings2.implicitWidth
        active: Rg.inputTranslator.analogAxisConfig1 !== null
        sourceComponent: AnalogAxisSettings {
            player: 1
        }
    }

    Loader {
        id: analogAxisSettings2
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignTop
        Layout.preferredWidth: implicitWidth || analogAxisSettings1.implicitWidth
        active: Rg.inputTranslator.analogAxisConfig2 !== null
        sourceComponent: AnalogAxisSettings {
            player: 2
        }
    }

    ColumnLayout {
        id: contentLayout
        Layout.fillWidth: true
        Layout.minimumWidth: 600
        Layout.maximumWidth: 800 + 405 * 2
        Layout.alignment: Qt.AlignTop | Qt.AlignHCenter
        ButtonGroup {
            title: qsTr("Player 1")
            model: ["col11", "col12", "col13", "col14", "col15", "col16", "col17", "col1sUp", "col1sDown", "start1", "select1"]
            Layout.fillWidth: true
        }
        ButtonGroup {
            title: qsTr("Player 2")
            model: ["col21", "col22", "col23", "col24", "col25", "col26", "col27", "col2sUp", "col2sDown", "start2", "select2"]
            Layout.fillWidth: true
        }
    }

    ScrollView {
        id: scrollArea

        anchors.fill: parent
        contentWidth: Math.max(600, width)
        contentHeight: {
            if (wideRow.visible) {
                return wideRow.implicitHeight;
            } else if (narrowRow.visible) {
                return narrowRow.implicitHeight;
            } else if (veryNarrowColumn.visible) {
                return veryNarrowColumn.implicitHeight;
            }
        }
        clip: true

        states: [
            State {
                name: "narrow"
                when: scrollArea.width <= 800 + 405 * 2 && scrollArea.width > 800 + 405
            },
            State {
                name: "veryNarrow"
                when: scrollArea.width <= 800 + 405
            }
        ]
        RowLayout {
            id: wideRow
            anchors.fill: parent
            anchors.margins: 5
            visible: scrollArea.state === ""
            LayoutItemProxy {
                visible: analogAxisSettings1.active || analogAxisSettings2.active
                target: analogAxisSettings1
            }
            LayoutItemProxy {
                target: contentLayout
            }
            LayoutItemProxy {
                visible: analogAxisSettings1.active || analogAxisSettings2.active
                target: analogAxisSettings2
            }
        }
        RowLayout {
            id: narrowRow
            anchors.fill: parent
            anchors.margins: 5
            visible: scrollArea.state === "narrow"
            spacing: analogAxisSettings1.active || analogAxisSettings2.active ? 5 : 0
            LayoutItemProxy {
                target: contentLayout
            }
            ColumnLayout {
                Layout.fillWidth: true
                Layout.preferredWidth: analogAxisSettings1.active || analogAxisSettings2.active ? -1 : 0
                Layout.alignment: Qt.AlignTop
                LayoutItemProxy {
                    visible: analogAxisSettings1.active
                    target: analogAxisSettings1
                }
                LayoutItemProxy {
                    visible: analogAxisSettings2.active
                    target: analogAxisSettings2
                }
            }
        }
        ColumnLayout {
            id: veryNarrowColumn
            anchors.fill: parent
            anchors.margins: 5
            visible: scrollArea.state === "veryNarrow"
            LayoutItemProxy {
                target: contentLayout
            }
            LayoutItemProxy {
                visible: analogAxisSettings1.active
                target: analogAxisSettings1
            }
            LayoutItemProxy {
                visible: analogAxisSettings2.active
                target: analogAxisSettings2
            }
        }

        Connections {
            function onConfiguringChanged() {
                if (!Rg.inputTranslator.configuring)
                    pressButton(null);

            }

            target: Rg.inputTranslator
        }

        component ButtonGroup: GroupBox {
            id: buttonGroup
            property alias model: keyRepeater.model
            readonly property var names: [QT_TR_NOOP("Key 1"), QT_TR_NOOP("Key 2"), QT_TR_NOOP("Key 3"),
                QT_TR_NOOP("Key 4"), QT_TR_NOOP("Key 5"), QT_TR_NOOP("Key 6"), QT_TR_NOOP("Key 7"),
                QT_TR_NOOP("Scratch Up"), QT_TR_NOOP("Scratch Down"), QT_TR_NOOP("Start"), QT_TR_NOOP("Select")]

            ColumnLayout {
                id: keyLayout

                property var keyConfig: Rg.inputTranslator.keyConfig

                anchors.fill: parent

                Repeater {
                    id: keyRepeater

                    RowLayout {
                        id: buttonRow
                        Layout.fillWidth: true
                        readonly property var button: BmsKey[Helpers.capitalizeFirstLetter(modelData)]

                        Label {
                            text: qsTr(buttonGroup.names[index])
                            color: palette.text
                            horizontalAlignment: Text.AlignRight
                        }

                        Label {
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignRight
                            color: palette.text
                            text: {
                                for (let i = 0; i < keyLayout.keyConfig.length; i++) {
                                    if (keyLayout.keyConfig[i].button === buttonRow.button) {
                                        let k = keyLayout.keyConfig[i].key;
                                        let deviceName = "Keyboard";
                                        if (k.gamepad) {
                                            deviceName = k.gamepad.name;
                                            if (k.gamepad.index !== 0) {
                                                deviceName += " (" + k.gamepad.index + ")";
                                            }
                                            if (k.device === key.Axis) {
                                                deviceName += qsTr(" axis");
                                                deviceName += " " + k.code;
                                                if (k.direction === key.Down) {
                                                    deviceName += qsTr(" down");
                                                } else if (k.direction === key.Up) {
                                                    deviceName += qsTr(" up");
                                                }
                                            }
                                        }
                                        let keyName = k.code;
                                        if (deviceName === "Keyboard") {
                                            deviceName = qsTr("Keyboard");
                                            keyName = Rg.inputTranslator.scancodeToString(k.code);
                                        }
                                        return keyName + " (" + deviceName + ")";
                                    }
                                }
                                return qsTr("Not Configured");
                            }
                        }

                        Label {
                            text: Rg.inputTranslator[modelData] ? qsTr("DOWN") : qsTr("UP")
                            horizontalAlignment: Text.AlignRight
                            color: palette.text
                            Layout.preferredWidth: Math.max(up.boundingRect.width, down.boundingRect.width)
                        }
                        TextMetrics {
                            id: up
                            text: qsTr("UP")
                        }
                        TextMetrics {
                            id: down
                            text: qsTr("DOWN")
                        }

                        Button {
                            text: qsTr("Configure")
                            checkable: true
                            enabled: !checked
                            onCheckedChanged: {
                                pressButton(this);
                                if (checked)
                                    Rg.inputTranslator.configuredButton = buttonRow.button;

                            }
                        }

                        Button {
                            text: qsTr("Reset")
                            onClicked: {
                                Rg.inputTranslator.resetButton(buttonRow.button);
                            }
                        }

                    }

                }

            }

        }
    }
}