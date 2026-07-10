import QtQuick

Item {
    id: root

    required property bool customizeMode
    required property string accessibleName
    property int horizontalEdge: 0
    property int verticalEdge: 0
    readonly property bool interactionActive: resizeHandler.active

    signal interactionStarted(int horizontalEdge, int verticalEdge)
    signal interactionDelta(real x, real y, int horizontalEdge, int verticalEdge)
    signal interactionEnded()

    width: 32
    height: 32
    visible: customizeMode
    enabled: customizeMode
    z: 1001

    Accessible.role: Accessible.Grip
    Accessible.name: accessibleName

    Rectangle {
        anchors.centerIn: parent
        width: root.horizontalEdge !== 0 && root.verticalEdge !== 0 ? 12 :
               root.horizontalEdge !== 0 ? 8 : 18
        height: root.horizontalEdge !== 0 && root.verticalEdge !== 0 ? 12 :
                root.verticalEdge !== 0 ? 8 : 18
        radius: 2
        color: "#f5f7ff"
        border.width: 2
        border.color: "#181b24"
    }

    HoverHandler {
        enabled: root.enabled
        cursorShape: {
            if (root.horizontalEdge === root.verticalEdge)
                return Qt.SizeFDiagCursor;
            if (root.horizontalEdge === -root.verticalEdge)
                return Qt.SizeBDiagCursor;
            if (root.horizontalEdge !== 0)
                return Qt.SizeHorCursor;
            return Qt.SizeVerCursor;
        }
    }

    DragHandler {
        id: resizeHandler

        target: null
        enabled: root.enabled
        acceptedButtons: Qt.LeftButton

        onActiveChanged: {
            if (active) {
                root.interactionStarted(root.horizontalEdge, root.verticalEdge);
            } else {
                root.interactionEnded();
            }
        }
        onTranslationChanged: {
            if (active) {
                root.interactionDelta(translation.x, translation.y,
                                      root.horizontalEdge, root.verticalEdge);
            }
        }
    }
}
