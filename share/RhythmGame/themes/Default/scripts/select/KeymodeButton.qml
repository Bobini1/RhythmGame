pragma ValueTypeBehavior: Addressable
import RhythmGameQml
import QtQuick 2.0

Image {
    id: keymodeButton

    property int current: {
        let savedKeymode = themeVars.keymodeFilter ? parseInt(themeVars.keymodeFilter) : null;
        let index = options.indexOf(savedKeymode);
        return index === -1 ? 0 : index;
    }
    property var options: Rg.profileList.battleActive ? [7] : [7, 14, null]
    required property var themeVars

    Binding {
        delayed: true
        target: themeVars
        property: "keymodeFilter"
        when: options[current] !== undefined
        value: options[current] === null ? "" : options[current].toString()
    }

    Binding {
        keymodeButton.current: {
            let defaultKeymode = Rg.profileList.battleActive ? 7 : null;
            let savedKeymode = parseInt(themeVars.keymodeFilter);
            if (isNaN(savedKeymode)) {
                savedKeymode = defaultKeymode;
            }
            let index = options.indexOf(savedKeymode);
            return index === -1 ? 0 : index;
        }
    }

    onOptionsChanged: {
        if (current >= options.length) {
            current = 0;
        }
        mouseArea.setFilter();
    }
    enabled: options.length > 1


    source: root.iniImagesUrl + "option.png/button_big"

    Text {
        anchors.centerIn: parent
        color: "black"
        font.pixelSize: 20
        text: {
            let current = keymodeButton.options[keymodeButton.current];
            if (current === null) {
                return qsTr("ALL keys");
            }
            return qsTr("%1 keys").arg(current)
        }
    }
    MouseArea {
        id: mouseArea
        function setFilter() {
            let currentKeymode = keymodeButton.options[keymodeButton.current];
            if (currentKeymode) {
                songList.filter = function (chart) {
                    return chart.keymode === currentKeymode || chart instanceof entry
                };
            } else {
                songList.filter = null;
            }
        }

        anchors.fill: parent

        cursorShape: Qt.PointingHandCursor

        Component.onCompleted: {
            setFilter();
        }
        onClicked: {
            keymodeButton.current = (keymodeButton.current + 1) % keymodeButton.options.length;
            setFilter();
        }
    }
}