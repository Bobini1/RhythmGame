import QtQuick

Row {
    id: textureText

    property real number: 0.0
    property string srcAfterDecimal: ""
    property string srcBeforeDecimal: ""

    spacing: 0

    Repeater {
        id: beforeDecimal

        model: {
            let numberString = number.toString();
            let decimalIndex = numberString.indexOf(".");
            let beforeDecimal = decimalIndex === -1 ? numberString : numberString.substring(0, decimalIndex);
            let beforeDecimalArray = beforeDecimal.split("");
            return beforeDecimalArray;
        }

        Image {
            source: textureText.srcBeforeDecimal + modelData
        }
    }
    Repeater {
        id: afterDecimal

        model: {
            let numberString = number.toString();
            let decimalIndex = numberString.indexOf(".");
            if (decimalIndex === -1) {
                return [];
            }
            let afterDecimal = numberString.substring(decimalIndex + 1);
            let afterDecimalArray = afterDecimal.split("");
            // add "dot" at the beginning
            afterDecimalArray.unshift(".");
            return afterDecimalArray;
        }

        Image {
            source: textureText.srcAfterDecimal + modelData
        }
    }
}