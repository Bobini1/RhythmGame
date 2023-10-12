//
// Created by bobini on 30.09.23.
//

#ifndef RHYTHMGAME_COMPRESS_H
#define RHYTHMGAME_COMPRESS_H

#include <QByteArray>
namespace support {

auto
compress(QByteArray data) -> QByteArray;
auto
decompress(QByteArray data) -> QByteArray;

} // namespace support

#endif // RHYTHMGAME_COMPRESS_H
