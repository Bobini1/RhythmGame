//
// Created by PC on 24/03/2026.
//

#include "ConvertTachiClearType.h"
QString
support::convertTachiClearType(const int enumIndex)
{
    switch (enumIndex) {
        case 0:
            return "NOPLAY";
        case 1:
            return "FAILED";
        case 2:
            return "AEASY";
        case 3:
            return "EASY";
        case 4:
            return "NORMAL";
        case 5:
            return "HARD";
        case 6:
            return "EXHARD";
        case 7:
            return "FC";
        default:
            return QString::number(enumIndex);
    }
}