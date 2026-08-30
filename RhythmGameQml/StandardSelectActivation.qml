import QtQuick
import RhythmGameQml

/*!
    \qmltype StandardSelectActivation
    \inqmlmodule RhythmGameQml
    \brief Opens selected charts and courses independently of presentation.

    Assign \c tryOpenPlayableAction to intercept activation. Returning true
    from that action suppresses the built-in behavior.
*/
QtObject {
    id: root

    /*! Optional opening pre-handler. True consumes; false or undefined continues. */
    property var tryOpenPlayableAction: null
    /*! Whether the local player is currently seated in an Arena room. */
    readonly property bool arenaSeated:
        Rg.arenaSession.state === ArenaSession.InRoom
        || Rg.arenaSession.state === ArenaSession.Reconnecting

    /*!
        Opens \a item using \a autoplay, \a replay, and \a replayScore to
        select the requested play mode.
    */
    function openPlayable(item, autoplay = false, replay = false,
                          replayScore = null) {
        if (typeof tryOpenPlayableAction === "function"
                && tryOpenPlayableAction(
                    item, autoplay, replay, replayScore)) {
            return true;
        }
        if (item instanceof ChartData) {
            if (arenaSeated) {
                if (!autoplay && !replay && !replayScore) {
                    Rg.arenaSession.selectChart(item);
                }
                return true;
            }
            let useReplay = !!replay && !!replayScore;
            if (Rg.profileList.battleActive) {
                globalRoot.openChart(
                    item.path, Rg.profileList.battleProfiles.player1Profile,
                    !!autoplay, useReplay, replayScore || null,
                    Rg.profileList.battleProfiles.player2Profile,
                    !!autoplay, false, null);
            } else {
                globalRoot.openChart(
                    item.path, Rg.profileList.mainProfile, !!autoplay,
                    useReplay, replayScore || null, null, false, false, null);
            }
            return true;
        }
        if (item instanceof course) {
            if (arenaSeated) {
                return true;
            }
            let useReplay = !!replay && !!replayScore;
            if (Rg.profileList.battleActive) {
                globalRoot.openCourse(
                    item, Rg.profileList.battleProfiles.player1Profile,
                    !!autoplay, useReplay, replayScore || null,
                    Rg.profileList.battleProfiles.player2Profile,
                    !!autoplay, false, null);
            } else {
                globalRoot.openCourse(
                    item, Rg.profileList.mainProfile, !!autoplay,
                    useReplay, replayScore || null, null, false, false, null);
            }
            return true;
        }
        return false;
    }
}
