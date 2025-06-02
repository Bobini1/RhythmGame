import Qt.labs.folderlistmodel
import QtQuick
import QtQuick.Controls.Basic
import RhythmGameQml
import QtQuick.Layouts
import "../../common/helpers.js" as Helpers
import ".."

RowLayout {
    id: file
    property var destination
    property string id_
    property var files
    property alias name: strLabel.text
    property alias description: strLabel.description
    property var default_
    property string path

    SettingsLabel {
        id: strLabel
    }


    ComboBox {
        id: fileComboBox
        property var files: Rg.fileQuery.getSelectableFilesForDirectory(Rg.themes.availableThemeFamilies[Rg.profileList.mainProfile.themeConfig[screen]].path + "/" + path)
        model: files
        Layout.fillWidth: true
        Layout.preferredWidth: 400
        Layout.minimumWidth: 200

        palette {
            window: "white"
            Component.onCompleted: {
                light = palette.button
            }
        }

        Binding {
            delayed: true
            fileComboBox.currentIndex: Helpers.getIndex(fileComboBox.files, destination[file.id_], fileComboBox.currentIndex);
        }

        onActivated: (_) => {
            destination[fileComboBox.id_] = currentText;
        }
    }


    ResetButton {
        destination: file.destination
        id_: file.id_
        default_: file.default_

        onClicked: {
            file.destination[file.id_] = file.default_
        }
    }
}
