pragma ComponentBehavior: Bound
import QtQuick
import QtTest
import "../../RhythmGameQml/Arena" as ArenaLocal

TestCase {
    id: testCase

    name: "ArenaStatusAnnouncer"
    when: windowShown
    width: 320
    height: 200
    visible: true

    Component {
        id: announcerComponent

        ArenaLocal.ArenaStatusAnnouncer {
            active: true
            errorMessageKey: ""
            reconnecting: false
            roundLaunchCancellationStatusKey: ""
            target: testCase
        }
    }

    function test_round_cancellation_is_announced_once_and_rearmed_by_clear() {
        const announcer = createTemporaryObject(announcerComponent, testCase);
        verify(announcer !== null);

        announcer.roundLaunchCancellationStatusKey = "arena.status.roundLaunchCancelled.parseFailed";
        tryCompare(announcer, "announcementCount", 1);
        compare(announcer.lastAnnouncementKey, "arena.status.roundLaunchCancelled.parseFailed");
        verify(announcer.lastAnnouncementText.length > 0);

        announcer.roundLaunchCancellationStatusKey = "arena.status.roundLaunchCancelled.parseFailed";
        compare(announcer.announcementCount, 1);
        announcer.roundLaunchCancellationStatusKey = "";
        compare(announcer.announcementCount, 1);
        announcer.errorMessageKey = "arena.error.parseFailed";
        compare(announcer.announcementCount, 1);

        announcer.roundLaunchCancellationStatusKey = "arena.status.roundLaunchCancelled.parseFailed";
        tryCompare(announcer, "announcementCount", 2);
    }

    function test_each_round_cancellation_reason_has_announcement_text_data() {
        return [
            {
                "tag": "missing file",
                "key": "arena.status.roundLaunchCancelled.missingFile"
            },
            {
                "tag": "hash mismatch",
                "key": "arena.status.roundLaunchCancelled.hashMismatch"
            },
            {
                "tag": "read failed",
                "key": "arena.status.roundLaunchCancelled.readFailed"
            },
            {
                "tag": "parse failed",
                "key": "arena.status.roundLaunchCancelled.parseFailed"
            },
            {
                "tag": "unsupported config",
                "key": "arena.status.roundLaunchCancelled.unsupportedConfig"
            },
            {
                "tag": "resource failed",
                "key": "arena.status.roundLaunchCancelled.resourceFailed"
            },
            {
                "tag": "probe timeout",
                "key": "arena.status.roundLaunchCancelled.probeTimeout"
            },
            {
                "tag": "load timeout",
                "key": "arena.status.roundLaunchCancelled.loadTimeout"
            },
            {
                "tag": "participant left",
                "key": "arena.status.roundLaunchCancelled.participantLeft"
            },
            {
                "tag": "participant kicked",
                "key": "arena.status.roundLaunchCancelled.participantKicked"
            },
            {
                "tag": "chart length mismatch",
                "key": "arena.status.roundLaunchCancelled.chartLengthMismatch"
            },
            {
                "tag": "server shutdown",
                "key": "arena.status.roundLaunchCancelled.serverShutdown"
            },
            {
                "tag": "cancelled",
                "key": "arena.status.roundLaunchCancelled.cancelled"
            }
        ];
    }

    function test_each_round_cancellation_reason_has_announcement_text(data) {
        const announcer = createTemporaryObject(announcerComponent, testCase);
        verify(announcer !== null);
        announcer.roundLaunchCancellationStatusKey = data.key;
        tryCompare(announcer, "announcementCount", 1);
        compare(announcer.lastAnnouncementKey, data.key);
        verify(announcer.lastAnnouncementText.length > 0);
    }
}
