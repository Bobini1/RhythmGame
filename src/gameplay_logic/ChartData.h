//
// Created by bobini on 14.08.23.
//

#ifndef RHYTHMGAME_CHARTDATA_H
#define RHYTHMGAME_CHARTDATA_H

#include <QObject>
#include <QtQmlIntegration>
#include <QQmlEngine>
#include <QJSEngine>
namespace gameplay_logic {

class ChartData : public QObject
{
    Q_OBJECT

    // use constant properties
    Q_PROPERTY(QString title READ getTitle CONSTANT)
    Q_PROPERTY(QString artist READ getArtist CONSTANT)
    Q_PROPERTY(QString level READ getLevel CONSTANT)

  public:
    ChartData(QString title,
              QString artist,
              QString level,
              QObject* parent = nullptr);

    [[nodiscard]] auto getTitle() const -> QString;
    [[nodiscard]] auto getArtist() const -> QString;
    [[nodiscard]] auto getLevel() const -> QString;

  private:
    QString title;
    QString artist;
    QString level;
};

} // namespace gameplay_logic

#endif // RHYTHMGAME_CHARTDATA_H
