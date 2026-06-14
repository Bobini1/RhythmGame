#include "DdsPlugin.h"
#include "DdsHandler.h"

QImageIOPlugin::Capabilities
DdsPlugin::capabilities(QIODevice* device, const QByteArray& format) const
{
    const auto lowerFormat = format.toLower();
    if (lowerFormat == "dds") {
        return CanRead;
    }
    if (!format.isEmpty()) {
        return {};
    }
    if (device && DdsHandler::canRead(device)) {
        return CanRead;
    }
    return {};
}

QImageIOHandler*
DdsPlugin::create(QIODevice* device, const QByteArray& format) const
{
    auto* handler = new DdsHandler;
    handler->setDevice(device);
    handler->setFormat(format);
    return handler;
}
