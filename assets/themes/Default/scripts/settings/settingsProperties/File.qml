import Qt.labs.folderlistmodel
import QtQuick
import QtQuick.Controls.Basic
import RhythmGameQml
import "../../common/helpers.js" as Helpers

ComboBox {
    id: fileComboBox
    property var files: FileQuery.getSelectableFilesForDirectory(Themes.availableThemeFamilies[ProfileList.mainProfile.themeConfig[screen]].path + "/" + props.path)
    model: files

    palette {
        window: "white"
        Component.onCompleted: {
            light = palette.button
        }
    }

    Binding {
        delayed: true
        fileComboBox.currentIndex: Helpers.getIndex(files, destination[props.id], currentIndex);
    }

    onActivated: (_) => {
        destination[props.id] = currentText;
    }
}