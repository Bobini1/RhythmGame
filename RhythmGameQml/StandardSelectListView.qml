import QtQuick
import RhythmGameQml

/*!
    \qmltype StandardSelectListView
    \inqmlmodule RhythmGameQml
    \brief Connects a finite song list to a selection controller.

    Set controller and supply a delegate. The list handles arrows, the mouse
    wheel and bound controller navigation, folder focus restoration, and preview movement.
    Call activate(index) when a row is clicked. Return, Enter and Right open
    the current entry; Left goes back. Movement wraps at the ends.

    Style and size this item like any ListView. Keyboard focus still belongs
    to the theme: set focus on this list and its enclosing focus scopes.
    Use StandardSelectController directly for a custom wheel instead.
*/
ListView {
    id: root

    /*! Controller that supplies entries and actions for this list. */
    required property StandardSelectController controller

    model: controller.entries
    currentIndex: -1
    keyNavigationEnabled: false

    /*! Selects and opens the row at \a index, or the current row when omitted. */
    function activate(index = currentIndex) {
        if (!root.enabled || !root.controller.enabled || !root.controller.inputEnabled
                || index < 0 || index >= root.count)
            return false;
        root.forceActiveFocus();
        root.currentIndex = index;
        root.controller.setFocused(root.controller.entries[index]);
        return root.controller.goForward(root.controller.focusedItem);
    }

    onCurrentIndexChanged: {
        if (currentIndex >= 0 && currentIndex < count)
            controller.setFocused(controller.entries[currentIndex]);
    }

    Connections {
        target: root.controller
        function onFocusRequested(index) { root.currentIndex = index; }
        function onMoveRequested(steps) {
            if (root.enabled && root.count > 0)
                root.currentIndex = ((root.currentIndex + steps) % root.count + root.count) % root.count;
        }
    }

    Binding {
        target: root.controller.preview
        property: "moving"
        value: root.moving
        when: root.controller.preview !== null
    }

    WheelHandler {
        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
        enabled: root.controller.enabled && root.controller.inputEnabled
        onWheel: event => event.accepted = root.controller.handleWheel(event)
    }

    Keys.onUpPressed: event => event.accepted = controller.handleUpPressed(event)
    Keys.onDownPressed: event => event.accepted = controller.handleDownPressed(event)
    Keys.onReleased: event => event.accepted = controller.handleReleased(event)
    Keys.onReturnPressed: event => event.accepted = activate()
    Keys.onEnterPressed: event => event.accepted = activate()
    Keys.onRightPressed: event => event.accepted = activate()
    Keys.onLeftPressed: event => {
        event.accepted = controller.enabled && controller.inputEnabled && controller.goBack();
    }

    Component.onCompleted: currentIndex = controller.focusedIndex
}
