#pragma once

class QObject;
class QQmlPropertyMap;

namespace support {

auto
createQmlPropertyMap(QObject* parent = nullptr) -> QQmlPropertyMap*;

} // namespace support
