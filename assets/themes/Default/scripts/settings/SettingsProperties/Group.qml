import QtQuick.Dialogs
import QtQuick.Controls.Basic
import QtQuick.Layouts
import RhythmGameQml
import QtQuick
import "../../common/helpers.js" as Helpers

Frame {
    id: groupFrame

    property var _props: props
    property var _destination: destination

    ColumnLayout {
        width: parent.width
        TextEdit {
            wrapMode: TextEdit.Wrap
            Layout.fillWidth: true
            text: groupFrame._props.name
            font.pixelSize: 24
            font.bold: true
            readOnly: true
        }
        Repeater {
            model: groupFrame._props.items
            RowLayout {
                width: parent.width
                Loader {
                    id: loader1
                    active: modelData.type !== "group"
                    Layout.fillWidth: modelData.type !== "group"
                    Layout.minimumWidth: modelData.type !== "group" ? 150 : -1
                    // I have no idea why this is needed here.
                    // It works like some kind of priority.
                    Layout.preferredWidth: modelData.type !== "group" ? 2 : -1
                    sourceComponent: Component {
                        TextEdit {
                            text: modelData.name
                            font.pixelSize: 16
                            font.bold: true
                            wrapMode: TextEdit.Wrap
                            readOnly: true
                        }
                    }
                }
                Loader {
                    id: loader

                    source: Helpers.capitalizeFirstLetter(modelData.type) + ".qml"
                    property var destination: groupFrame._destination
                    property var props: modelData
                    Layout.fillWidth: true
                    Layout.maximumWidth: modelData.type === "group" ? -1 : 300
                    Layout.minimumWidth: modelData.type === "group" ? -1 : 100
                    // Priority
                    Layout.preferredWidth: 1
                }
            }
        }
    }
}
