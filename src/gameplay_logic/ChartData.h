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

    // use constant properties
    Q_PROPERTY(QString title READ getTitle CONSTANT)
    Q_PROPERTY(QString artist READ getArtist CONSTANT)
    Q_PROPERTY(QString level READ getLevel CONSTANT)
    Q_PROPERTY(int noteCount READ getNoteCount CONSTANT)
    Q_PROPERTY(int length READ getLength CONSTANT)
    Q_PROPERTY(QUrl directory READ getDirectory CONSTANT)
    Q_PROPERTY(BmsNotes* noteData READ getNoteData CONSTANT)

  public:
    ChartData(QString title,
              QString artist,
              QString level,
              int noteCount,
              int length,
              QUrl directory,
              BmsNotes* noteData,
              QObject* parent = nullptr);

    [[nodiscard]] auto getTitle() const -> QString;
    [[nodiscard]] auto getArtist() const -> QString;
    [[nodiscard]] auto getLevel() const -> QString;
    [[nodiscard]] auto getNoteCount() const -> int;
    [[nodiscard]] auto getLength() const -> int;
    [[nodiscard]] auto getDirectory() const -> QUrl;
    [[nodiscard]] auto getNoteData() const -> BmsNotes*;

    [[nodiscard]] auto createEmptyScore() const -> BmsScore*;

  private:
    QString title;
    QString artist;
    QString level;
    int noteCount;
    int length;
    QUrl directory;
    BmsNotes* noteData;
};

} // namespace gameplay_logic

#endif // RHYTHMGAME_CHARTDATA_H
