#include "Lr2SkinOptionRules.h"

#include <cstdlib>

Lr2SkinOptionRules::Lr2SkinOptionRules(QObject* parent)
    : QObject(parent) {}

bool Lr2SkinOptionRules::isRuntimeOwnedOption(int option) const {
    return isRuntimeOwnedOptionValue(option);
}

bool Lr2SkinOptionRules::isRuntimeOwnedOptionValue(int option) {
    switch (std::abs(option)) {
    case 1:
    case 2:
    case 3:
    case 5:
    case 30:
    case 31:
    case 32:
    case 33:
    case 34:
    case 35:
    case 36:
    case 37:
    case 38:
    case 39:
    case 40:
    case 41:
    case 42:
    case 43:
    case 44:
    case 45:
    case 46:
    case 47:
    case 50:
    case 51:
    case 52:
    case 53:
    case 54:
    case 55:
    case 56:
    case 57:
    case 60:
    case 61:
    case 62:
    case 63:
    case 64:
    case 65:
    case 66:
    case 80:
    case 81:
    case 82:
    case 83:
    case 84:
    case 160:
    case 161:
    case 162:
    case 163:
    case 164:
    case 165:
    case 166:
    case 167:
    case 168:
    case 169:
    case 180:
    case 181:
    case 182:
    case 183:
    case 184:
    case 196:
    case 197:
    case 198:
    case 1196:
    case 1197:
    case 1198:
    case 1199:
    case 1200:
    case 1201:
    case 1202:
    case 1203:
    case 1204:
    case 1205:
    case 1206:
    case 1207:
    case 1208:
    case 1008:
    case 1046:
    case 1047:
    case 1160:
    case 1161:
        return true;
    default:
        return false;
    }
}
