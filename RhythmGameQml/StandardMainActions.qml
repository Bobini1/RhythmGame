import QtQuick
import RhythmGameQml

// Common main-menu destinations. Skins provide their own presentation and can
// replace any action while keeping the standard controller shortcuts.
Item {
    id: root

    property var openSelectAction: null
    property var openArenaAction: null
    property var openSettingsAction: null
    property var quitAction: null
    property bool startOpensSelect: true

    function run(overrideAction, defaultAction) {
        if (typeof overrideAction === "function") {
            overrideAction();
            return true;
        }
        defaultAction();
        return true;
    }

    function openSelect() {
        return run(openSelectAction, () => globalRoot.openSelect());
    }

    function openArena() {
        return run(openArenaAction, () => globalRoot.openArenaBrowser());
    }

    function openSettings() {
        return run(openSettingsAction, () => globalRoot.openSettings());
    }

    function quit() {
        return run(quitAction, () => globalRoot.quitApplication());
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
