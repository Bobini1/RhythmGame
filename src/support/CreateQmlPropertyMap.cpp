#include "CreateQmlPropertyMap.h"

#include <QQmlPropertyMap>
#include <QtGlobal>

namespace support {

auto
createQmlPropertyMap(QObject* parent) -> QQmlPropertyMap*
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 11, 0)
    return QQmlPropertyMap::create(parent);
#else
    return new QQmlPropertyMap(parent);
#endif
}

} // namespace support
