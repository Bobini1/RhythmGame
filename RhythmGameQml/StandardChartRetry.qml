import QtQuick
import RhythmGameQml

/*!
    \qmltype StandardChartRetry
    \inqmlmodule RhythmGameQml
    \brief Replaces local gameplay without prescribing an input gesture.

    Supply \l chart in gameplay, or set \l fromResult to infer the runner from
    the gameplay underneath results. \l retry replaces gameplay and any result
    above it, without visiting selection or decide. The old runner is destroyed
    with the old gameplay. Unfinished attempts are not saved; already saved
    results are unaffected.

    \l StandardGameplayInput and \l StandardResultInput use this internally.
    Instantiate it directly for custom retry buttons or input mappings.
*/
Item {
    id: root

    /*! Local runner; inferred from the preceding gameplay for result retry. */
    property var chart: null
    /*! Whether the current screen is a completed result. */
    property bool fromResult: false
    /*! Whether the current screen and runner support standard retry. */
    readonly property bool available: retryState.gameplay !== null
        && retryState.runner instanceof ChartRunner
        && retryState.gameplay.chart === retryState.runner
        && retryState.gameplay.arenaManagedRunner !== true
        && retryState.runner !== Rg.arenaSession.arenaRunner
        && (!root.fromResult || (retryState.runner.status === ChartRunner.Finished
            && globalRoot.currentScreen?.chartData === retryState.runner.chartData))
        && Rg.chartLoader.canRetry(retryState.runner)

    QtObject {
        id: retryState
        readonly property Item gameplay: root.fromResult
            ? globalRoot.previousScreen() : globalRoot.currentScreen
        readonly property var runner: root.chart
            || (root.fromResult && retryState.gameplay ? retryState.gameplay.chart : null)
    }

    /*!
        Retries with the same pattern when \a samePattern is true, or fresh
        randomization otherwise. Returns false when disabled or unsupported.
        A supported request is consumed even if loading fails; in that case
        the current screen remains. No call to finish() is made.
    */
    function retry(samePattern) {
        if (!root.enabled || !root.available) {
            return false;
        }
        const gameplay = retryState.gameplay;
        const runner = Rg.chartLoader.retryChart(retryState.runner, samePattern);
        if (runner && !globalRoot.replaceGameplay(runner, gameplay)) {
            runner.destroy();
        }
        return true;
    }
}
