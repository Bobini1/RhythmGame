import Qt.labs.folderlistmodel
import QtQuick
import QtQuick.Controls.Basic
import RhythmGameQml

ComboBox {
    id: fileComboBox
    property var files: FileQuery.getSelectableFilesForDirectory(Themes.availableThemeFamilies[ProfileList.currentProfile.themeConfig[screen]].path + "/" + props.path)
    model: files
    delegate: ItemDelegate {
        text: modelData
        width: parent.width
    }

    Component.onCompleted: {
        let index = 0;
        for (let file of files) {
            if (destination[props.id] === file) {
                fileComboBox.currentIndex = index;
                break;
            }
            index++;
        }
    }
    onActivated: (_) => {
        destination[props.id] = currentText;
        currentText = Qt.binding(() => destination[props.id]);
    }
}