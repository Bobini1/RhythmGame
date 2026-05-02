.pragma library

let textCache = {};
let textSize = 0;
let numberCache = {};
let numberSize = 0;
const maxSize = 4096;

function getText(key) {
    return textCache[key];
}

function putText(key, value) {
    if (textSize > maxSize) {
        textCache = {};
        textSize = 0;
    }
    textCache[key] = value;
    ++textSize;
    return value;
}

function getNumber(key) {
    return numberCache[key];
}

function putNumber(key, value) {
    if (numberSize > maxSize) {
        numberCache = {};
        numberSize = 0;
    }
    numberCache[key] = value;
    ++numberSize;
    return value;
}
