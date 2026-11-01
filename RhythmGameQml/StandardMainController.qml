import QtQuick
import RhythmGameQml

/*!
    \qmltype StandardMainController
    \inqmlmodule RhythmGameQml
    \brief Supplies main-menu actions, Start input and background music.

    Place this component inside the main screen. It inherits the screen's
    enabled state and includes StandardMainActions, so buttons can call
    openSelect(), openArena(), openSettings() and quit().

    The included \l backgroundMusic plays the active profile's main-menu track
    while the controller is enabled. Change its source, volume or fadeInMillis
    to customize playback. Set backgroundMusic to null to omit it.
    StandardMainActions is also available on its own for menus that only need
    the actions and Start input.
*/
StandardMainActions {
    id: root

    /*!
        Looping main-menu music. Its source defaults to the active profile's
        bgmPath followed by "main". The player accepts a local file path.
        Disabling the controller stops playback.

        Customize the player's properties, set it to null, or supply another
        AudioPlayer. A replacement owns its playback binding and must stop
        when the controller is disabled.
    */
    property AudioPlayer backgroundMusic: AudioPlayer {
        id: defaultBackgroundMusic

        source: {
            const path = Rg.profileList.mainProfile.vars.generalVars.bgmPath;
            return root.backgroundMusic === defaultBackgroundMusic && path
                ? path + "main" : "";
        }
        playing: root.enabled && root.backgroundMusic === defaultBackgroundMusic
            && defaultBackgroundMusic.loaded
        looping: true
        fadeInMillis: 1000
    }
}
