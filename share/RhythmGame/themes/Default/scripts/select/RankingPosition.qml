import QtQuick
import QtQuick.Effects
import QtQuick.Controls
import RhythmGameQml
import "../common"

Item {
    id: ir
    height: 80
    property var rankingTotalEntries: 0
    property var rankingPosition: 0
    property bool loading: false
    property string rankingLink
    property var profile: Rg.profileList.mainProfile
    readonly property var generalVars: profile.vars.generalVars
    property var provider: OnlineRankingModel.RhythmGame

    ThemeFont {
        id: rankingPositionFont
        fileName: root.themeVars.rankingFont
        fallbackFileName: "file:NotoSansJP-VariableFont_wght.ttf"
    }

    function syncProviderFromGeneralVars() {
        ir.provider = ir.generalVars ? ir.generalVars.rankingProvider : OnlineRankingModel.RhythmGame;
    }

    function setProvider(providerValue) {
        if (ir.generalVars) {
            ir.generalVars.rankingProvider = providerValue;
        }
        ir.provider = providerValue;
    }

    Component.onCompleted: syncProviderFromGeneralVars()

    Connections {
        target: ir.generalVars

        function onRankingProviderChanged() {
            ir.syncProviderFromGeneralVars();
        }
    }

    function incrementProvider() {
        switch (ir.provider) {
            case OnlineRankingModel.RhythmGame:
                ir.setProvider(OnlineRankingModel.LR2IR);
                break;
            case OnlineRankingModel.LR2IR:
                ir.setProvider(OnlineRankingModel.Tachi);
                break;
            case OnlineRankingModel.Tachi:
                ir.setProvider(OnlineRankingModel.RhythmGame);
                break;
        }
    }

    function decrementProvider() {
        switch (ir.provider) {
            case OnlineRankingModel.RhythmGame:
                ir.setProvider(OnlineRankingModel.Tachi);
                break;
            case OnlineRankingModel.LR2IR:
                ir.setProvider(OnlineRankingModel.RhythmGame);
                break;
            case OnlineRankingModel.Tachi:
                ir.setProvider(OnlineRankingModel.LR2IR);
                break;
        }
    }

    Image {
        source: root.imagesUrl + "arrow"
        anchors.right: providerIcon.left
        anchors.rightMargin: 10
        anchors.top: providerIcon.top
        anchors.bottom: providerIcon.bottom
        fillMode: Image.PreserveAspectFit
        mirror: true
        layer.enabled: true
        layer.effect: MultiEffect {
            shadowEnabled: true
        }
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: ir.decrementProvider()
        }
    }
    Image {
        id: providerIcon
        source: {
            switch (ir.provider) {
                case OnlineRankingModel.RhythmGame:
                    return root.commonImagesUrl + "rhythmgame";
                case OnlineRankingModel.LR2IR:
                    return root.commonImagesUrl + "lr2ir";
                case OnlineRankingModel.Tachi:
                    return root.commonImagesUrl + "bokutachi";
            }
        }
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.leftMargin: 30
        fillMode: Image.PreserveAspectFit
        width: rankingPosition.height
        height: rankingPosition.height
        sourceSize.width: 64
        sourceSize.height: 64
        layer.enabled: true
        layer.effect: MultiEffect {
            shadowEnabled: true
        }
        z: 1
    }
    Image {
        source: root.imagesUrl + "arrow"
        anchors.left: providerIcon.right
        anchors.leftMargin: 10
        anchors.top: providerIcon.top
        anchors.bottom: providerIcon.bottom
        fillMode: Image.PreserveAspectFit
        layer.enabled: true
        layer.effect: MultiEffect {
            shadowEnabled: true
        }
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: ir.incrementProvider()
        }
    }


    MouseArea {
        anchors {
            left: rankingPosition.left
            right: rankingNumbers.right
            top: rankingPosition.top
            bottom: rankingPosition.bottom
        }
        cursorShape: enabled ? Qt.PointingHandCursor : undefined
        enabled: ir.rankingLink !== ""
        onClicked: {
            Qt.openUrlExternally(ir.rankingLink)
        }
    }
    Image {
        id: rankingPosition
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.leftMargin: 100
        asynchronous: true
        source: root.iniImagesUrl + "parts.png/ir"
    }
    Item {
        id: rankingNumbers

        anchors.left: rankingPosition.right
        anchors.leftMargin: 10
        anchors.right: parent.right
        anchors.rightMargin: 4
        anchors.top: rankingPosition.top
        anchors.bottom: rankingPosition.bottom
        visible: !ir.loading

        Text {
            id: positionSizer

            visible: false
            font.family: rankingPositionFont.fontFamily
            font.italic: rankingPositionFont.italic
            font.pixelSize: 28
            font.weight: rankingPositionFont.fontWeight
            text: ir.rankingPosition + "/"
        }

        Text {
            id: totalSizer

            visible: false
            font.family: rankingPositionFont.fontFamily
            font.italic: rankingPositionFont.italic
            font.pixelSize: 22
            font.weight: rankingPositionFont.fontWeight
            text: ir.rankingTotalEntries
        }

        Text {
            id: positionReserveSizer

            visible: false
            font.family: rankingPositionFont.fontFamily
            font.italic: rankingPositionFont.italic
            font.pixelSize: 28
            font.weight: rankingPositionFont.fontWeight
            text: "88888/"
        }

        Text {
            id: totalReserveSizer

            visible: false
            font.family: rankingPositionFont.fontFamily
            font.italic: rankingPositionFont.italic
            font.pixelSize: 22
            font.weight: rankingPositionFont.fontWeight
            text: "88888"
        }

        readonly property real slotGap: 3
        readonly property real naturalPositionWidth: Math.max(positionSizer.implicitWidth, positionReserveSizer.implicitWidth)
        readonly property real naturalTotalWidth: Math.max(totalSizer.implicitWidth, totalReserveSizer.implicitWidth)
        readonly property real naturalTextWidth: naturalPositionWidth + naturalTotalWidth
        readonly property real textAvailableWidth: Math.max(0, width - slotGap)
        readonly property real scaleFactor: naturalTextWidth > 0 && textAvailableWidth > 0 ? Math.min(1, textAvailableWidth / naturalTextWidth) : 1
        readonly property real positionWidth: naturalPositionWidth * scaleFactor
        readonly property real totalWidth: naturalTotalWidth * scaleFactor
        readonly property real contentWidth: positionWidth + totalWidth + slotGap
        readonly property real contentLeft: Math.max(0, width - contentWidth)
        readonly property real textBaseline: rankingPosition.height - 1

        Text {
            id: rankingPositionNumber

            anchors.baseline: parent.top
            anchors.baselineOffset: rankingNumbers.textBaseline
            anchors.left: parent.left
            anchors.leftMargin: rankingNumbers.contentLeft
            font.family: rankingPositionFont.fontFamily
            font.weight: rankingPositionFont.fontWeight
            font.italic: rankingPositionFont.italic
            font.pixelSize: 28 * rankingNumbers.scaleFactor
            horizontalAlignment: Text.AlignRight
            text: ir.rankingPosition + "/"
            width: rankingNumbers.positionWidth
        }
        Text {
            id: total

            anchors.baseline: parent.top
            anchors.baselineOffset: rankingNumbers.textBaseline
            anchors.left: rankingPositionNumber.right
            anchors.leftMargin: rankingNumbers.slotGap
            font.family: rankingPositionFont.fontFamily
            font.weight: rankingPositionFont.fontWeight
            font.italic: rankingPositionFont.italic
            font.pixelSize: 22 * rankingNumbers.scaleFactor
            horizontalAlignment: Text.AlignRight
            text: ir.rankingTotalEntries
            width: rankingNumbers.totalWidth
        }
    }
    BusyIndicator {
        anchors.fill: rankingNumbers
        visible: ir.loading
    }
}
