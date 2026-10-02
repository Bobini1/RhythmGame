import QtQuick
import RhythmGameQml

/*!
    \qmltype StandardSelectPreview
    \inqmlmodule RhythmGameQml
    \brief Plays the focused chart's preview after a short delay.

    StandardSelectController includes this component as its preview property
    and coordinates it with background music. Customize that property instead
    of adding a second preview player.

    For a custom selector, set \l selectState to a StandardSelectState.
    The component uses its focused chart and discovered preview file. Without
    a preview file, it plays the chart's audio through ChartAudioPlayer, which
    does not load BGA. Set \l chartAudioEnabled to false to omit that fallback.
    For custom browsing, supply \l chartData and \l previewSource directly.

    Bind \l moving to your list's movement state to wait until scrolling stops.
    Changing the source cancels the previous load and starts the delay again.
    Disabling the component stops audio and clears its sources. Bind a theme's
    background music to \l playing if it should stop during previews.
*/
Item {
    id: root

    /*! Supplies the current chart and preview paths. Optional for custom browsing. */
    property StandardSelectState selectState: null
    /*! Chart to preview. Follows the selection unless supplied directly. */
    property ChartData chartData: selectState?.focusedItem instanceof ChartData
        ? selectState.focusedItem : null
    /*! Preview file or song-asset source. Empty enables the chart-audio fallback. */
    property string previewSource: selectState && chartData
        ? selectState.previewFiles[chartData.chartDirectory] || "" : ""
    /*! Allows audio-only chart playback when no preview file is present. */
    property bool chartAudioEnabled: true
    /*! Delays a pending preview while the theme's list is moving. */
    property bool moving: false
    /*! Delay after the source changes or scrolling stops, in milliseconds. */
    property int delayMillis: 300
    /*! Fade-in duration for either kind of preview, in milliseconds. */
    property int fadeInMillis: 1000
    /*! Playback volume, from zero to one. */
    property real volume: 1
    /*! Whether loaded preview audio is currently playing. */
    readonly property bool playing: (filePlayer.loaded && filePlayer.playing)
        || (chartPlayer.loaded && chartPlayer.playing)

    enabled: selectState ? selectState.enabled : true

    QtObject {
        id: state
        readonly property bool chartAudio: !root.previewSource && root.chartAudioEnabled
        readonly property string source: root.enabled
            ? root.previewSource || (chartAudio ? root.chartData?.path || "" : "") : ""
        property string delayedSource: ""
        readonly property bool ready: source !== "" && delayedSource === source

        function restartDelay() {
            delayedSource = "";
            if (delay && source !== "" && !root.moving) delay.restart();
        }

        onSourceChanged: restartDelay()
        onChartAudioChanged: restartDelay()
    }

    Timer {
        id: delay
        interval: Math.max(1, root.delayMillis)
        running: state.source !== "" && !state.ready && !root.moving
        onTriggered: state.delayedSource = state.source
    }

    AudioPlayer {
        id: filePlayer
        source: state.ready && !state.chartAudio ? state.source : ""
        playing: root.enabled && filePlayer.source !== "" && filePlayer.loaded
        looping: true
        fadeInMillis: root.fadeInMillis
        volume: root.volume
    }

    ChartAudioPlayer {
        id: chartPlayer
        source: state.ready && state.chartAudio ? state.source : ""
        playing: root.enabled && chartPlayer.source !== "" && chartPlayer.loaded
        looping: true
        fadeInMillis: root.fadeInMillis
        volume: root.volume
    }
}
