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
    or set \c viewport to the item whose coordinates it should use.

    Replace \l panelComponent to draw another panel. Its root item must expose
    an Item property named \c dragHandle, or you can set \c moveHandle yourself.
    Bind the panel to \l session and \c expanded. Call \c setExpanded() when the
    user changes its size mode, and \c setChatSelected() after changing the
    session's chat tab. \l toggleChat handles both chat and its saved preference.

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
    viewport: parent
    moveHandle: root.panel ? root.panel.dragHandle : null
    visible: root.active && (root.overlayVisible || root.customizeMode)

    /*! Toggles chat and saves the choice, when the overlay is presented. */
    function toggleChat() {
        if (!root.presentationActive || !root.session) {
            return;
        }
        root.session.toggleChat();
        root.setChatSelected(root.session.chatOpen === true);
    }

    /*! \internal */
    function restorePresentation() {
        Qt.callLater(function() {
            if (root.presentationActive) {
                root.restoreChatSelection(root.session);
            }
        });
    }

    onPresentationActiveChanged: root.restorePresentation()
    onPresentationStateReloaded: root.restorePresentation()
    onOverlayVisibilityCommitted: visible => {
        if (!visible && root.active && root.enabled && root.session) {
            root.session.setChatOpen(false);
        }
    }

    Loader {
        id: panelLoader

        anchors.fill: parent
        active: root.active
        sourceComponent: root.panelComponent
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
