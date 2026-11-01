import QtQml
import RhythmGameQml

/*!
    \qmltype StandardGameplayViewport
    \inqmlmodule RhythmGameQml
    \brief Applies the player's note speed and keeps visible note models in sync.

    Bind \l player and \l chartData to the current stage, and set \l noteHeight
    to the distance from the top clipping edge to the judgement line.
    Use \l pixelsPerBeat to place notes relative
    to player.position. The component updates the player's filtered note and
    bar-line models when the dimensions, profile settings or stage change.

    Speed follows the profile's noteScreenTimeMillis and HiSpeedFix setting.
    Off uses 120 BPM. Main, Start, Min, Max and Avg use the corresponding chart
    BPM. BPM and scroll changes within the chart still affect note movement.

    A field with lane cover or lift should set \l visibleHeight to the distance
    from the lane cover to the judgement line. Hidden cover does not reduce
    this distance for speed calculation. \l barLineHeight can describe a
    separate clipped area for bar lines. All heights use the renderer's local
    coordinates. The component draws nothing and does not occupy layout space.

    It owns the calls to GameplayState::setVisiblePositionSpans() for the
    player. Fields sharing a player, such as two sides in double play, share
    the filtered models and must use the same visible position spans.
*/
QtObject {
    id: root

    /*! Player whose settings and filtered note models are used. */
    required property Player player
    /*! Metadata for the chart currently playing, including a course stage. */
    required property ChartData chartData
    /*! Distance from the top note clipping edge to the judgement line. */
    required property real noteHeight
    /*! Note travel distance after lane cover and lift. Defaults to noteHeight. */
    property real visibleHeight: noteHeight
    /*! Height used to draw bar lines. Defaults to noteHeight. */
    property real barLineHeight: noteHeight
    /*! Distance for one unit of notePosition or player.position. Zero for an empty field. */
    readonly property real pixelsPerBeat: state.visibleSpan > 0
        ? Math.max(0, visibleHeight) / state.visibleSpan
        : 0

    readonly property QtObject implementation: QtObject {
        id: state
        readonly property Player player: root.player
        readonly property var vars: player?.profile.vars.generalVars
        readonly property real bpm: {
            const chart = root.chartData;
            if (!chart || !vars) return 120;
            let value = 120;
            switch (vars.hiSpeedFix) {
            case HiSpeedFix.Main: value = chart.mainBpm; break;
            case HiSpeedFix.Start: value = chart.initialBpm; break;
            case HiSpeedFix.Min: value = chart.minBpm; break;
            case HiSpeedFix.Max: value = chart.maxBpm; break;
            case HiSpeedFix.Avg: value = chart.avgBpm; break;
            }
            return value > 0 && isFinite(value) ? value : 120;
        }
        // Seconds on screen multiplied by beats per second.
        readonly property real visibleSpan: root.chartData && vars
            ? (vars.noteScreenTimeMillis / 1000) * (bpm / 60) : 0
        readonly property real noteSpan: root.pixelsPerBeat > 0
            ? Math.max(0, root.noteHeight) / root.pixelsPerBeat : 0
        readonly property real barLineSpan: root.pixelsPerBeat > 0
            ? Math.max(0, root.barLineHeight) / root.pixelsPerBeat : 0
        property bool ready: false

        function updateVisibleRange() {
            if (ready && player)
                player.state.setVisiblePositionSpans(noteSpan, barLineSpan);
        }

        onPlayerChanged: updateVisibleRange()
        onNoteSpanChanged: updateVisibleRange()
        onBarLineSpanChanged: updateVisibleRange()
    }

    Component.onCompleted: {
        state.ready = true;
        state.updateVisibleRange();
    }
}
