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

    Flickable {
        id: scrollArea

        anchors.centerIn: parent
        width: parent.width
        height: parent.height
        contentHeight: contentLayout.implicitHeight
        contentWidth: Math.max(684 + 405 * 2, parent.width)
        clip: true

        Connections {
            function onConfiguringChanged() {
                if (!Rg.inputTranslator.configuring)
                    pressButton(null);

            }

            target: Rg.inputTranslator
        }

        Loader {
            id: analogAxisSettings1
            anchors.left: parent.left
            anchors.right: contentLayout.left
            anchors.margins: 10
            active: Rg.inputTranslator.analogAxisConfig1 !== null
            sourceComponent: AnalogAxisSettings {
                player: 1
            }
        }

        Loader {
            id: analogAxisSettings2
            anchors.left: contentLayout.right
            anchors.right: parent.right
            anchors.margins: 10
            active: Rg.inputTranslator.analogAxisConfig2 !== null
            sourceComponent: AnalogAxisSettings {
                player: 2
            }
        }

        ColumnLayout {
            id: contentLayout
            width: parent.width / 3
            anchors.horizontalCenter: parent.horizontalCenter
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
        component ButtonGroup: GroupBox {
            id: buttonGroup
            property alias model: keyRepeater.model
            readonly property var names: ["Key 1", "Key 2", "Key 3", "Key 4", "Key 5", "Key 6", "Key 7", "Scratch Up", "Scratch Down", "Start", "Select"]

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
                            text: buttonGroup.names[index]
                            color: "black"
                            horizontalAlignment: Text.AlignRight
                        }

                        Label {
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignRight
                            color: "black"
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
                                                deviceName += " axis";
                                            }
                                        }
                                        let keyName = k.code;
                                        if (deviceName === "Keyboard") {
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
                            color: Rg.inputTranslator[modelData] ? "green" : "red"
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


        ScrollIndicator.vertical: ScrollIndicator {
        }
    }
}