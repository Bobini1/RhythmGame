//
// Created by PC on 13/12/2025.
//

#include "TgaHandler.h"
#include <SDL2/SDL_image.h>
#include <QIODevice>

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
    // TGA files often start with specific header bytes
    // Check first few bytes without consuming them
    QByteArray header = device->peek(18);
    return header.size() >= 18; // Basic TGA header size check
}

bool
TgaHandler::read(QImage* image)
{
    QByteArray data = device()->readAll();

    SDL_RWops* rw = SDL_RWFromConstMem(data.constData(), data.size());
    if (!rw)
        return false;

    SDL_Surface* surface = IMG_LoadTGA_RW(rw);
    SDL_RWclose(rw);

    if (!surface)
        return false;

    QImage::Format format;
    if (surface->format->BitsPerPixel == 32) {
        format = QImage::Format_RGBA8888;
    } else if (surface->format->BitsPerPixel == 24) {
        format = QImage::Format_RGB888;
    } else {
        SDL_FreeSurface(surface);
        return false;
    }

    *image = QImage(surface->w, surface->h, format);
    std::memcpy(
      image->bits(), surface->pixels, (size_t)surface->h * surface->pitch);
    SDL_FreeSurface(surface);

    return true;
}
