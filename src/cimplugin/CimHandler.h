#ifndef RHYTHMGAME_CIMHANDLER_H
#define RHYTHMGAME_CIMHANDLER_H

#include <QImageIOHandler>
#include <QRect>

class CimHandler : public QImageIOHandler
{
  public:
    bool canRead() const override;
    bool read(QImage* image) override;
    QVariant option(ImageOption option) const override;
    void setOption(ImageOption option, const QVariant& value) override;
    bool supportsOption(ImageOption option) const override;

    static bool canRead(QIODevice* device);

  private:
    QRect m_clipRect;
    QRect m_scaledClipRect;
};

#endif // RHYTHMGAME_CIMHANDLER_H
