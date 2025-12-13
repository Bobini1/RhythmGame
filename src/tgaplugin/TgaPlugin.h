//
// Created by PC on 13/12/2025.
//

#ifndef RHYTHMGAME_TGAPLUGIN_H
#define RHYTHMGAME_TGAPLUGIN_H

#include <QImageIOPlugin>

class TgaPlugin : public QImageIOPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID
                      "org.qt-project.Qt.QImageIOHandlerFactoryInterface" FILE
                      "tga.json")

  public:
    Capabilities capabilities(QIODevice* device,
                              const QByteArray& format) const override;
    QImageIOHandler* create(QIODevice* device,
                            const QByteArray& format) const override;
};

#endif // RHYTHMGAME_TGAPLUGIN_H
