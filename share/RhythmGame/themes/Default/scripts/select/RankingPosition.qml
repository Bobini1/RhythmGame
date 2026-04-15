import QtQuick
import QtQuick.Effects
import QtQuick.Controls
import RhythmGameQml

Item {
    id: ir
    height: 80
    property var rankingTotalEntries: 0
    property var rankingPosition: 0
    property bool loading: false
    property string rankingLink
    property var profile: Rg.profileList.mainProfile
    property var provider: {
        let choice = profile.vars.themeVars.select[QmlUtils.themeName].rankingProvider
        switch (choice) {
            case "rhythmgame":
                return OnlineRankingModel.RhythmGame;
            case "lr2ir":
                return OnlineRankingModel.LR2IR;
            case "bokutachi":
                return OnlineRankingModel.Tachi;
            default:
                return OnlineRankingModel.RhythmGame;
        }
    }
    Binding {
        delayed: true
        ir.provider: {
            switch (profile.vars.themeVars.select[QmlUtils.themeName].rankingProvider) {
                case "rhythmgame":
                    return OnlineRankingModel.RhythmGame;
                case "lr2ir":
                    return OnlineRankingModel.LR2IR;
                case "bokutachi":
                    return OnlineRankingModel.Tachi;
                default:
                    return OnlineRankingModel.RhythmGame;
            }
        }
    }
    Binding {
        delayed: true
        target: profile.vars.themeVars.select[QmlUtils.themeName]
        property: "rankingProvider"
        value: {
            switch (ir.provider) {
                case OnlineRankingModel.RhythmGame:
                    return "rhythmgame";
                case OnlineRankingModel.LR2IR:
                    return "lr2ir";
                case OnlineRankingModel.Tachi:
                    return "bokutachi";
            }
        }
    }

    function incrementProvider() {
        switch (ir.provider) {
            case OnlineRankingModel.RhythmGame:
                ir.provider = OnlineRankingModel.LR2IR;
                break;
            case OnlineRankingModel.LR2IR:
                ir.provider = OnlineRankingModel.Tachi;
                break;
            case OnlineRankingModel.Tachi:
                ir.provider = OnlineRankingModel.RhythmGame;
                break;
        }
    }

    function decrementProvider() {
        switch (ir.provider) {
            case OnlineRankingModel.RhythmGame:
                ir.provider = OnlineRankingModel.Tachi;
                break;
            case OnlineRankingModel.LR2IR:
                ir.provider = OnlineRankingModel.RhythmGame;
                break;
            case OnlineRankingModel.Tachi:
                ir.provider = OnlineRankingModel.LR2IR;
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
            right: total.right
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
    // position/ total (total is supposed to be a bit smaller, same baseline
    Text {
        id: rankingPositionNumber
        anchors.baseline: rankingPosition.bottom
        anchors.baselineOffset: -1
        anchors.right: parent.right
        anchors.rightMargin: 55

        text: ir.rankingPosition + "/ ";

        font.pixelSize: 28
        horizontalAlignment: Text.AlignRight

        visible: !ir.loading
    }
    Text {
        id: total

        anchors.baseline: rankingPosition.bottom
        anchors.baselineOffset: -1
        anchors.right: parent.right

        text: ir.rankingTotalEntries;

        font.pixelSize: 22
        horizontalAlignment: Text.AlignRight

        visible: !ir.loading
    }
    BusyIndicator {
        anchors.top: rankingPosition.top
        anchors.bottom: rankingPosition.bottom
        anchors.left: rankingPositionNumber.left
        anchors.right: total.right
        visible: ir.loading
    }
}