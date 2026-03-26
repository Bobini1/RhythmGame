//
// Created by PC on 03/03/2026.
//

#ifndef RHYTHMGAME_ONLINEPROFILE_H
#define RHYTHMGAME_ONLINEPROFILE_H
#include <QObject>

namespace qml_components {

class OnlineProfileInfo
{
    Q_GADGET
    Q_PROPERTY(QString username MEMBER username CONSTANT)
    Q_PROPERTY(QString avatarUrl MEMBER avatarUrl CONSTANT)
    Q_PROPERTY(QString userId MEMBER userId CONSTANT)
  public:
    QString username;
    QString avatarUrl;
    QString userId;
};
} // namespace qml_components

#endif // RHYTHMGAME_ONLINEPROFILE_H
