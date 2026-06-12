#include "CimPlugin.h"
#include "CimHandler.h"

QImageIOPlugin::Capabilities
CimPlugin::capabilities(QIODevice* device, const QByteArray& format) const
{
    const auto lowerFormat = format.toLower();
    if (lowerFormat == "cim") {
        return CanRead;
    }
    if (!format.isEmpty()) {
        return {};
    }
    if (device && CimHandler::canRead(device)) {
        return CanRead;
    }
    return {};
}

QImageIOHandler*
CimPlugin::create(QIODevice* device, const QByteArray& format) const
{
    auto* handler = new CimHandler;
    handler->setDevice(device);
    handler->setFormat(format);
    return handler;
}
