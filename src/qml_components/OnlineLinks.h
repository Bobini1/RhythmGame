//
// Created by PC on 17/03/2026.
//

#ifndef RHYTHMGAME_ONLINELINKS_H
#define RHYTHMGAME_ONLINELINKS_H

#include <QObject>

namespace qml_components {

/**
 * @brief Provides methods to create links to online content related to the
 * game.
 * @details If URLs ever change, this class will return new URLs.
 *
 */
class OnlineLinks : public QObject
{
    Q_OBJECT
  public:
    explicit OnlineLinks(QObject* parent = nullptr);
    Q_INVOKABLE QString profile(QString websiteUrl, int profileId);
    Q_INVOKABLE QString chart(QString websiteUrl, QString md5);
    Q_INVOKABLE QString scoresByUserOnChart(QString websiteUrl,
                                            int profileId,
                                            QString md5);
};

} // namespace qml_components

#endif // RHYTHMGAME_ONLINELINKS_H
