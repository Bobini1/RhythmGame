pragma ValueTypeBehavior: Addressable

import QtQuick

QtObject {
    id: root

    required property var host
    property var skinRuntime: null
    property bool tracking: false
    property real skinScale: 1

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

    onCandidateKeysChanged: root.refreshIfTracking()

    function copyObject(value: var) : var {
        let result = {};
        for (let key in value || {}) {
            result[key] = value[key];
        }
        return result;
    }

    function pointInSkinCoordinates() : var {
        if (!root.tracking || !root.hasPoint) {
            return null;
        }
        return Qt.point(root.skinX * root.skinScale, root.skinY * root.skinScale);
    }

    function updatePoint(x: var, y: var) : var {
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
        root.refreshIfTracking();
        return true;
    }

    function clearPoint() : var {
        if (!root.hasPoint) {
            root.clearVisibleState();
            return;
        }
        root.skinX = -1;
        root.skinY = -1;
        root.clearVisibleState();
    }

    function registerElement(elementIndex: var, src: var, enabled: var) : var {
        const key = String(elementIndex);
        const hadElement = root.elements[key] !== undefined;
        if (!enabled || !src || !src.onMouse) {
            if (hadElement) {
                let nextElements = root.copyObject(root.elements);
                delete nextElements[key];
                root.elements = nextElements;
                root.refreshIfTracking();
            }
            return;
        }

        let nextElements = root.copyObject(root.elements);
        nextElements[key] = {
            "src": src,
            "runtimeIndex": elementIndex
        };
        root.elements = nextElements;
        root.refreshIfTracking();
    }

    function unregisterElement(elementIndex: var) : var {
        const key = String(elementIndex);
        if (root.elements[key] === undefined) {
            return;
        }
        let nextElements = root.copyObject(root.elements);
        delete nextElements[key];
        root.elements = nextElements;
        root.refreshIfTracking();
    }

    function refreshIfTracking() : var {
        if (!root.tracking) {
            return;
        }
        root.refreshVisibleState();
    }

    function clearVisibleState() : var {
        if (Object.keys(root.visibleByIndex).length === 0) {
            return;
        }
        root.visibleByIndex = {};
    }

    function elementState(element: var) : var {
        if (!element) {
            return null;
        }
        if (!root.skinRuntime || element.runtimeIndex === undefined) {
            return null;
        }

        const staticState = root.skinRuntime.staticStateForElement(element.runtimeIndex);
        if (staticState) {
            return staticState;
        }

        const descriptor = root.skinRuntime.descriptor(element.runtimeIndex);
        return root.skinRuntime.stateForElement(
            element.runtimeIndex,
            descriptor.usesLiveDstClock ? root.host.selectSourceSkinTime : root.host.renderSkinTime);
    }

    function hoverStateAt(element: var, mx: var, my: var) : var {
        if (!element || !element.src || !element.src.onMouse
                || !root.host.panelMatches(element.src.hoverPanel || 0)) {
            return null;
        }
        return root.host.onMouseStateContainsPoint(element.src, root.elementState(element), mx, my);
    }

    function refreshVisibleState() : var {
        if (!root.tracking || root.candidateKeys.length <= 0 || !root.hasPoint) {
            root.clearVisibleState();
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
            if (root.hoverStateAt(element, mx, my) !== null) {
                visible[key] = true;
                visibleKeys.push(key);
            }
        }

        const signature = visibleKeys.join(",");
        if (signature === root.visibleSignature) {
            return;
        }
        root.visibleByIndex = visible;
    }
}
