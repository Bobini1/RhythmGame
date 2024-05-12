import Qt.labs.folderlistmodel
import QtQuick
import QtQuick.Controls.Basic
import RhythmGameQml

ComboBox {
    id: fileComboBox
    model: ProfileList.currentProfile.vars.getSelectableFilesForDirectory(Themes.availableThemeFamilies[ProfileList.currentProfile.themeConfig[screen]].path + "/" + props.path)
    delegate: ItemDelegate {
        text: modelData
        width: parent.width
        Component.onCompleted: {
            if (destination[props.id] === modelData) {
                fileComboBox.currentIndex = index;
            }
        }
    }
    onCurrentTextChanged: {
        destination[props.id] = currentText;
    }
}