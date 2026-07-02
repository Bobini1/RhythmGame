pragma ValueTypeBehavior: Addressable
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import RhythmGameQml
import "../common/helpers.js" as Helpers

Item {
    id: keySettings

    property Button checkedButton: null
    readonly property bool hasAnalogAxis1: Rg.inputTranslator.analogAxisConfig1 !== null
    readonly property bool hasAnalogAxis2: Rg.inputTranslator.analogAxisConfig2 !== null

    function pressButton(button) {
        if (checkedButton !== null && button !== checkedButton) {
            checkedButton.checked = false;
        }
        checkedButton = button;
    }

    SettingsPageScaffold {
        id: pageScaffold

        anchors.fill: parent
        SettingsPageHeader {
            title: qsTr("Key config")
            subtitle: qsTr("Configure keyboard and controller bindings for each player.")
            Layout.fillWidth: true
        }

        GridLayout {
            Layout.fillWidth: true
            columns: pageScaffold.availableWidth >= 1100 ? 2 : 1
            columnSpacing: 14
            rowSpacing: 14

            ColumnLayout {
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                Layout.alignment: Qt.AlignTop
                spacing: 14

                ButtonGroup {
                    title: qsTr("Player 1")
                    model: ["col11", "col12", "col13", "col14", "col15", "col16", "col17", "col1sUp", "col1sDown", "start1", "select1"]
                    Layout.fillWidth: true
                }

                Loader {
                    active: keySettings.hasAnalogAxis1
                    visible: active
                    sourceComponent: AnalogAxisSettings {
                        player: 1
                    }
                    Layout.fillWidth: true
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.preferredWidth: 1
                Layout.alignment: Qt.AlignTop
                spacing: 14

                ButtonGroup {
                    title: qsTr("Player 2")
                    model: ["col21", "col22", "col23", "col24", "col25", "col26", "col27", "col2sUp", "col2sDown", "start2", "select2"]
                    Layout.fillWidth: true
                }

                Loader {
                    active: keySettings.hasAnalogAxis2
                    visible: active
                    sourceComponent: AnalogAxisSettings {
                        player: 2
                    }
                    Layout.fillWidth: true
                }
            }
        }
    }

    Connections {
        target: Rg.inputTranslator

        function onConfiguringChanged() {
            if (!Rg.inputTranslator.configuring) {
                pressButton(null);
            }
        }
    }

    component ButtonGroup: WorkbenchPanel {
        id: buttonGroup

        property alias model: keyRepeater.model
        readonly property int stateColumnWidth: 86
        readonly property var names: [QT_TR_NOOP("Key 1"), QT_TR_NOOP("Key 2"), QT_TR_NOOP("Key 3"),
            QT_TR_NOOP("Key 4"), QT_TR_NOOP("Key 5"), QT_TR_NOOP("Key 6"), QT_TR_NOOP("Key 7"),
            QT_TR_NOOP("Scratch Up"), QT_TR_NOOP("Scratch Down"), QT_TR_NOOP("Start"), QT_TR_NOOP("Select")]

        ColumnLayout {
            id: keyLayout

            property var keyConfig: Rg.inputTranslator.keyConfig

            Layout.fillWidth: true
            spacing: 4

            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 4
                Layout.rightMargin: 4
                spacing: 8

                Label {
                    text: qsTr("Action")
                    color: palette.placeholderText
                    font.pixelSize: 12
                    Layout.preferredWidth: 120
                }

                Label {
                    text: qsTr("Binding")
                    color: palette.placeholderText
                    font.pixelSize: 12
                    horizontalAlignment: Text.AlignRight
                    Layout.fillWidth: true
                }

                Label {
                    text: qsTr("State")
                    color: palette.placeholderText
                    font.pixelSize: 12
                    horizontalAlignment: Text.AlignHCenter
                    Layout.minimumWidth: buttonGroup.stateColumnWidth
                    Layout.preferredWidth: buttonGroup.stateColumnWidth
                }

                Item {
                    Layout.preferredWidth: 202
                }
            }

            Repeater {
                id: keyRepeater

                RowLayout {
                    id: buttonRow

                    Layout.fillWidth: true
                    Layout.minimumHeight: 36
                    spacing: 8
                    readonly property var button: BmsKey[Helpers.capitalizeFirstLetter(modelData)]

                    Label {
                        text: qsTr(buttonGroup.names[index])
                        color: palette.text
                        elide: Text.ElideRight
                        maximumLineCount: 1
                        Layout.preferredWidth: 120
                    }

                    Label {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignRight
                        color: palette.text
                        elide: Text.ElideMiddle
                        maximumLineCount: 1
                        clip: true
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

                    StatusChip {
                        property bool active: Rg.inputTranslator[modelData]

                        text: active ? qsTr("DOWN") : qsTr("UP")
                        tone: active ? StatusChip.Accent : StatusChip.Neutral
                        Layout.minimumWidth: buttonGroup.stateColumnWidth
                        Layout.preferredWidth: Math.max(buttonGroup.stateColumnWidth, implicitWidth)
                    }

                    ActionButton {
                        text: checked ? qsTr("Listening") : qsTr("Configure")
                        tone: checked ? ActionButton.Primary : ActionButton.Secondary
                        checkable: true
                        enabled: !checked
                        Layout.preferredWidth: 96

                        onCheckedChanged: {
                            if (checked) {
                                pressButton(this);
                                Rg.inputTranslator.configuredButton = buttonRow.button;
                            }
                        }
                    }

                    ActionButton {
                        text: qsTr("Reset")
                        tone: ActionButton.Tertiary
                        Layout.preferredWidth: 84

                        onClicked: {
                            Rg.inputTranslator.resetButton(buttonRow.button);
                        }
                    }
                }
            }
        }
    }
}
