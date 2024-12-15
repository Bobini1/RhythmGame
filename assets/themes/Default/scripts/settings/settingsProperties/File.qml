import Qt.labs.folderlistmodel
import QtQuick
import QtQuick.Controls.Basic
import RhythmGameQml

ComboBox {
    id: fileComboBox
    property var files: FileQuery.getSelectableFilesForDirectory(Themes.availableThemeFamilies[ProfileList.mainProfile.themeConfig[screen]].path + "/" + props.path)
    model: files

    palette {
        window: "white"
        light: palette.button
    }

    function getIndex(text) {
        let index = 0;
        for (let file of files) {
            if (text === file) {
                return index;
                break;
            }
            index++;
        }
    }

    currentIndex: getIndex(destination[props.id]);
    onActivated: (_) => {
        destination[props.id] = currentText;
    }
}