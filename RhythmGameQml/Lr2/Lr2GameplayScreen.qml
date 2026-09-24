import QtQuick

Lr2SkinScreenWrapper {
    id: root
    required gameplay
    screenKey: root.gameplay
        ? "k" + root.gameplay.keymode
            + (root.gameplay.players.length === 2 ? "battle" : "")
        : ""
}
