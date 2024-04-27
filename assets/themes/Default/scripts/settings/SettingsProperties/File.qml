import Qt.labs.folderlistmodel 2.7
import QtQuick
import QtQuick.Controls.Basic

Item {
    id: fileItem

    required property string destination
    required property var props

    FolderListModel {
        id: folderModel

        folder: Themes.availableThemeFamilies[ProfileList.currentProfile.themeConfig[screen]].path + fileItem.props.path
        nameFilters: ["!*.ini"]
        showDirs: false
    }
    ComboBox {
        id: fileComboBox

        model: folderModel

        delegate: ItemDelegate {
            required property string fileName

            text: fileName
            width: parent.width
        }

        Component.onCompleted: {
            if (folderModel.count === 0) {
                fileComboBox.enabled = false;
                return;
            }
            for (let i = 0; i < folderModel.count; i++) {
                if (folderModel.get(i, "fileName") === fileItem.destination[props.id]) {
                    fileComboBox.currentIndex = i;
                    break;
                }
            }
        }
        onCurrentIndexChanged: {
            fileItem.destination[props.id] = fileComboBox.currentText;
        }
    }
}
