import QtQuick
import RhythmGameQml

Text {
    id: root

    text: {
        if (Rg.arenaRoomCount < 0) {
            return qsTr("Arena: unavailable");
        }
        const count = Rg.arenaRoomCount;
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
