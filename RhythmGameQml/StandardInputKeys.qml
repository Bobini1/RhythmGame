pragma Singleton

import QtQml

QtObject {
    function isPlayKey(key): bool {
        return (key >= BmsKey.Col11 && key <= BmsKey.Col17)
            || (key >= BmsKey.Col21 && key <= BmsKey.Col27);
    }
}
