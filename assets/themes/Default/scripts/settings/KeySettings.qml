import QtQuick 2.5
import QtQuick.Controls.Basic
import QtQuick.Layouts 1.2
import QtQuick.Window 2.0
import RhythmGameQml

Item {
    property Button checkedButton: null

    function pressButton(button) {
        if (checkedButton !== null && button !== checkedButton)
            checkedButton.checked = false;

        checkedButton = button;
    }

    Connections {
        function onConfiguringChanged() {
            if (!InputTranslator.configuring)
                pressButton(null);

        }

        target: InputTranslator
    }

    Flickable {
        id: scrollArea

        anchors.top: parent.top
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: 8
        contentHeight: contentLayout.implicitHeight
        clip: true

        ColumnLayout {
            id: contentLayout

            width: parent.width

            ColumnLayout {
                GroupBox {
                    title: qsTr("Configure Gamepad Buttons")
                    Layout.fillWidth: true

                    ColumnLayout {
                        id: keyLayout

                        property var keyConfig: InputTranslator.keyConfig

                        anchors.fill: parent

                        Repeater {
                            model: ["col11", "col12", "col13", "col14", "col15", "col16", "col17", "col1sUp", "col21", "col22", "col23", "col24", "col25", "col26", "col27", "col2sUp", "start", "select", "col1sDown", "col2sDown"]

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
                                    text: InputTranslator[modelData] ? qsTr("DOWN") : qsTr("UP")
                                    horizontalAlignment: Text.AlignRight
                                    color: InputTranslator[modelData] ? "green" : "red"
                                }

                                Button {
                                    text: qsTr("Configure")
                                    checkable: true
                                    enabled: !checked
                                    onCheckedChanged: {
                                        pressButton(this);
                                        if (checked)
                                            InputTranslator.configuredButton = index;

                                    }
                                }

                                Button {
                                    text: qsTr("Reset")
                                    onClicked: {
                                        InputTranslator.resetButton(index);
                                    }
                                }

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