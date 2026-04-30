pragma ValueTypeBehavior: Addressable

import QtQuick
import "Lr2Timeline.js" as Lr2Timeline

QtObject {
    id: root

    required property var host
    property bool tracking: false
    property real skinScale: 1
    property var zeroTimers: ({ "0": 0 })
    property var emptyActiveOptions: []

    property var elements: ({})
    readonly property int elementCount: Object.keys(root.elements).length
    readonly property var candidateKeys: {
        if (!root.host) {
            return [];
        }
        root.host.selectPanel;
        let result = [];
        const keys = Object.keys(root.elements);
        for (let i = 0; i < keys.length; ++i) {
            const key = keys[i];
            const element = root.elements[key];
            if (element && element.src && root.host.panelMatches(element.src.hoverPanel || 0)) {
                result.push(key);
            }
        }
        return result;
    }
    property var visibleByIndex: ({})
    readonly property string visibleSignature: Object.keys(root.visibleByIndex).join(",")
    property int revision: 0
    property bool refreshQueued: false
    property int skinX: -1
    property int skinY: -1
    readonly property bool hasPoint: root.skinX >= 0 && root.skinY >= 0

    onTrackingChanged: {
        if (!root.tracking) {
            root.clearPoint();
        }
    }

    onElementCountChanged: {
        if (root.elementCount <= 0) {
            root.clearPoint();
        }
    }

    onCandidateKeysChanged: root.scheduleRefresh()

    property FrameAnimation refreshAnimation: FrameAnimation {
        running: false
        onTriggered: {
            root.refreshQueued = false;
            running = false;
            root.refreshCache();
        }
    }

    function copyObject(value) {
        let result = {};
        for (let key in value || {}) {
            result[key] = value[key];
        }
        return result;
    }

    function pointInSkinCoordinates() {
        if (!root.tracking || !root.hasPoint) {
            return null;
        }
        return Qt.point(root.skinX * root.skinScale, root.skinY * root.skinScale);
    }

    function updatePoint(x, y) {
        if (!root.tracking) {
            root.clearPoint();
            return false;
        }
        const mx = Math.floor(x / root.skinScale);
        const my = Math.floor(y / root.skinScale);
        if (mx === root.skinX && my === root.skinY) {
            return false;
        }
        root.skinX = mx;
        root.skinY = my;
        root.scheduleRefresh();
        return true;
    }

    function clearPoint() {
        if (!root.hasPoint) {
            root.clearCache();
            return;
        }
        root.skinX = -1;
        root.skinY = -1;
        root.clearCache();
    }

    function registerElement(elementIndex, src, dsts, enabled) {
        const key = String(elementIndex);
        const hadElement = root.elements[key] !== undefined;
        if (!enabled || !src || !src.onMouse) {
            if (hadElement) {
                let nextElements = root.copyObject(root.elements);
                delete nextElements[key];
                root.elements = nextElements;
                root.scheduleRefresh();
            }
            return;
        }

        let nextElements = root.copyObject(root.elements);
        nextElements[key] = {
            "src": src,
            "dsts": dsts,
            "staticState": Lr2Timeline.canUseStaticState(dsts)
                ? Lr2Timeline.getCurrentState(dsts, 0, root.zeroTimers, root.emptyActiveOptions)
                : null
        };
        root.elements = nextElements;
        root.scheduleRefresh();
    }

    function unregisterElement(elementIndex) {
        const key = String(elementIndex);
        if (root.elements[key] === undefined) {
            return;
        }
        let nextElements = root.copyObject(root.elements);
        delete nextElements[key];
        root.elements = nextElements;
        root.scheduleRefresh();
    }

    function scheduleRefresh() {
        if (!root.tracking || root.refreshQueued || root.refreshAnimation.running) {
            return;
        }
        root.refreshQueued = true;
        root.refreshAnimation.running = true;
    }

    function stopRefresh() {
        root.refreshQueued = false;
        root.refreshAnimation.running = false;
    }

    function runRefresh() {
        if (root.refreshQueued) {
            root.refreshQueued = false;
            root.refreshCache();
        }
    }

    function clearCache() {
        if (Object.keys(root.visibleByIndex).length === 0) {
            return;
        }
        root.visibleByIndex = {};
        root.revision += 1;
    }

    function refreshCache() {
        if (!root.tracking || root.candidateKeys.length <= 0 || !root.hasPoint) {
            root.clearCache();
            return;
        }

        const mx = root.skinX;
        const my = root.skinY;
        let visible = {};
        let visibleKeys = [];
        const keys = root.candidateKeys;
        for (let i = 0; i < keys.length; ++i) {
            const key = keys[i];
            const element = root.elements[key];
            if (root.host.onMouseElementStateAt(element, mx, my) !== null) {
                visible[key] = true;
                visibleKeys.push(key);
            }
        }

        const signature = visibleKeys.join(",");
        if (signature === root.visibleSignature) {
            return;
        }
        root.visibleByIndex = visible;
        root.revision += 1;
    }
}
