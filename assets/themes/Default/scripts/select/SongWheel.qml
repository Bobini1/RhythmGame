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

        StageFile {
            Layout.alignment: Qt.AlignLeft
            Layout.preferredHeight: 480
            Layout.preferredWidth: 640
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