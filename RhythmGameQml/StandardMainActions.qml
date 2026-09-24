import QtQuick
import RhythmGameQml

/*!
    \qmltype StandardMainActions
    \inqmlmodule RhythmGameQml
    \brief Opens the standard destinations from a main menu.

    Create buttons in your skin and connect them to \l openSelect, \l openArena,
    \l openSettings or \l quit. The component draws no menu. Each method uses
    the corresponding \c globalRoot operation unless you provide a replacement
    action. A replacement handles the whole operation, and its return value is ignored.

    The component also listens for either player's bound Start button. Start opens
    song selection while \l enabled and \l startOpensSelect are true. Set
    \l startOpensSelect to false if your skin handles Start itself.

    See the \l {../skin_tutorial_first_screen.html}{first menu lesson} for an
    installable example.
*/
Item {
    id: root

    /*! Calls \c openSelectAction() in place of the standard action. */
    property var openSelectAction: null
    /*! Calls \c openArenaAction() in place of the standard action. */
    property var openArenaAction: null
    /*! Calls \c openSettingsAction() in place of the standard action. */
    property var openSettingsAction: null
    /*! Calls \c quitAction() in place of the standard action. */
    property var quitAction: null
    /*! Controls whether the controller Start button opens song selection. */
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
