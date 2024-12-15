import RhythmGameQml
import QtQuick.Layouts
import QtQuick
import QtQuick.Controls.Basic

RowLayout {
    required property var props
    property alias sourceComponent: loader.sourceComponent
    property alias text: text.text
    anchors {
        left: parent ? parent.left : undefined
        right: parent ? parent.right : undefined
    }

    TextEdit {
        id: text
        font.pixelSize: 16
        font.bold: true
        readOnly: true
        text: "Green Number"
        Layout.alignment: Qt.AlignVCenter
        Layout.preferredWidth: 200
        wrapMode: TextEdit.Wrap
    }
    Loader {
        id: loader
        active: true
        Layout.fillWidth: true
        property var props: parent.props
        property var destination: ProfileList.mainProfile.vars.globalVars
        Layout.alignment: Qt.AlignVCenter
    }
    Button {
        text: "Reset"

        implicitWidth: 50
        enabled: ProfileList.mainProfile.vars.globalVars[parent.props.id] !== parent.props.default
        Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
        onClicked: {
            ProfileList.mainProfile.vars.globalVars[parent.props.id] = parent.props.default
        }
    }
}