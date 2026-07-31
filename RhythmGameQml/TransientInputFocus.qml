pragma Singleton

import QtQuick

QtObject {
    id: root

    property Item editor: null
    property Item fallbackItem: null

    function activate(editorItem: Item, fallbackItem: Item): void {
        root.editor = editorItem;
        root.fallbackItem = fallbackItem;
    }

    function release(editorItem: Item): void {
        if (root.editor !== editorItem) {
            return;
        }
        root.editor = null;
        root.fallbackItem = null;
    }

    function dismiss(editorItem: Item): void {
        if (root.editor !== editorItem || !editorItem.activeFocus) {
            return;
        }
        const nextFocusItem = root.fallbackItem;
        root.editor = null;
        root.fallbackItem = null;
        editorItem.focus = false;
        if (nextFocusItem) {
            nextFocusItem.forceActiveFocus();
        }
    }
}
