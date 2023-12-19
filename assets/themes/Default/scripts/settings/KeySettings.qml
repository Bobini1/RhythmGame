import QtQuick 2.5
import QtQuick.Controls.Basic
import QtQuick.Layouts 1.2
import QtQuick.Window 2.0
import QtGamepadLegacy
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
                        anchors.fill: parent

                        Repeater {
                            model: ["Col11", "Col12", "Col13", "Col14", "Col15", "Col16", "Col17", "Col1sUp", "Col21", "Col22", "Col23", "Col24", "Col25", "Col26", "Col27", "Col2sUp", "Start", "Select", "Col1sDown", "Col2sDown"]

                            RowLayout {
                                Layout.fillWidth: true

                                Label {
                                    text: modelData
                                    horizontalAlignment: Text.AlignRight
                                }

                                Label {
                                    text: InputTranslator[modelData] ? qsTr("DOWN") : qsTr("UP")
                                    Layout.fillWidth: true
                                    horizontalAlignment: Text.AlignRight
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