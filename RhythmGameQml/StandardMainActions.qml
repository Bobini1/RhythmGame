import QtQuick
import RhythmGameQml

/*!
    \qmltype StandardMainActions
    \inqmlmodule RhythmGameQml
    \brief Provides common main-menu destinations.

    Skins provide their own presentation and can replace any action while
    retaining the standard controller input.
*/
Item {
    id: root

    /*! Optional replacement for opening song selection. */
    property var openSelectAction: null
    /*! Optional replacement for opening the Arena browser. */
    property var openArenaAction: null
    /*! Optional replacement for opening settings. */
    property var openSettingsAction: null
    /*! Optional replacement for quitting the application. */
    property var quitAction: null
    /*! Whether the controller Start button opens song selection. */
    property bool startOpensSelect: true

    QtObject {
        id: implementation

        function run(overrideAction, defaultAction) {
            if (typeof overrideAction === "function") {
                overrideAction();
                return true;
            }
            defaultAction();
            return true;
        }
    }

    /*! Opens song selection using the replacement or built-in action. */
    function openSelect() {
        return implementation.run(openSelectAction,
                                  () => globalRoot.openSelect());
    }

    /*! Opens the Arena browser using the replacement or built-in action. */
    function openArena() {
        return implementation.run(openArenaAction,
                                  () => globalRoot.openArenaBrowser());
    }

    /*! Opens settings using the replacement or built-in action. */
    function openSettings() {
        return implementation.run(openSettingsAction,
                                  () => globalRoot.openSettings());
    }

    /*! Quits using the replacement or built-in action. */
    function quit() {
        return implementation.run(quitAction,
                                  () => globalRoot.quitApplication());
    }

    Input.onStart1Pressed: {
        if (root.enabled && root.startOpensSelect) {
            root.openSelect();
        }
    }
    Input.onStart2Pressed: {
        if (root.enabled && root.startOpensSelect) {
            root.openSelect();
        }
    }
}
