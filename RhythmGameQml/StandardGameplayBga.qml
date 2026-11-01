import QtQuick
import QtMultimedia
import RhythmGameQml

/*!
    \qmltype StandardGameplayBga
    \inqmlmodule RhythmGameQml
    \brief Draws the current stage's background animation and miss layer.

    Bind \l gameplay and give the item a size. It connects the four BGA layers
    to video outputs and follows course stage changes. The two overlay layers
    treat black as transparent. A Bad or Poor judgement shows the miss layer
    for 400 milliseconds. Image proportions and relative layer sizes are kept.

    Visibility follows the player's bgaOn setting, or the main profile's setting
    in battle. Override visible to choose differently. This item is optional;
    themes can instead connect gameplay.bga.layers to their own video sinks.
*/
Item {
    id: root

    /*! Supplies the current BGA layers and the players whose misses are shown. */
    property GameplayContext gameplay: null

    visible: !!bgaState.profile && bgaState.profile.vars.generalVars.bgaOn

    QtObject {
        id: bgaState
        readonly property Profile profile: root.gameplay?.players.length > 1
            ? Rg.profileList.mainProfile : (root.gameplay?.players[0]?.profile || null)
        readonly property var bga: root.gameplay?.bga || null
        onBgaChanged: poorTimer.stop()
    }

    Timer {
        id: poorTimer
        interval: 400
    }
    onEnabledChanged: if (!enabled) poorTimer.stop()

    Repeater {
        model: root.gameplay?.players || []
        Item {
            id: participant
            required property var modelData
            Connections {
                target: participant.modelData.score
                enabled: root.enabled
                function onHit(hit) {
                    if (hit.judgement === Judgement.Bad || hit.judgement === Judgement.Poor)
                        poorTimer.restart();
                }
            }
        }
    }

    Item {
        anchors.centerIn: parent
        width: Math.max(videoBase.sourceRect.width, videoLayer.sourceRect.width, videoLayer2.sourceRect.width, videoPoor.sourceRect.width, 256)
        height: Math.max(videoBase.sourceRect.height, videoLayer.sourceRect.height, videoLayer2.sourceRect.height, videoPoor.sourceRect.height, 256)
        scale: Math.min(root.width / width, root.height / height)

        VideoOutput {
            id: videoBase
            anchors.centerIn: parent
            width: sourceRect.width || 256
            height: sourceRect.height || 256
            Binding {
                target: bgaState.bga?.layers[0] || null
                property: "videoSink"
                value: videoBase.videoSink
            }
        }
        VideoOutput {
            id: videoLayer
            anchors.centerIn: parent
            width: sourceRect.width || 256
            height: sourceRect.height || 256
            Binding {
                target: bgaState.bga?.layers[1] || null
                property: "videoSink"
                value: videoLayer.videoSink
            }
        }
        ColorChanger {
            anchors.fill: videoLayer
            source: ShaderEffectSource {
                sourceItem: videoLayer
                hideSource: true
            }
        }
        VideoOutput {
            id: videoLayer2
            anchors.centerIn: parent
            width: sourceRect.width || 256
            height: sourceRect.height || 256
            Binding {
                target: bgaState.bga?.layers[2] || null
                property: "videoSink"
                value: videoLayer2.videoSink
            }
        }
        ColorChanger {
            anchors.fill: videoLayer2
            source: ShaderEffectSource {
                sourceItem: videoLayer2
                hideSource: true
            }
        }
        VideoOutput {
            id: videoPoor
            anchors.centerIn: parent
            width: sourceRect.width || 256
            height: sourceRect.height || 256
            visible: poorTimer.running
            Binding {
                target: bgaState.bga?.layers[3] || null
                property: "videoSink"
                value: videoPoor.videoSink
            }
        }
    }
}
