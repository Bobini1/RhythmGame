import QtQuick
import RhythmGameQml
import QtQuick.Layouts
import QtQuick.Controls.Basic

Pane {
    id: root

    readonly property bool active: StackView.status === StackView.Active
    readonly property string imagesUrl: rootUrl + "images/"
    readonly property string iniImagesUrl: "image://ini/" + rootUrl + "images/"
    property string rootUrl: globalRoot.urlToPath(Qt.resolvedUrl(".").toString())

    RowLayout {
        anchors.fill: parent

        Image {
            Layout.alignment: Qt.AlignLeft
            Layout.preferredWidth: 512
            Layout.preferredHeight: 512
            sourceSize.width: 512
            sourceSize.height: 512
            asynchronous: true
            source: {
                let currentItem = songList.model.at(songList.currentIndex);
                if (!(currentItem instanceof ChartData) || currentItem.stageFile === "") {
                    return "";
                }
                return "file://" + currentItem.directory + currentItem.stageFile;
            }

            onStatusChanged: {
                if (status === Image.Error) {
                    let currentItem = songList.model.at(songList.currentIndex);
                    console.warn("Could not load stagefile for " + currentItem.path + ":", currentItem.stageFile);
                }
            }
        }
        List {
            id: songList

            Layout.alignment: Qt.AlignRight
            Layout.fillHeight: true
            Layout.preferredWidth: parent.width / 2
        }
    }
    Shortcut {
        enabled: active
        sequence: "Esc"

        onActivated: {
            sceneStack.pop();
        }
    }
}