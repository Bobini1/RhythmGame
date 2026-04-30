pragma ValueTypeBehavior: Addressable

import QtQuick

QtObject {
    id: root

    required property real sceneStartMs

    readonly property var initialNow: new Date()
    readonly property double initialNowMs: initialNow.getTime()
    property double nowMs: initialNowMs
    property int year: initialNow.getFullYear()
    property int month: initialNow.getMonth() + 1
    property int day: initialNow.getDate()
    property int hour: initialNow.getHours()
    property int minute: initialNow.getMinutes()
    property int second: initialNow.getSeconds()
    property int uptimeSeconds: 0

    readonly property int uptimeHours: Math.floor(uptimeSeconds / 3600)
    readonly property int uptimeMinutes: Math.floor(uptimeSeconds / 60) % 60
    readonly property int uptimeSecondPart: uptimeSeconds % 60

    function update() {
        let nextNowMs = Date.now();
        let nextNow = new Date(nextNowMs);
        let nextUptimeSeconds = Math.floor(Math.max(0, nextNowMs - root.sceneStartMs) / 1000);

        if (root.nowMs !== nextNowMs)
            root.nowMs = nextNowMs;
        if (root.year !== nextNow.getFullYear())
            root.year = nextNow.getFullYear();
        if (root.month !== nextNow.getMonth() + 1)
            root.month = nextNow.getMonth() + 1;
        if (root.day !== nextNow.getDate())
            root.day = nextNow.getDate();
        if (root.hour !== nextNow.getHours())
            root.hour = nextNow.getHours();
        if (root.minute !== nextNow.getMinutes())
            root.minute = nextNow.getMinutes();
        if (root.second !== nextNow.getSeconds())
            root.second = nextNow.getSeconds();
        if (root.uptimeSeconds !== nextUptimeSeconds)
            root.uptimeSeconds = nextUptimeSeconds;
    }
}
