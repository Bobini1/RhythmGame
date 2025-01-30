import QtQuick
import QtQml
import RhythmGameQml
import QtQuick.Controls.Basic 2.15
import QtCore

ApplicationWindow {
    id: contentContainer

    height: 720
    visible: true
    width: 1280

    Settings {
        property alias height: contentContainer.height
        property alias width: contentContainer.width
    }
    Component {
        id: chartContext

        FocusScope {
            id: chartFocusScope

            readonly property bool active: StackView.status === StackView.Active
            required property Chart chart
            readonly property string screen: {
                let keys = chartFocusScope.chart.chartData.keymode;
                let battle = chartFocusScope.chart.player1 && chartFocusScope.chart.player2;
                return "k" + keys + (battle ? "battle" : "");
            }

            Loader {
                id: loader

                anchors.fill: parent
                source: Themes.availableThemeFamilies[ProfileList.mainProfile.themeConfig[chartFocusScope.screen]].screens[chartFocusScope.screen].script
            }
        }
    }
    Component {
        id: resultContext

        FocusScope {
            id: resultFocusScope

            readonly property bool active: StackView.status === StackView.Active
            required property ChartData chartData
            required
            property
                list<BmsScoreAftermath> result

            Loader {
                id: loader

                anchors.fill: parent
                source: Themes.availableThemeFamilies[ProfileList.mainProfile.themeConfig.result].screens.result.script
            }
        }
    }
    Item {
        id: globalRoot

        readonly property Profile mainProfile: ProfileList.mainProfile
        readonly property Component gameplayComponent: chartContext
        readonly property Component mainComponent: Qt.createComponent(Themes.availableThemeFamilies[mainProfile.themeConfig.main].screens.main.script)
        readonly property Component resultComponent: resultContext
        readonly property Component settingsComponent: Qt.createComponent(Themes.availableThemeFamilies[mainProfile.themeConfig.settings].screens.settings.script)
        readonly property Component songWheelComponent: Qt.createComponent(Themes.availableThemeFamilies[mainProfile.themeConfig.songWheel].screens.songWheel.script)

        function openChart(path) {
            let chart;
            if (ProfileList.battleActive) {
                chart = ChartLoader.loadChart(path, ProfileList.battleProfiles.player1Profile, ProfileList.battleProfiles.player2Profile);
            } else {
                chart = ChartLoader.loadChart(path, ProfileList.mainProfile, null);
            }
            if (!chart) {
                console.error("Failed to load chart");
                return;
            }
            sceneStack.pushItem(gameplayComponent, {
                "chart": chart
            });
        }

        function openResult(result, chartData) {
            sceneStack.pushItem(resultComponent, {
                "result": result,
                "chartData": chartData
            });
        }

        function urlToPath(urlString) {
            let s;
            if (urlString.startsWith("file:///")) {
                let k = urlString.charAt(9) === ':' ? 8 : 7;
                s = urlString.substring(k);
            } else {
                s = urlString;
            }
            return decodeURIComponent(s);
        }

        anchors.fill: parent

        StackView {
            id: sceneStack

            onCurrentItemChanged: {
                updateEnabledStates();
            }

            function updateEnabledStates() {
                let topIndex = depth - 1;
                for (let i = 0; i < depth; ++i) {
                    let item = get(i, StackView.ForceLoad);
                    if (item) {
                        item.enabled = (i === topIndex);
                    }
                }
            }

            anchors.fill: parent
            initialItem: globalRoot.mainComponent

            popEnter: Transition {
                PropertyAnimation {
                    duration: 0
                    properties: "opacity"
                }
            }
            popExit: Transition {
                PropertyAnimation {
                    duration: 0
                    properties: "opacity"
                }
            }
            pushEnter: Transition {
                PropertyAnimation {
                    duration: 0
                    properties: "opacity"
                }
            }
            pushExit: Transition {
                PropertyAnimation {
                    duration: 0
                    properties: "opacity"
                }
            }
            replaceEnter: Transition {
                PropertyAnimation {
                    duration: 0
                    properties: "opacity"
                }
            }
            replaceExit: Transition {
                PropertyAnimation {
                    duration: 0
                    properties: "opacity"
                }
            }
        }
        Loader {
            id: debugLogLoader

            active: false
            anchors.fill: parent
            asynchronous: true
            source: "Log.qml"
        }
        Shortcut {
            autoRepeat: false
            sequence: "F11"

            onActivated: {
                debugLogLoader.active = !debugLogLoader.active;
            }
        }
    }
}
