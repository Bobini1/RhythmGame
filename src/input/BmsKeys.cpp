//
// Created by bobini on 22.06.23.
//

#include "BmsKeys.h"
auto
input::playerIndexFromKey(BmsKey key) -> int
{
    if (key == BmsKey::Col11 || key == BmsKey::Col12 || key == BmsKey::Col13 ||
        key == BmsKey::Col14 || key == BmsKey::Col15 || key == BmsKey::Col16 ||
        key == BmsKey::Col17 || key == BmsKey::Col1sUp ||
        key == BmsKey::Col1sDown || key == BmsKey::Start1 ||
        key == BmsKey::Select1) {
        return 0;
    }
    if (key == BmsKey::Col21 || key == BmsKey::Col22 || key == BmsKey::Col23 ||
        key == BmsKey::Col24 || key == BmsKey::Col25 || key == BmsKey::Col26 ||
        key == BmsKey::Col27 || key == BmsKey::Col2sUp ||
        key == BmsKey::Col2sDown || key == BmsKey::Start2 ||
        key == BmsKey::Select2) {
        return 1;
    }
    return -1;
}
auto
input::convertToP1Key(BmsKey key) -> BmsKey
{
    if (key >= BmsKey::Col21 && key <= BmsKey::Col27) {
        return static_cast<BmsKey>(static_cast<int>(key) -
                                   static_cast<int>(BmsKey::Col21));
    }
    if (key == BmsKey::Col2sUp) {
        return BmsKey::Col1sUp;
    }
    if (key == BmsKey::Col2sDown) {
        return BmsKey::Col1sDown;
    }
    if (key == BmsKey::Start2) {
        return BmsKey::Start1;
    }
    if (key == BmsKey::Select2) {
        return BmsKey::Select1;
    }
    return key;
}