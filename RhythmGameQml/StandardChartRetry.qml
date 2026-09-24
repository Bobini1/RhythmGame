import QtQuick
import RhythmGameQml

/*!
    \qmltype StandardChartRetry
    \inqmlmodule RhythmGameQml
    \brief Restarts a supported play without choosing an input gesture.

    For a gameplay button, supply \l gameplay. For a normal result button, supply
    \l result. StandardGameplayInput and
    StandardResultInput already use this component internally.

    \l retry replaces gameplay and any result above it without opening selection
    or decide. The game destroys the old play with the old screen. An unfinished
    attempt is not saved, and a result that was already saved is kept.

    Standard retry is available for manual single-chart play. It excludes courses,
    autoplay, replay, battle and Arena. Check \l available before offering a custom
    retry button. A course summary never offers single-chart retry.
*/
Item {
    id: root

    /*! Supplies the result context. Course summaries do not support chart retry. */
    property QtObject result: null
    /*! Supplies the gameplay context when retry is offered during play. */
    property GameplayContext gameplay: null
    /*! Reports whether the current screen and runner support standard retry. */
    readonly property bool available: !!retryState.context?._screen
        && !retryState.context.isArena
        && !retryState.context.isCourse
        && (root.result
            ? globalRoot.currentScreen?.result === root.result
                && retryState.context.status === ChartRunner.Finished
                && retryState.normalResult.chartData === retryState.context.chartData
            : globalRoot.currentScreen === retryState.context._screen)
        && Rg.chartLoader.canRetry(retryState.context._runner)

    QtObject {
        id: retryState
        readonly property ResultContext normalResult: root.result as ResultContext
        readonly property GameplayContext context: root.result
            ? retryState.normalResult?.gameplay || null : root.gameplay
    }

    /*!
        Retries with the same pattern when \a samePattern is true, or fresh randomization
        otherwise. Returns false when disabled or unsupported. A supported request is consumed
        even if loading fails; in that case the current screen remains. No call to finish() is
        made.
    */
    function retry(samePattern) {
        if (!root.enabled || !root.available) {
            return false;
        }
        const screen = retryState.context._screen;
        const runner = Rg.chartLoader.retryChart(retryState.context._runner, samePattern);
        if (runner) {
            const gameplay = ScreenContexts.createGameplay(runner);
            if (!globalRoot.replaceGameplay(gameplay, screen)) {
                runner.destroy();
            }
        }
        return true;
    }
}
