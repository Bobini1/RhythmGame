//
// Created by PC on 17/03/2026.
//

#include "OnlineLinks.h"

namespace qml_components {

OnlineLinks::OnlineLinks(QObject* parent)
  : QObject(parent)
{
}
QString
OnlineLinks::profile(QString websiteUrl, int profileId)
{
    return websiteUrl + "/players/" + QString::number(profileId);
}
QString
OnlineLinks::chart(QString websiteUrl, QString md5)
{
    return websiteUrl + "/chart/" + md5;
}
QString
OnlineLinks::scoresByUserOnChart(QString websiteUrl, int profileId, QString md5)
{
    return websiteUrl + "/chart/" + md5 + "/players/" +
           QString::number(profileId);
}
} // namespace qml_components