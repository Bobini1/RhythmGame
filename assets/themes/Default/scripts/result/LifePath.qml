import QtQuick.Shapes
import QtQuick
import QtQml.Models

Shape {
    id: shape

    required property var history
    required property double maxGauge
    required property var songLength

    ShapePath {
        id: path

        fillColor: "transparent"
        joinStyle: ShapePath.MiterJoin
        miterLimit: 10
        startX: (1E9 + shape.history[0].offsetFromStart) / (4E9 + shape.songLength) * shape.width
        startY: shape.height - shape.history[0].gauge / shape.maxGauge * shape.height
        strokeColor: "red"
        strokeWidth: 4
    }
    Instantiator {
        model: {
            let array = shape.history;
            let arrayDuplicated = [];
            let prevElem = array[0];
            for (let elem of array.slice(1)) {
                arrayDuplicated.push({
                        "gauge": prevElem.gauge,
                        "offsetFromStart": elem.offsetFromStart
                    });
                arrayDuplicated.push(elem);
                prevElem = elem;
            }
            arrayDuplicated.push({
                    "gauge": shape.history[shape.history.length - 1].gauge,
                    "offsetFromStart": 3E9 + shape.songLength
                });
            return arrayDuplicated;
        }

        onObjectAdded: (index, object) => {
            path.pathElements.push(object);
        }

        PathLine {
            x: (1E9 + modelData.offsetFromStart) / (4E9 + shape.songLength) * shape.width
            y: shape.height - modelData.gauge / shape.maxGauge * shape.height
        }
    }
}