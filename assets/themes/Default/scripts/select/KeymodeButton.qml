import RhythmGameQml
import QtQuick 2.0

Image {
    id: keymodeButton

    property int current: 1
    property var options: [null, 7, 14]

    source: root.iniImagesUrl + "option.png/button_big"

    Text {
        anchors.centerIn: parent
        color: "black"
        font.pixelSize: 20
        text: (keymodeButton.options[keymodeButton.current] ? keymodeButton.options[keymodeButton.current] : "ALL") + " keys"
    }
    MouseArea {
        function setFilter() {
            let currentKeymode = keymodeButton.options[keymodeButton.current];
            if (currentKeymode) {
                songList.filter = function (chart) {
                    return chart.keymode === currentKeymode;
                };
            } else {
                songList.filter = null;
            }
        }

        anchors.fill: parent

        Component.onCompleted: {
            setFilter();
        }
        onClicked: {
            keymodeButton.current = (keymodeButton.current + 1) % keymodeButton.options.length;
            setFilter();
        }
    }
}