#ifndef RHYTHMGAME_CIMHANDLER_H
#define RHYTHMGAME_CIMHANDLER_H

#include <QImageIOHandler>

class CimHandler : public QImageIOHandler
{
  public:
    bool canRead() const override;
    bool read(QImage* image) override;

    static bool canRead(QIODevice* device);
};

#endif // RHYTHMGAME_CIMHANDLER_H
