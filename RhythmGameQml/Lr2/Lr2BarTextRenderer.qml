import QtQuick
import RhythmGameQml

Item {
    id: root

    property var dsts: []
    property var srcData
    property int skinTime: 0
    property var activeOptionsState: null
    property var activeOptions: []
    property var timers: ({ 0: 0 })
    property int timerFire: -2147483648
    property real scaleOverride: 1.0
    property var selectContext
    property var barRows: []
    property var barBaseStateResolver
    property var barPositionMap
    property var barCells: []
    property bool fastBarScrollActive: false
    property real fastBarScrollX: 0
    property real fastBarScrollY: 0
    property real selectedFastBarDrawX: 0
    property real selectedFastBarDrawY: 0
    property int barCenter: 0
    x: fastBarScrollActive ? fastBarScrollX * scaleOverride : 0
    y: fastBarScrollActive ? fastBarScrollY * scaleOverride : 0
    readonly property int selectedRow: selectContext ? barCenter + selectContext.selectedOffset : barCenter
    readonly property int barCellCount: barCells ? barCells.length : 0
    readonly property var timelineDsts: {
        if (!dsts || dsts.length === 0 || !dsts[0]) {
            return dsts;
        }

        // OpenLR2's select bar title path applies the type-0/type-1 source
        // choice itself and does not gate #DST_BAR_TITLE by opt1/opt2/opt3.
        let result = dsts.slice();
        let first = {};
        for (let key in dsts[0]) {
            first[key] = dsts[0][key];
        }
        first.op1 = 0;
        first.op2 = 0;
        first.op3 = 0;
        result[0] = first;
        return result;
    }

    readonly property int textSlotCount: barRows && selectContext && srcData
        ? Math.max(0, barRows.length)
        : 0
    readonly property bool hasStaticTimelineState: timelineState.canUseStaticState
    readonly property var staticTimelineState: hasStaticTimelineState && timelineState.staticState.valid
        ? timelineState.staticState
        : null
    readonly property var timelineTimers: timelineState.usesDynamicTimer ? timers : null
    property Lr2TimelineState timelineState: Lr2TimelineState {
        enabled: !root.hasStaticTimelineState
        dsts: root.timelineDsts
        skinTime: root.skinTime
        timers: root.timelineTimers
        timerFire: root.timerFire
        activeOptionsState: root.activeOptionsState
        activeOptions: root.activeOptions
    }
    Lr2BarTextItem {
        id: nativeText

        anchors.fill: parent
        visible: supported
        dsts: root.timelineDsts
        srcData: root.srcData
        barCells: root.barCells
        barPositionMap: root.barPositionMap
        scaleOverride: root.scaleOverride
    }

    Repeater {
        id: textRepeater

        model: nativeText.supported ? 0 : root.textSlotCount

        Lr2BarPositionedItem {
            id: barTextDelegate

            readonly property var cell: root.barCells && slot >= 0 && slot < root.barCells.length
                ? root.barCells[slot]
                : null
            readonly property int sourceTitleType: root.srcData ? root.srcData.titleType : -1
            property string cellText: ""
            property bool contentVisible: false
            function resolvedCellText() : string {
                return cell ? cell.textForTitleType(sourceTitleType) : "";
            }
            function refreshContentVisible() : void {
                contentVisible = rowVisible && cellText.length > 0;
            }
            function refreshResolvedText() : void {
                let nextText = resolvedCellText();
                if (cellText !== nextText) {
                    cellText = nextText;
                }
                refreshContentVisible();
                if (textRenderer) {
                    textRenderer.resolvedText = cellText;
                }
            }
            onCellChanged: refreshResolvedText()
            onSourceTitleTypeChanged: refreshResolvedText()
            onRowVisibleChanged: refreshContentVisible()
            positionMap: root.barPositionMap
            slot: index
            scaleOverride: root.scaleOverride
            useSlotRow: true
            usePositionMap: true
            hasOverride: false
            adjustX: 0
            adjustY: 0
            fallbackX: 0
            fallbackY: 0
            width: root.width
            height: root.height
            visible: contentVisible

            Lr2TextRenderer {
                id: textRenderer

                anchors.fill: parent
                dsts: root.timelineDsts
                srcData: root.srcData
                skinTime: 0
                activeOptions: root.activeOptions
                timers: root.timers
                timerFire: -2147483648
                scaleOverride: root.scaleOverride
                stateOverride: root.staticTimelineState
                stateOverrideSource: root.hasStaticTimelineState ? null : root.timelineState
                Component.onCompleted: barTextDelegate.refreshResolvedText()
            }

            Connections {
                target: barTextDelegate.cell
                function onCoreChanged() : void {
                    barTextDelegate.refreshResolvedText();
                }
            }
        }
    }
}

