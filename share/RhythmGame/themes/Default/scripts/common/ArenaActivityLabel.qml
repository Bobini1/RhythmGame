import QtQuick
import RhythmGameQml

Text {
    id: root

    readonly property var session: Rg.arenaSession.active
        ? Rg.arenaSession : Rg.arenaDirectorySession

    text: {
        if (!root.session.directoryReady
                || (root.session.state !== ArenaSession.Browsing
                    && root.session.state !== ArenaSession.InRoom)) {
            return qsTr("Arena: unavailable");
        }
        const count = root.session.rooms.count;
        return count === 1 ? qsTr("Arena: 1 room online")
            : qsTr("Arena: %1 rooms online").arg(count.toLocaleString(Qt.locale(), "f", 0));
    }
    color: "#202020"
    elide: Text.ElideRight
    maximumLineCount: 1
    verticalAlignment: Text.AlignVCenter
    Accessible.role: Accessible.StaticText
    Accessible.name: text
}
