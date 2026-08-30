import QtQuick
import RhythmGameQml

/*!
    \qmltype StandardSelectFeedback
    \inqmlmodule RhythmGameQml
    \brief Provides optional audio feedback for selection history navigation.

    The default sounds come from the active profile's sound set. Assign the
    action properties to replace either sound.
*/
Item {
    id: root

    /*! Sound played after entering a selection-history item. */
    property url enterSoundSource:
        Rg.profileList.mainProfile.vars.generalVars.soundsetPath + "f-open"
    /*! Sound played after leaving a selection-history item. */
    property url leaveSoundSource:
        Rg.profileList.mainProfile.vars.generalVars.soundsetPath + "f-close"
    /*! Optional replacement for entering-folder feedback. */
    property var enterAction: null
    /*! Optional replacement for leaving-folder feedback. */
    property var leaveAction: null

    /*! Plays or invokes the entering-folder feedback. */
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

    /*! Plays or invokes the leaving-folder feedback. */
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
