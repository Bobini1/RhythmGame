import QtQuick
import RhythmGameQml

Rectangle {
    color: {
        let c = Qt.color("black");
        c.a = 0.5;
        return c;
    }

    MouseArea {
        anchors.fill: parent
        enabled: parent.visible
        onClicked: (event) => {
            login.enabled = false;
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
            enabled: options.startPressed && !login.enabled
            anchors.centerIn: parent

            source: Rg.profileList.battleActive ? "PlayOptionsBattle.qml" : "PlayOptionsSingle.qml"
        }

        Loader {
            id: targetOptions
            active: enabled
            enabled: options.selectPressed && !login.enabled
            anchors.centerIn: parent

            source: Rg.profileList.battleActive ? "ScoreTargetSettingsBattle.qml" : "ScoreTargetSettingsSingle.qml"
        }

        Login {
            id: login

            anchors.centerIn: parent
            enabled: false
        }
        Timer {
            id: p1StartTimer
            interval: 500
        }
        property bool start1Pressed: Input.start1
        onStart1PressedChanged: {
            if (start1Pressed) {
                if (p1StartTimer.running && !login.enabled || login.enabled) {
                    login.enabled = !login.enabled;
                } else {
                    p1StartTimer.restart();
                }
            }
        }
        Timer {
            id: p2StartTimer
            interval: 500
        }
        property bool start2Pressed: Input.start2
        onStart2PressedChanged: {
            if (start2Pressed) {
                if (p2StartTimer.running && !login.enabled || login.enabled) {
                    login.enabled = !login.enabled;
                } else {
                    p2StartTimer.restart();
                }
            }
        }
    }
}