#ifndef RHYTHMGAME_DDSHANDLER_H
#define RHYTHMGAME_DDSHANDLER_H

#include <QImageIOHandler>

class DdsHandler : public QImageIOHandler
{
  public:
    bool canRead() const override;
    bool read(QImage* image) override;

    static bool canRead(QIODevice* device);
};

#endif // RHYTHMGAME_DDSHANDLER_H
