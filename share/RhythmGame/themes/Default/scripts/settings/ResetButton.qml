import QtQuick.Layouts

ActionButton {
    id: resetButton

    text: qsTr("Reset")
    tone: ActionButton.Tertiary

    property var destination
    property string id_
    property var default_

    enabled: destination[id_] != default_
    Layout.alignment: Qt.AlignRight | Qt.AlignVCenter

    onClicked: {
        destination[id_] = default_;
    }
}
