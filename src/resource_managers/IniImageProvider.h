//
// Created by bobini on 04.09.23.
//

#ifndef RHYTHMGAME_INIIMAGEPROVIDER_H
#define RHYTHMGAME_INIIMAGEPROVIDER_H

#include <QQuickImageProvider>
namespace resource_managers {

class IniImageProvider final : public QQuickImageProvider
{
    QHash<QString, QHash<QRect, QPixmap>> pixmaps;

  public:
    IniImageProvider();
    auto requestPixmap(const QString& id,
                       QSize* size,
                       const QSize& requestedSize) -> QPixmap override;
};

} // namespace resource_managers

#endif // RHYTHMGAME_INIIMAGEPROVIDER_H
