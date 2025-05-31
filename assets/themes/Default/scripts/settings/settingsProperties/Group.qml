import QtQuick.Dialogs
import QtQuick.Controls.Basic
import QtQuick.Layouts
import RhythmGameQml
import QtQuick
import "../../common/helpers.js" as Helpers

Frame {
    id: groupFrame

    property string name
    property string description
    property var items
    property var destination

    Column {
        width: parent.width
        spacing: 5
        TextEdit {
            wrapMode: TextEdit.Wrap
            anchors {
                left: parent.left
                right: parent.right
            }
            text: groupFrame.name
            font.pixelSize: 24
            font.bold: true
            readOnly: true
        }
        TextEdit {
            wrapMode: TextEdit.Wrap
            anchors {
                left: parent.left
                right: parent.right
            }
            text: groupFrame.description || ""
            font.pixelSize: 16
            readOnly: true
        }
        // empty space separator
        Rectangle {
            anchors {
                left: parent.left
                right: parent.right
            }
            Layout.preferredHeight: 10
            color: "transparent"
        }
        Repeater {
            model: groupFrame.items
            RowLayout {
                anchors {
                    left: parent.left
                    right: parent.right
                }

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
                            HoverHandler {
                                id: hoverHandler
                            }
                            ToolTip.visible: hoverHandler.hovered && (modelData.description || false)
                            ToolTip.text: modelData.description || ""
                        }
                    }
                }
                Loader {
                    id: loader

                    Component.onCompleted: {
                        let props = {};
                        Object.assign(props, modelData);
                        if ("id" in props) {
                            props.id_ = props.id;
                        }
                        delete props.id;
                        if (props.type !== "group") {
                            delete props.name;
                            delete props.description;
                        }
                        if (props.type === "range") {
                            props.default_ = props.default;
                        }
                        delete props.default;
                        delete props.type;
                        props.destination = groupFrame.destination;
                        setSource(Helpers.capitalizeFirstLetter(modelData.type) + ".qml", props);
                    }
                    Layout.fillWidth: true
                    Layout.maximumWidth: modelData.type === "group" ? -1 : 600
                    Layout.minimumWidth: modelData.type === "group" ? -1 : 150
                    // Priority
                    Layout.preferredWidth: 1
                }
                Loader {
                    active: modelData.type !== "group"
                    Layout.fillWidth: active
                    Layout.minimumWidth: active ? 50 : -1
                    Layout.maximumWidth: active ? 50 : -1
                    sourceComponent: Component {
                        Button {
                            text: "Reset"
                            enabled: groupFrame.destination[modelData.id] !== modelData.default

                            implicitWidth: 50
                            onClicked: {
                                groupFrame.destination[modelData.id] = modelData.default
                            }
                        }
                    }
                }
            }
        }
    }
}
