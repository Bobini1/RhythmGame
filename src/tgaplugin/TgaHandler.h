//
// Created by PC on 13/12/2025.
//

#ifndef TGAHANDLER_H
#define TGAHANDLER_H

#include <QImageIOHandler>

class TgaHandler : public QImageIOHandler
{
  public:
    bool canRead() const override;
    bool read(QImage* image) override;

    static bool canRead(QIODevice* device);
};

#endif