#include "Lr2SkinOptionRules.h"

#include <cstdlib>

Lr2SkinOptionRules::Lr2SkinOptionRules(QObject* parent)
    : QObject(parent) {}

bool Lr2SkinOptionRules::isRuntimeOwnedOption(int option) const {
    return isRuntimeOwnedOptionValue(option);
}

bool Lr2SkinOptionRules::isRuntimeOwnedOptionValue(int option) {
    const int id = std::abs(option);
    const auto inRange = [id](int first, int last) {
        return id >= first && id <= last;
    };

    if (inRange(1, 5)
        || inRange(30, 66)
        || inRange(70, 84)
        || inRange(90, 91)
        || inRange(100, 145)
        || inRange(150, 197)
        || inRange(200, 354)
        || inRange(499, 624)
        || id == 625
        || inRange(700, 755)
        || inRange(1100, 1104)
        || inRange(1196, 1208)
        || inRange(1240, 1263)
        || inRange(1330, 1336)
        || inRange(2241, 2246)) {
        return true;
    }

    switch (id) {
    case 1046:
    case 1047:
    case 1128:
    case 1160:
    case 1161:
    case 1177:
        return true;
    default:
        return false;
    }
}
