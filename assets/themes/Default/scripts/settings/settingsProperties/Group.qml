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
            text: groupFrame.description
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
                        props.default_ = props.default;
                    }
                    delete props.default;
                    if (props.type === "choice") {
                        props.displayStrings = loader.displayStrings;
                    }
                    delete props.type;
                    props.destination = groupFrame.destination;
                    props.description = loader.description;
                    props.name = loader.name;
                    setSource(Helpers.capitalizeFirstLetter(modelData.type) + ".qml", props);
                }
                readonly property string name: modelData.name[Rg.languages.getClosestLanguage(Rg.languages.selectedLanguage, Object.keys(modelData.name))] || "";
                onNameChanged: {
                    if (loader.item) {
                        loader.item.name = name;
                    }
                }
                readonly property string description: modelData.description ? modelData.description[Rg.languages.getClosestLanguage(Rg.languages.selectedLanguage, Object.keys(modelData.description))] || "" : "";
                onDescriptionChanged: {
                    if (loader.item) {
                        loader.item.description = description;
                    }
                }
                readonly property list<string> displayStrings: modelData.type === "choice" ? modelData.displayStrings[Rg.languages.getClosestLanguage(Rg.languages.selectedLanguage, Object.keys(modelData.displayStrings))] : [];
                onDisplayStringsChanged: {
                    if (loader.item) {
                        loader.item.displayStrings = displayStrings;
                    }
                }
                Layout.fillWidth: true
                Layout.maximumWidth: modelData.type === "group" ? -1 : 600
                Layout.minimumWidth: modelData.type === "group" ? -1 : 150
                width: parent.width
            }
        }
    }
}
