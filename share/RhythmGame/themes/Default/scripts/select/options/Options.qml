import QtQuick
import RhythmGameQml

Rectangle {
    id: optionOverlay

    readonly property bool arenaSeated: Rg.arenaSession.state === ArenaSession.InRoom
        || Rg.arenaSession.state === ArenaSession.Reconnecting

    color: {
        let c = Qt.color("black");
        c.a = 0.5;
        return c;
    }

    property bool forcePlayOptions: false
    property bool forceTargetOptions: false

    function closeForcedOptions() {
        forcePlayOptions = false;
        forceTargetOptions = false;
        login.enabled = false;
    }

    onArenaSeatedChanged: {
        if (arenaSeated) {
            login.enabled = false;
        }
    }

    function togglePlayOptions() {
        forcePlayOptions = !forcePlayOptions;
        if (forcePlayOptions) {
            forceTargetOptions = false;
        }
    }

    MouseArea {
        anchors.fill: parent
        enabled: parent.visible
        onClicked: (event) => {
            optionOverlay.closeForcedOptions();
        }
        onWheel: (wheel) => {
            wheel.accepted = true;
        }
    }

    visible: playOptions.enabled || login.enabled || targetOptions.enabled
    property bool ready: false
    Component.onCompleted: {
        ready = true;
    }
    onVisibleChanged: {
        if (!ready) {
            return;
        }
        if (visible) {
            optionsOpenSound.stop();
            optionsOpenSound.play();
        } else {
            optionsCloseSound.stop();
            optionsCloseSound.play();
        }
    }
    AudioPlayer {
        id: optionsOpenSound
        source: Rg.profileList.mainProfile.vars.generalVars.soundsetPath + "o-open";
    }
    AudioPlayer {
        id: optionsCloseSound
        source: Rg.profileList.mainProfile.vars.generalVars.soundsetPath + "o-close";
    }

    Item {
        id: options
        anchors.centerIn: parent
        height: 1080
        scale: Math.min(parent.width / 1920, parent.height / 1080)
        width: 1920

        property bool startPressed: false
        property bool selectPressed: false
        property var menuHoldOrder: []

        function applyHeldMenu() {
            const current = menuHoldOrder.length ? menuHoldOrder[menuHoldOrder.length - 1] : null;
            startPressed = current === "start";
            selectPressed = current === "select";
        }

        function holdMenu(menu) {
            const idx = menuHoldOrder.indexOf(menu);
            if (idx !== -1) {
                menuHoldOrder.splice(idx, 1);
            }
            menuHoldOrder.push(menu);
            applyHeldMenu();
        }

        function releaseMenu(menu) {
            const idx = menuHoldOrder.indexOf(menu);
            if (idx !== -1) {
                menuHoldOrder.splice(idx, 1);
            }
            applyHeldMenu();
        }

        property bool anyStartHeld: Input.start1 || Input.start2
        onAnyStartHeldChanged: anyStartHeld ? holdMenu("start") : releaseMenu("start")

        property bool anySelectHeld: Input.select1 || Input.select2
        onAnySelectHeldChanged: anySelectHeld ? holdMenu("select") : releaseMenu("select")

        Loader {
            id: playOptions
            active: enabled
            enabled: (options.startPressed || optionOverlay.forcePlayOptions) && !login.enabled
            anchors.centerIn: parent

            source: Rg.profileList.battleActive ? "PlayOptionsBattle.qml" : "PlayOptionsSingle.qml"
        }

        Loader {
            id: targetOptions
            active: enabled
            enabled: (options.selectPressed || optionOverlay.forceTargetOptions) && !login.enabled
            anchors.centerIn: parent

            source: Rg.profileList.battleActive ? "ScoreTargetSettingsBattle.qml" : "ScoreTargetSettingsSingle.qml"
        }

        Loader {
            id: login

            anchors.centerIn: parent
            active: !optionOverlay.arenaSeated
            enabled: false
            sourceComponent: Login {}
        }

        function handleStartPressed(timer) {
            if (optionOverlay.arenaSeated) {
                if (timer.running) {
                    timer.stop();
                    const session = Rg.arenaSession;
                    const preparingRound =
                        String(session.currentRoundId || "").length > 0;
                    if (session.roundsAvailable !== false
                            && !preparingRound
                            && (session.ready === true
                                || session.canReady === true)) {
                        session.setReady(session.ready !== true);
                    }
                } else {
                    timer.restart();
                }
                return;
            }
            if ((timer.running && !login.enabled) || login.enabled) {
                login.enabled = !login.enabled;
            } else {
                timer.restart();
            }
        }

        Timer {
            id: p1StartTimer
            interval: 500
        }
        property bool start1Pressed: Input.start1
        onStart1PressedChanged: {
            if (start1Pressed)
                options.handleStartPressed(p1StartTimer);
        }
        Timer {
            id: p2StartTimer
            interval: 500
        }
        property bool start2Pressed: Input.start2
        onStart2PressedChanged: {
            if (start2Pressed)
                options.handleStartPressed(p2StartTimer);
        }
    }
}
