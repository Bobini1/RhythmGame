.pragma library

let cache = {};
let size = 0;
const maxSize = 1024;

function get(key) {
    return cache[key];
}

function put(key, value) {
    if (size > maxSize) {
        cache = {};
        size = 0;
    }
    cache[key] = value;
    ++size;
    return value;
}
