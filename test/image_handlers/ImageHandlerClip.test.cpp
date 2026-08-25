#include "cimplugin/CimHandler.h"
#include "ddsplugin/DdsHandler.h"
#include "tgaplugin/TgaHandler.h"

#include <catch2/catch_test_macros.hpp>

#include <QBuffer>
#include <QImage>
#include <QRect>

namespace {

const QByteArray ddsFixture = QByteArray::fromBase64(
  "RERTIHwAAAAHEAoABAAAAAQAAAAQAAAAAAAAAAMAAABJTUFHRU1BR0lDSwAAAAAAAAAA"
  "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAACAAAAAEAAAARFhUNQAAAAAAAAAAAAAAAAAA"
  "AAAAAAAACBBAAAAAAAAAAAAAAAAAAAAAAAAABf//////////APj19QoKAAU/8AMAAADb"
  "viS5raKqqgAFBwAAAAAA+98KUquqqqo=");

const QByteArray tgaFixture = QByteArray::fromBase64(
  "AAACAAAAAAAAAAAABAAEACAoAAD//wAA//8A/wD/AP8A/wAA//8AAP//AP8A/wD/AP//"
  "AAD//wAA/////////////wAA//8AAP///////////w==");

const QByteArray cimFixture =
  QByteArray::fromBase64("eJxjYGBgYYDi/wwM/0EYBtH5UC4coPMBZ1wn5Q==");

template<typename Handler>
QImage
readFixture(const QByteArray& fixture,
            const QImageIOHandler::ImageOption option,
            const QRect& clipRect = {})
{
    QBuffer buffer;
    buffer.setData(fixture);
    if (!buffer.open(QIODevice::ReadOnly)) {
        return {};
    }

    Handler handler;
    handler.setDevice(&buffer);
    if (clipRect.isValid()) {
        handler.setOption(option, clipRect);
    }

    QImage image;
    if (!handler.read(&image)) {
        return {};
    }
    return image;
}

template<typename Handler>
void
checkClipOptions(const QByteArray& fixture)
{
    const QImage full =
      readFixture<Handler>(fixture, QImageIOHandler::ClipRect);
    REQUIRE(full.size() == QSize(4, 4));

    const QRect clipRect(2, 0, 2, 2);
    const QImage clipped =
      readFixture<Handler>(fixture, QImageIOHandler::ClipRect, clipRect);
    const QImage scaledClipped =
      readFixture<Handler>(fixture, QImageIOHandler::ScaledClipRect, clipRect);

    REQUIRE(clipped.size() == clipRect.size());
    CHECK(clipped == full.copy(clipRect));
    REQUIRE(scaledClipped.size() == clipRect.size());
    CHECK(scaledClipped == full.copy(clipRect));
}

} // namespace

TEST_CASE("DDS decoder honors source clip rectangles", "[image][dds][lr2]")
{
    checkClipOptions<DdsHandler>(ddsFixture);
}

TEST_CASE("TGA decoder honors source clip rectangles", "[image][tga][lr2]")
{
    checkClipOptions<TgaHandler>(tgaFixture);
}

TEST_CASE("CIM decoder honors source clip rectangles", "[image][cim][lr2]")
{
    checkClipOptions<CimHandler>(cimFixture);
}
