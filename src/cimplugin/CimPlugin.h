#ifndef RHYTHMGAME_CIMPLUGIN_H
#define RHYTHMGAME_CIMPLUGIN_H

#include <QImageIOPlugin>

class CimPlugin : public QImageIOPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID
                      "org.qt-project.Qt.QImageIOHandlerFactoryInterface" FILE
                      "cim.json")

  public:
    Capabilities capabilities(QIODevice* device,
                              const QByteArray& format) const override;
    QImageIOHandler* create(QIODevice* device,
                            const QByteArray& format) const override;
};

#endif // RHYTHMGAME_CIMPLUGIN_H
