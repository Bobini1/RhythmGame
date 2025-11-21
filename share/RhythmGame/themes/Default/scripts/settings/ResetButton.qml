import QtQuick.Controls
import QtQuick.Layouts

Button {
    id: resetButton

    text: qsTr("Reset")

    property var destination
    property string id_
    property var default_

    enabled: destination[id_] != default_
    Layout.alignment: Qt.AlignRight | Qt.AlignVCenter

    onClicked: {
        destination[id_] = default_;
    }
}