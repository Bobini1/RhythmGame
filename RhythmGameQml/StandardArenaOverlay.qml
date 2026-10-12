pragma ComponentBehavior: Bound
import QtQuick
import RhythmGameQml

/*!
    \qmltype StandardArenaOverlay
    \inqmlmodule RhythmGameQml
    \brief Places an Arena panel and remembers its size and chat state.

    Usually use StandardArenaSelectOverlay, StandardArenaGameplayOverlay or
    StandardArenaResultOverlay. They choose when to show the panel and supply
    its default content. Place the overlay directly inside the screen root,
    or set its visual parent to the item whose coordinates it should use.

    Replace \l panelComponent to draw another panel. Bind the panel to session
    and expanded. Call setExpanded() when the user changes its size mode.
    Call setChatOpen() to change chat and save the choice in one action;
    toggleChat() switches to the other tab. Inactive overlays ignore chat actions.
    An optional Item property named dragHandle supplies a move handle. You can
    also set moveHandle explicitly. Without either, the panel has no drag handle.

    The expanded, overlayVisible and chatSelected properties are read-only
    views of the settings. Bind to them and use the overlay's actions for edits.

    The inherited \c defaultPixelRectHint sets the initial position and size.
    Players can move and resize the panel, and \c customizeMode shows its edit
    controls. Positions and preferences are saved in \c themeVars, which defaults
    to the active profile's settings for \c layoutVariant. Set it to null for
    an overlay that does not save settings. \c z controls the stacking order.

    Each screen owns its overlay. Hidden or disabled screens cannot restore chat
    state or handle the chat shortcut. The game does not install an overlay
    automatically, inspect the screen's properties or call methods on the theme.
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
