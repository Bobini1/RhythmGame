import QtQuick
import RhythmGameQml

Item {
    id: root

    required property var members
    property real avatarSize: 24
    property string objectNamePrefix: "arenaRoomAvatar-"
    readonly property int previewCount: Math.min(4, root.members ? root.members.length : 0)
    readonly property int overflowCount: Math.max(0, (root.members ? root.members.length : 0) - root.previewCount)
    readonly property string memberNames: root.memberNamesFor(root.members)

    function memberNamesFor(memberList): string {
        if (!memberList) {
            return "";
        }
        const names = [];
        for (let index = 0; index < memberList.length; ++index) {
            names.push(String(memberList[index].displayName || ""));
        }
        return names.filter(name => name.length > 0).join(", ");
    }

    implicitHeight: root.avatarSize
    implicitWidth: previewRow.implicitWidth

    Row {
        id: previewRow

        anchors.fill: parent
        spacing: -6

        Repeater {
            model: root.previewCount

            delegate: ArenaAvatar {
                id: avatarDelegate

                required property int index
                readonly property var member: root.members[avatarDelegate.index]

                objectName: root.objectNamePrefix + avatarDelegate.index
                avatarUrl: String(avatarDelegate.member.avatarUrl || "")
                connected: avatarDelegate.member.connected !== false
                displayName: String(avatarDelegate.member.displayName || "")
                height: root.avatarSize
                width: root.avatarSize
                z: avatarDelegate.index
            }
        }

        Item {
            height: root.avatarSize
            visible: root.overflowCount > 0
            width: visible ? root.avatarSize + 8 : 0

            Rectangle {
                anchors.fill: parent
                border.color: "#7f93aa"
                border.width: 1
                color: "#283444"
                radius: height / 2
            }

            Text {
                objectName: root.objectNamePrefix + "overflow"
                Accessible.ignored: true
                anchors.centerIn: parent
                color: "white"
                font.bold: true
                font.pixelSize: 11
                text: "+" + root.overflowCount
                textFormat: Text.PlainText
            }
        }
    }
}
