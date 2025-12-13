//
// Created by PC on 13/12/2025.
//

#include "TgaPlugin.h"
#include "TgaHandler.h"

QImageIOPlugin::Capabilities
TgaPlugin::capabilities(QIODevice* device, const QByteArray& format) const
{
    if (format == "tga") {
        return CanRead;
    }
    if (!format.isEmpty()) {
        return {};
    }
    if (device && TgaHandler::canRead(device)) {
        return CanRead;
    }
    return {};
}

QImageIOHandler*
TgaPlugin::create(QIODevice* device, const QByteArray& format) const
{
    auto* handler = new TgaHandler;
    handler->setDevice(device);
    handler->setFormat(format);
    return handler;
}