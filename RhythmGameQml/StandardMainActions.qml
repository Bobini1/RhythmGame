import QtQuick
import RhythmGameQml

/*!
    \qmltype StandardMainActions
    \inqmlmodule RhythmGameQml
    \brief Provides common main-menu destinations.

    Skins provide their own presentation and can replace any action while
    retaining the standard controller input.

    Instantiating this component also listens for either player's Start button.
    While \l enabled and \l startOpensSelect are true, Start calls
    \l openSelect. Set \l startOpensSelect to false when the skin owns that
    input. The four public methods are suitable for buttons and menus; each
    calls its matching replacement action when provided, otherwise the
    application-owned \c globalRoot operation.
*/
Item {
    id: root

    /*! Optional \c openSelectAction() replacement. */
    property var openSelectAction: null
    /*! Optional \c openArenaAction() replacement. */
    property var openArenaAction: null
    /*! Optional \c openSettingsAction() replacement. */
    property var openSettingsAction: null
    /*! Optional \c quitAction() replacement. */
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
