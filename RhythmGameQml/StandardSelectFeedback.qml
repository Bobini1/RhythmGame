import QtQuick
import RhythmGameQml

// Optional default audio feedback for selection history navigation.
Item {
    id: root

    property url enterSoundSource:
        Rg.profileList.mainProfile.vars.generalVars.soundsetPath + "f-open"
    property url leaveSoundSource:
        Rg.profileList.mainProfile.vars.generalVars.soundsetPath + "f-close"
    property var enterAction: null
    property var leaveAction: null

    function enterFolder() {
        if (!enabled) {
            return;
        }
        if (typeof enterAction === "function") {
            enterAction();
            return;
        }
        enterSound.stop();
        enterSound.play();
    }

    function leaveFolder() {
        if (!enabled) {
            return;
        }
        if (typeof leaveAction === "function") {
            leaveAction();
            return;
        }
        leaveSound.stop();
        leaveSound.play();
    }

    AudioPlayer {
        id: enterSound
        source: root.enterSoundSource
    }

    AudioPlayer {
        id: leaveSound
        source: root.leaveSoundSource
    }
}
