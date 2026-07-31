import QtQuick

Item {
    id: root

    anchors.fill: parent
    z: 3000000

    PointHandler {
        acceptedButtons: Qt.LeftButton

        onActiveChanged: {
            if (!active) {
                return;
            }

            const editor = TransientInputFocus.editor;
            if (!editor || !editor.activeFocus) {
                return;
            }
            const editorPoint = editor.mapFromItem(
                root,
                point.position.x,
                point.position.y);
            if (editorPoint.x < 0 || editorPoint.x > editor.width
                    || editorPoint.y < 0 || editorPoint.y > editor.height) {
                TransientInputFocus.dismiss(editor);
            }
        }
    }
}
