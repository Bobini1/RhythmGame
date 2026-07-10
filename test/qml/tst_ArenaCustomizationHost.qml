import QtQuick
import QtTest
import RhythmGameQml

TestCase {
    id: testCase

    name: "ArenaCustomizationHost"
    when: windowShown
    width: 1280
    height: 720
    visible: true

    Component {
        id: runnerComponent

        QtObject {
        }
    }

    Component {
        id: sessionComponent

        QtObject {
            property bool arenaGameplayActive: true
            property var arenaRunner: null
            property bool resultPresentationActive: false
            property bool gameplayChatOpen: false
            property bool overlayCustomizationActive: false
            property var liveStandings: null
            property var chat: null
            property string arenaOptionsSummary: ""
            property var callLog: []

            function record(value) {
                callLog = callLog.concat([value]);
            }

            function setGameplayChatOpen(open) {
                record("chat:" + open);
                gameplayChatOpen = open && arenaGameplayActive;
            }

            function setOverlayCustomizationActive(active) {
                record("overlay:" + active);
                overlayCustomizationActive = active && arenaGameplayActive
                    && arenaRunner !== null;
            }

            function toggleGameplayChat() {
                setGameplayChatOpen(!gameplayChatOpen);
            }
        }
    }

    Component {
        id: defaultScreenComponent

        Item {
            required property var session
            property var chart: null
            property bool arenaCustomizeMode: false

            function setArenaCustomizeMode(active) {
                session.record("screen:" + active);
                arenaCustomizeMode = active;
            }
        }
    }

    Component {
        id: legacyScreenComponent

        Item {
            id: legacyScreen

            required property var session
            property var chart: null
            property int escapeActivations: 0

            Shortcut {
                enabled: !legacyScreen.session.overlayCustomizationActive
                sequence: "Esc"

                onActivated: legacyScreen.escapeActivations += 1
            }
        }
    }

    Component {
        id: hostComponent

        ArenaOverlayHost {
        }
    }

    function createHarness(nativeScreen = true, chatOpen = false) {
        const runner = createTemporaryObject(runnerComponent, testCase);
        verify(runner !== null);
        const session = createTemporaryObject(sessionComponent, testCase, {
            "arenaRunner": runner,
            "gameplayChatOpen": chatOpen
        });
        verify(session !== null);
        const screen = createTemporaryObject(
            nativeScreen ? defaultScreenComponent : legacyScreenComponent,
            testCase,
            nativeScreen ? { "chart": runner, "session": session }
                         : { "chart": runner, "session": session });
        verify(screen !== null);
        const host = createTemporaryObject(hostComponent, testCase, {
            "currentItem": screen,
            "layoutVariant": "k7",
            "resolvedSkinId": nativeScreen ? "Default" : "Legacy",
            "session": session,
            "width": testCase.width,
            "height": testCase.height
        });
        verify(host !== null);
        wait(1);
        return {
            "runner": runner,
            "session": session,
            "screen": screen,
            "host": host
        };
    }

    function test_default_f2_has_one_owner_and_ordered_coordination() {
        const harness = createHarness(true, true);
        compare(harness.host.arenaShortcutEnabled, true);
        compare(harness.host.customizeMode, false);

        keyClick(Qt.Key_F2);
        tryCompare(harness.host, "customizeMode", true);
        compare(harness.session.gameplayChatOpen, false);
        compare(harness.session.overlayCustomizationActive, true);
        compare(harness.screen.arenaCustomizeMode, true);
        compare(harness.session.callLog,
                ["chat:false", "overlay:true", "screen:true"]);
        const shield = findChild(harness.host,
                                 "arenaLegacyCustomizationShield");
        verify(shield !== null);
        compare(shield.visible, false);

        keyClick(Qt.Key_F2);
        tryCompare(harness.host, "customizeMode", false);
        compare(harness.session.overlayCustomizationActive, false);
        compare(harness.screen.arenaCustomizeMode, false);
        compare(harness.session.callLog,
                ["chat:false", "overlay:true", "screen:true",
                 "overlay:false", "screen:false"]);
    }

    function test_legacy_shield_and_keyboard_exit_are_mode_scoped() {
        const harness = createHarness(false);
        const shield = findChild(harness.host,
                                 "arenaLegacyCustomizationShield");
        verify(shield !== null);
        compare(shield.visible, false);
        compare(shield.enabled, false);

        keyClick(Qt.Key_F2);
        tryCompare(harness.host, "customizeMode", true);
        compare(shield.visible, true);
        compare(shield.enabled, true);

        keyClick(Qt.Key_Return);
        tryCompare(harness.host, "customizeMode", false);
        compare(shield.visible, false);

        keyClick(Qt.Key_F2);
        tryCompare(harness.host, "customizeMode", true);
        keyClick(Qt.Key_Escape);
        tryCompare(harness.host, "customizeMode", false);
        compare(harness.session.overlayCustomizationActive, false);
        compare(harness.screen.escapeActivations, 0);
    }

    function test_chat_open_and_host_destruction_restore_customization() {
        const harness = createHarness(true);
        keyClick(Qt.Key_F2);
        tryCompare(harness.host, "customizeMode", true);

        harness.session.setGameplayChatOpen(true);
        tryCompare(harness.host, "customizeMode", false);
        compare(harness.screen.arenaCustomizeMode, false);
        compare(harness.session.overlayCustomizationActive, false);
        compare(harness.session.gameplayChatOpen, true);

        harness.session.setGameplayChatOpen(false);
        keyClick(Qt.Key_F2);
        tryCompare(harness.host, "customizeMode", true);
        harness.host.destroy();
        wait(1);
        compare(harness.session.overlayCustomizationActive, false);
        compare(harness.screen.arenaCustomizeMode, false);
    }

    function test_current_screen_loss_restores_customization() {
        const harness = createHarness(true);
        keyClick(Qt.Key_F2);
        tryCompare(harness.host, "customizeMode", true);

        harness.host.currentItem = null;
        tryCompare(harness.host, "customizeMode", false);
        compare(harness.session.overlayCustomizationActive, false);
        compare(harness.screen.arenaCustomizeMode, false);
    }
}
