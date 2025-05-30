import RhythmGameQml
import QtQuick.Layouts
import QtQuick
import QtQuick.Controls.Basic

RowLayout {
    required property var props
    property alias sourceComponent: loader.sourceComponent
    property alias text: text.text

    Loader {
        id: loader
        active: true
        Layout.fillWidth: true
        property var props: parent.props
        property var destination: Rg.profileList.mainProfile.vars.globalVars
        Layout.alignment: Qt.AlignVCenter
    }
    Button {
        text: "Reset"

        implicitWidth: 50
        enabled: Rg.profileList.mainProfile.vars.globalVars[parent.props.id] !== parent.props.default
        Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
        onClicked: {
            Rg.profileList.mainProfile.vars.globalVars[parent.props.id] = parent.props.default;
        }
    }
}