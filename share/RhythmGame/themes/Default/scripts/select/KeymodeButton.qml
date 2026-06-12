pragma ValueTypeBehavior: Addressable
import RhythmGameQml
import QtQuick 2.0

Image {
    id: keymodeButton

    required property var generalVars
    readonly property int selectedFilter: generalVars ? generalVars.selectKeymodeFilter : SelectKeymodeFilter.All
    property var options: Rg.profileList.battleActive
        ? [
            { text: QT_TR_NOOP("SINGLE"), value: SelectKeymodeFilter.Single },
            { text: QT_TR_NOOP("5 keys"), value: SelectKeymodeFilter.K5 },
            { text: QT_TR_NOOP("7 keys"), value: SelectKeymodeFilter.K7 }
        ]
        : [
            { text: QT_TR_NOOP("SINGLE"), value: SelectKeymodeFilter.Single },
            { text: QT_TR_NOOP("5 keys"), value: SelectKeymodeFilter.K5 },
            { text: QT_TR_NOOP("7 keys"), value: SelectKeymodeFilter.K7 },
            { text: QT_TR_NOOP("DOUBLE"), value: SelectKeymodeFilter.Double },
            { text: QT_TR_NOOP("10 keys"), value: SelectKeymodeFilter.K10 },
            { text: QT_TR_NOOP("14 keys"), value: SelectKeymodeFilter.K14 },
            { text: QT_TR_NOOP("ALL keys"), value: SelectKeymodeFilter.All }
        ]

    function indexForFilter(filter) {
        for (let i = 0; i < options.length; ++i) {
            if (options[i].value === filter) {
                return i;
            }
        }
        return -1;
    }

    function textForFilter(filter) {
        switch (filter) {
        case SelectKeymodeFilter.Single: return QT_TR_NOOP("SINGLE");
        case SelectKeymodeFilter.Double: return QT_TR_NOOP("DOUBLE");
        case SelectKeymodeFilter.K5: return QT_TR_NOOP("5 keys");
        case SelectKeymodeFilter.K7: return QT_TR_NOOP("7 keys");
        case SelectKeymodeFilter.K10: return QT_TR_NOOP("10 keys");
        case SelectKeymodeFilter.K14: return QT_TR_NOOP("14 keys");
        default: return QT_TR_NOOP("ALL keys");
        }
    }

    function cycle(delta) {
        let current = keymodeButton.indexForFilter(keymodeButton.selectedFilter);
        let step = delta === undefined ? 1 : delta;
        let next = current < 0 ? 0 : (current + step) % keymodeButton.options.length;
        if (next < 0) {
            next += keymodeButton.options.length;
        }
        keymodeButton.generalVars.selectKeymodeFilter = keymodeButton.options[next].value;
    }

    enabled: options.length > 1
    source: root.iniImagesUrl + "option.png/button_big"

    Text {
        anchors.centerIn: parent
        color: "black"
        font.pixelSize: 20
        text: qsTr(keymodeButton.textForFilter(keymodeButton.selectedFilter))
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: keymodeButton.cycle(1)
    }
}