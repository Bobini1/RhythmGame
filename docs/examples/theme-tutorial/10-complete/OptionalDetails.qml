pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import RhythmGameQml

ColumnLayout {
    id: root

    required property string details

    Button {
        id: detailsButton
        checkable: true
        text: detailsButton.checked ? qsTr("Hide details") : qsTr("Show details")
    }

    // [lazy-panel]
    Loader {
        Layout.fillWidth: true
        active: root.enabled && detailsButton.checked
        sourceComponent: Label {
            text: root.details
            wrapMode: Text.Wrap
        }
    }
    // [lazy-panel]

    // [profile-connection]
    Connections {
        target: Rg.profileList
        enabled: root.enabled

        function onMainProfileChanged() {
            detailsButton.checked = false;
        }
    }
    // [profile-connection]

    // [screen-shortcut]
    Shortcut {
        sequence: "F7"
        enabled: root.enabled
        onActivated: detailsButton.toggle()
    }
    // [screen-shortcut]
}
