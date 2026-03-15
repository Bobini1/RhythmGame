pragma ValueTypeBehavior: Addressable
import RhythmGameQml
import QtQuick 2.0

Image {
    id: keymodeButton

    property int current: {
        let savedKeymode = themeVars.keymodeFilter;
        let index = options.indexOf(savedKeymode);
        return index === -1 ? 2 : index;
    }
    property var options: Rg.profileList.battleActive ? ["5", "7", "SINGLE"] : ["5", "7", "SINGLE", "10", "14", "DOUBLE", null]
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
            let defaultKeymode = Rg.profileList.battleActive ? "SINGLE" : null;
            let savedKeymode = themeVars.keymodeFilter;
            if (savedKeymode === undefined) {
                savedKeymode = defaultKeymode;
            }
            let index = options.indexOf(savedKeymode);
            return index === -1 ? 2 : index;
        }
    }

    onOptionsChanged: {
        if (current >= options.length) {
            current = 2;
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
            if (current === "SINGLE") {
                return qsTr("SINGLE");
            }
            if (current === "DOUBLE") {
                return qsTr("DOUBLE");
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
                    if (chart instanceof entry) return true;
                    switch (currentKeymode) {
                        case "SINGLE":
                            return chart.keymode === 5 || chart.keymode === 7;
                        case "DOUBLE":
                            return chart.keymode === 10 || chart.keymode === 14;
                        default:
                            return chart.keymode === parseInt(currentKeymode);
                    }
                }
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