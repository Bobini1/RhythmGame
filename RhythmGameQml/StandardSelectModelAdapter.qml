import QtQuick

// Optional presentation adapter for circular lists that need enough repeated
// rows to fill every visible slot. The source remains a logical, unique list.
QtObject {
    id: root

    property var source: []
    property int minimumCount: 0
    readonly property var entries: repeatToMinimumCount(source, minimumCount)

    function repeatToMinimumCount(input, count) {
        if (!input || input.length === 0 || input.length >= count) {
            return input || [];
        }
        let result = input.slice();
        let limit = Math.ceil(count / input.length) * input.length;
        for (let i = input.length; i < limit; ++i) {
            result.push(input[i % input.length]);
        }
        return result;
    }
}
