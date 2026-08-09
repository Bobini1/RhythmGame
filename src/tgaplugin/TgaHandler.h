//
// Created by PC on 13/12/2025.
//

#ifndef TGAHANDLER_H
#define TGAHANDLER_H

#include <QImageIOHandler>
#include <QRect>

class TgaHandler : public QImageIOHandler
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

#endif
