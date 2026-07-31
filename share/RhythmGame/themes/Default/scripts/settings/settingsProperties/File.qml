import Qt.labs.folderlistmodel
import QtQuick
import QtQuick.Controls
import RhythmGameQml
import QtQuick.Layouts
import "../../common/helpers.js" as Helpers
import ".."

RowLayout {
    id: file
    spacing: 12
    Layout.fillWidth: true
    Layout.minimumHeight: 34

    property var destination
    property string id_
    property alias name: strLabel.text
    property alias description: strLabel.description
    property var default_
    property string path
    property var filters: []

    SettingsLabel {
        id: strLabel
    }


    ComboBox {
        id: fileComboBox
        property var files: Rg.fileQuery.getSelectableFilesForDirectory(
                                Rg.themes.availableThemeFamilies[Rg.profileList.mainProfile.themeConfig[screen]].path + "/" + path,
                                file.filters || [])
        model: files
        Layout.fillWidth: true
        Layout.preferredWidth: 460
        Layout.minimumWidth: 220

        Binding {
            delayed: true
            fileComboBox.currentIndex: Helpers.getIndex(fileComboBox.files, file.destination[file.id_], fileComboBox.currentIndex);
        }

        onActivated: (_) => {
            file.destination[file.id_] = currentText;
        }
    }


    ResetButton {
        destination: file.destination
        id_: file.id_
        default_: file.default_
        Layout.preferredWidth: 84
        Layout.minimumWidth: 76

        onClicked: {
            file.destination[file.id_] = file.default_
        }
    }
}
