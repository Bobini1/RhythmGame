import Qt.labs.folderlistmodel
import QtQuick
import QtQuick.Controls.Basic
import RhythmGameQml
import "../../common/helpers.js" as Helpers

ComboBox {
    id: fileComboBox
    property var files: Rg.fileQuery.getSelectableFilesForDirectory(Rg.themes.availableThemeFamilies[Rg.profileList.mainProfile.themeConfig[screen]].path + "/" + path)
    model: files
    property var destination
    property string id_
    property string path

    palette {
        window: "white"
        Component.onCompleted: {
            light = palette.button
        }
    }

    Binding {
        delayed: true
        fileComboBox.currentIndex: Helpers.getIndex(fileComboBox.files, destination[fileComboBox.id_], currentIndex);
    }

    onActivated: (_) => {
        destination[fileComboBox.id_] = currentText;
    }
}