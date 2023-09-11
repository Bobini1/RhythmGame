//
// Created by bobini on 14.08.23.
//

#ifndef RHYTHMGAME_CHARTDATA_H
#define RHYTHMGAME_CHARTDATA_H

#include <QObject>
#include <QtQmlIntegration>
#include <QQmlEngine>
#include <QJSEngine>
#include "BmsScore.h"
#include "BmsNotes.h"
namespace gameplay_logic {

class ChartData : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString title READ getTitle CONSTANT)
    Q_PROPERTY(QString artist READ getArtist CONSTANT)
    Q_PROPERTY(QString subtitle READ getSubtitle CONSTANT)
    Q_PROPERTY(QString subartist READ getSubartist CONSTANT)
    Q_PROPERTY(QString genre READ getGenre CONSTANT)
    Q_PROPERTY(int rank READ getRank CONSTANT)
    Q_PROPERTY(double total READ getTotal CONSTANT)
    Q_PROPERTY(int playLevel READ getPlayLevel CONSTANT)
    Q_PROPERTY(int difficulty READ getDifficulty CONSTANT)
    Q_PROPERTY(int noteCount READ getNoteCount CONSTANT)
    Q_PROPERTY(int length READ getLength CONSTANT)
    Q_PROPERTY(QUrl directory READ getDirectory CONSTANT)
    Q_PROPERTY(BmsNotes* noteData READ getNoteData CONSTANT)

  public:
    ChartData(QString title,
              QString artist,
              QString subtitle,
              QString subartist,
              QString genre,
              int rank,
              double total,
              int playLevel,
              int difficulty,
              int noteCount,
              int length,
              QUrl directory,
              BmsNotes* noteData,
              QObject* parent = nullptr);

    [[nodiscard]] auto getTitle() const -> QString;
    [[nodiscard]] auto getArtist() const -> QString;
    [[nodiscard]] auto getSubtitle() const -> QString;
    [[nodiscard]] auto getSubartist() const -> QString;
    [[nodiscard]] auto getGenre() const -> QString;
    [[nodiscard]] auto getNoteCount() const -> int;
    [[nodiscard]] auto getLength() const -> int;
    [[nodiscard]] auto getDirectory() const -> QUrl;
    [[nodiscard]] auto getNoteData() const -> BmsNotes*;
    [[nodiscard]] auto getRank() const -> int;
    [[nodiscard]] auto getTotal() const -> double;
    [[nodiscard]] auto getPlayLevel() const -> int;
    [[nodiscard]] auto getDifficulty() const -> int;

  private:
    QString title;
    QString artist;
    QString level;
    QString subtitle;
    QString subartist;
    QString genre;
    int rank;
    double total;
    int playLevel;
    int difficulty;
    int noteCount;
    int length;
    QUrl directory;
    BmsNotes* noteData;
};

} // namespace gameplay_logic

#endif // RHYTHMGAME_CHARTDATA_H
