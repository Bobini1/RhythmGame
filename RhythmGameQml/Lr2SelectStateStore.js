.pragma library

let nextId = 1;
let states = {};

function allocate() {
    return nextId++;
}

function setState(id, state) {
    states[id] = state;
}

function state(id) {
    return states[id] || null;
}

function clear(id) {
    delete states[id];
}
