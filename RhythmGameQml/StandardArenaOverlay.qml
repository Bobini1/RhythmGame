pragma ComponentBehavior: Bound
import QtQuick
import RhythmGameQml

/*!
    \qmltype StandardArenaOverlay
    \inqmlmodule RhythmGameQml
    \brief Places an Arena panel and saves its position and chat preferences.

    Use StandardArenaSelectOverlay, StandardArenaGameplayOverlay or
    StandardArenaResultOverlay for a screen. They supply the default panel
    and decide when it should appear. This base component provides placement
    and chat behavior for those overlays.

    Place an overlay directly inside the screen root, or set its visual parent
    to the item whose coordinates it should use. The \c viewport property is
    that parent. Each screen owns its overlay; the game does not add one.

    \section1 Replace the panel

    Set \l panelComponent to your own Component. The loader sizes it to the
    overlay, but does not inject session or presentation properties. Bind your
    panel explicitly to the overlay's \l session and \c expanded properties.
    Call \c setExpanded(bool) when the user changes its size mode, and
    \l setChatOpen when the user opens or closes chat.

    An Item property named \c dragHandle on your panel becomes its move handle.
    Alternatively, set the overlay's \c moveHandle. Without either, the panel
    has no drag handle. Use \l panel to access the loaded item, checking for
    null while the overlay is inactive.

    \section1 Choose placement and persistence

    The placement frame supplies these additional properties:
    \table
        \header \li Property \li Use
        \row \li \c defaultPixelRectHint \li Initial rectangle in the viewport's coordinates, used when no saved rectangle exists.
        \row \li \c minimumPixelSize \li Minimum panel size at the 1920 by 1080 design scale; scaled and limited to fit the viewport.
        \row \li \c customizeMode \li Show move, resize and visibility controls.
        \row \li \c visibilityCustomizable \li Allow the player to hide the panel. The selection overlay sets this to false.
        \row \li \c themeVars \li Settings object used for saved position and preferences; null keeps changes only in memory.
        \row \li \c layoutVariant \li Screen key used for settings: select, result, k5, k7, k10 or k14. Screen-specific overlays set it for you.
    \endtable

    themeVars defaults to the active profile's settings for layoutVariant.
    Saved positions are normalized to the viewport size and kept within its
    bounds. Use \c z to choose the overlay's stacking order.

    \section1 Control visibility and chat

    \c expanded, \c overlayVisible and \c chatSelected are read-only settings.
    Use \c setExpanded(bool), \c setOverlayVisible(bool) and \l setChatOpen
    to change them. setChatOpen updates both the session and saved preference;
    \l toggleChat switches between chat and standings.

    Hidden or disabled overlays ignore chat actions and shortcuts. The panel
    can remain loaded while hidden, so use \l presentationActive when custom
    panel behavior should run only while it is shown.
*/
ArenaOverlayPlacementFrame {
    id: root

    /*! Supplies the Arena state. Defaults to Rg.arenaSession. */
    property var session: Rg.arenaSession
    /*! Controls whether this screen has an Arena panel. */
    property bool active: false
    /*! Enables the chat shortcut while the overlay is presented. */
    property bool chatShortcutEnabled: true
    /*! Sets the chat shortcut. The default is F8. */
    property string chatShortcut: "F8"
    /*! Supplies the panel's visual content. See the type description for its bindings. */
    required property Component panelComponent
    /*! Provides the loaded panel, or null while it is inactive. */
    readonly property var panel: panelLoader.status === Loader.Ready ? panelLoader.item : null
    /*! Reports whether the overlay is active, visible and enabled. */
    readonly property bool presentationActive: root.active && root.visible && root.enabled

    themeVars: {
        const profile = Rg.profileList.mainProfile;
        const vars = profile.vars.themeVars[root.layoutVariant];
        return vars ? vars[profile.themeConfig[root.layoutVariant]] : null;
    }
    moveHandle: root.panel?.dragHandle ?? null
    visible: root.active && (root.overlayVisible || root.customizeMode)

    /*! Opens chat when \a open is true, or closes it otherwise. Inactive overlays ignore the action. */
    function setChatOpen(open) {
        if (!root.presentationActive || !root.session) {
            return;
        }
        root.session.setChatOpen(open);
        root.setChatSelected(open);
    }

    /*! Toggles chat and saves the choice, when the overlay is presented. */
    function toggleChat() { root.setChatOpen(!root.session?.chatOpen); }

    onPresentationActiveChanged: Qt.callLater(panelLoader.restorePresentation)
    onChatSelectedChanged: Qt.callLater(panelLoader.restorePresentation)
    onSessionChanged: Qt.callLater(panelLoader.restorePresentation)
    onOverlayVisibleChanged: {
        if (!root.overlayVisible && root.active && root.enabled
                && root.viewport && root.viewport.visible && root.session) {
            root.session.setChatOpen(false);
        }
        Qt.callLater(panelLoader.restorePresentation);
    }

    Loader {
        id: panelLoader

        anchors.fill: parent
        active: root.active
        sourceComponent: root.panelComponent

        function restorePresentation() {
            if (root.presentationActive && root.session) {
                root.session.setChatOpen(root.overlayVisible && root.chatSelected);
            }
        }
    }

    Shortcut {
        autoRepeat: false
        context: Qt.ApplicationShortcut
        enabled: root.presentationActive && root.chatShortcutEnabled
        sequence: root.chatShortcut
        onActivated: root.toggleChat()
    }

    TransientInputFocusDismissLayer {
        parent: root.viewport
        enabled: root.presentationActive
        visible: enabled
    }
}
