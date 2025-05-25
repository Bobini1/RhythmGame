import QtQuick 2.5
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Window 2.0
import RhythmGameQml

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
        contentWidth: parent.width
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

        GroupBox {
            id: contentLayout
            title: qsTr("Configure Gamepad Buttons")
            width: parent.width / 3
            anchors.horizontalCenter: parent.horizontalCenter

            ColumnLayout {
                id: keyLayout

                property var keyConfig: Rg.inputTranslator.keyConfig

                anchors.fill: parent

                Repeater {
                    model: ["col11", "col12", "col13", "col14", "col15", "col16", "col17", "col1sUp", "col21", "col22", "col23", "col24", "col25", "col26", "col27", "col2sUp", "start1", "select1", "col1sDown", "start2", "select2", "col2sDown"]

                    RowLayout {
                        Layout.fillWidth: true

                        Label {
                            text: modelData
                            color: "black"
                            horizontalAlignment: Text.AlignRight
                        }

                        Label {
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignRight
                            color: "black"
                            text: {
                                for (let i = 0; i < keyLayout.keyConfig.length; i++) {
                                    if (keyLayout.keyConfig[i].button === index) {
                                        let key = keyLayout.keyConfig[i].key;
                                        let deviceName = "Keyboard";
                                        if (key.gamepad) {
                                            deviceName = key.gamepad.name;
                                            if (key.gamepad.index !== 0) {
                                                deviceName += " (" + key.gamepad.index + ")";
                                            }
                                            if (key.device === Key.Axis) {
                                                deviceName += " axis";
                                            }
                                        }
                                        return key.code + " (" + deviceName + ")";
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
                                    Rg.inputTranslator.configuredButton = index;

                            }
                        }

                        Button {
                            text: qsTr("Reset")
                            onClicked: {
                                Rg.inputTranslator.resetButton(index);
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