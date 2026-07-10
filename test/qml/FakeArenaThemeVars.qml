import QtQuick

QtObject {
    id: root

    property var arenaOverlayK7XNormalized: -1
    property var arenaOverlayK7YNormalized: -1
    property var arenaOverlayK7WidthNormalized: -1
    property var arenaOverlayK7HeightNormalized: -1
    property var arenaOverlayResultXNormalized: -1
    property var arenaOverlayResultYNormalized: -1
    property var arenaOverlayResultWidthNormalized: -1
    property var arenaOverlayResultHeightNormalized: -1
    property int writeCount: 0
    property int commitCount: 0
    property bool trackWrites: false

    onArenaOverlayK7XNormalizedChanged: {
        if (trackWrites)
            writeCount += 1;
    }
    onArenaOverlayK7YNormalizedChanged: {
        if (trackWrites)
            writeCount += 1;
    }
    onArenaOverlayK7WidthNormalizedChanged: {
        if (trackWrites)
            writeCount += 1;
    }
    onArenaOverlayK7HeightNormalizedChanged: {
        if (trackWrites)
            writeCount += 1;
    }
    onArenaOverlayResultXNormalizedChanged: {
        if (trackWrites)
            writeCount += 1;
    }
    onArenaOverlayResultYNormalizedChanged: {
        if (trackWrites)
            writeCount += 1;
    }
    onArenaOverlayResultWidthNormalizedChanged: {
        if (trackWrites)
            writeCount += 1;
    }
    onArenaOverlayResultHeightNormalizedChanged: {
        if (trackWrites)
            writeCount += 1;
    }

    function beginTracking() {
        writeCount = 0;
        commitCount = 0;
        trackWrites = true;
    }
}
