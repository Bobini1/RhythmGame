#ifndef RHYTHMGAME_DDSPLUGIN_H
#define RHYTHMGAME_DDSPLUGIN_H

#include <QImageIOPlugin>

class DdsPlugin : public QImageIOPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID
                      "org.qt-project.Qt.QImageIOHandlerFactoryInterface" FILE
                      "dds.json")

  public:
    Capabilities capabilities(QIODevice* device,
                              const QByteArray& format) const override;
    QImageIOHandler* create(QIODevice* device,
                            const QByteArray& format) const override;
};

#endif // RHYTHMGAME_DDSPLUGIN_H
