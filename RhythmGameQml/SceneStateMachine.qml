import QtQuick
import QtQml.StateMachine
import RhythmGameQml

StateMachine {
    initialState: mainScene
    running: true

    State {
        id: mainScene

        onEntered: sceneLoader.source = SceneUrls.mainSceneUrl
    }
    State {
        id: songWheelScene

        onEntered: sceneLoader.source = SceneUrls.songWheelSceneUrl
    }
    State {
        id: gameplayScene

        onEntered: sceneLoader.source = SceneUrls.gameplaySceneUrl
    }
}