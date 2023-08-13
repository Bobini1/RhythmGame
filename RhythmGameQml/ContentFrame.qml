import QtQuick 2.0
import RhythmGameQml

Item {
    id: contentContainer

    anchors.fill: parent

    Item {
        id: root

        anchors.fill: parent

        Loader {
            id: sceneLoader

        }
        SceneStateMachine {
            id: sceneStateMachine

        }
    }
}
