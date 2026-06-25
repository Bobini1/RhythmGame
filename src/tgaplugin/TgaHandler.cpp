//
// Created by PC on 13/12/2025.
//

#include "TgaHandler.h"
#include <SDL2/SDL_image.h>
#include <QIODevice>
#include <algorithm>
#include <cstring>

bool
TgaHandler::canRead() const
{
    return canRead(device());
}

bool
TgaHandler::canRead(QIODevice* device)
{
    if (!device)
        return false;
    QByteArray header = device->peek(18);
    if (header.size() < 18) {
        return false;
    }

    const auto byteAt = [&header](int index) {
        return static_cast<unsigned char>(header[index]);
    };
    const auto littleEndian16 = [&byteAt](int index) {
        return static_cast<quint16>(byteAt(index) | (byteAt(index + 1) << 8));
    };

    const auto colorMapType = byteAt(1);
    if (colorMapType > 1) {
        return false;
    }

    const auto imageType = byteAt(2);
    if (imageType != 1 && imageType != 2 && imageType != 3 && imageType != 9 &&
        imageType != 10 && imageType != 11) {
        return false;
    }

    const auto width = littleEndian16(12);
    const auto height = littleEndian16(14);
    const auto bitsPerPixel = byteAt(16);
    return width > 0 && height > 0 &&
           (bitsPerPixel == 8 || bitsPerPixel == 15 || bitsPerPixel == 16 ||
            bitsPerPixel == 24 || bitsPerPixel == 32);
}

bool
TgaHandler::read(QImage* image)
{
    if (!image || !device()) {
        return false;
    }

    QByteArray data = device()->readAll();

    SDL_RWops* rw = SDL_RWFromConstMem(data.constData(), data.size());
    if (!rw)
        return false;

    SDL_Surface* surface = IMG_LoadTGA_RW(rw);
    SDL_RWclose(rw);

    if (!surface)
        return false;

    QImage::Format format;
    Uint32 sdlFormat;
    if (surface->format->BitsPerPixel == 32) {
        format = QImage::Format_RGBA8888;
        sdlFormat = SDL_PIXELFORMAT_RGBA32;
    } else if (surface->format->BitsPerPixel == 24) {
        format = QImage::Format_RGB888;
        sdlFormat = SDL_PIXELFORMAT_RGB24;
    } else {
        SDL_FreeSurface(surface);
        return false;
    }

    SDL_Surface* converted = SDL_ConvertSurfaceFormat(surface, sdlFormat, 0);
    SDL_FreeSurface(surface);
    if (!converted)
        return false;

    QImage result(converted->w, converted->h, format);
    if (result.isNull()) {
        SDL_FreeSurface(converted);
        return false;
    }

    const auto* sourcePixels =
      static_cast<const unsigned char*>(converted->pixels);
    for (int y = 0; y < converted->h; ++y) {
        const auto bytesToCopy = static_cast<std::size_t>(std::min<qsizetype>(
          result.bytesPerLine(), static_cast<qsizetype>(converted->pitch)));
        std::memcpy(
          result.scanLine(y), sourcePixels + y * converted->pitch, bytesToCopy);
    }

    *image = result;
    SDL_FreeSurface(converted);

    return true;
}
