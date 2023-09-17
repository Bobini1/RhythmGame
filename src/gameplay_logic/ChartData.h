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
#include "db/SqliteCppDb.h"
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
    Q_PROPERTY(int64_t length READ getLength CONSTANT)
    Q_PROPERTY(QString path READ getPath CONSTANT)
    Q_PROPERTY(QString directoryInDb READ getPath CONSTANT)
    Q_PROPERTY(QString sha256 READ getSha256 CONSTANT)
    Q_PROPERTY(BmsNotes* noteData READ getNoteData CONSTANT)
    ChartData() = default;

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
              int64_t length,
              QString path,
              QString directoryInDb,
              QString sha256,
              BmsNotes* noteData,
              QObject* parent = nullptr);

    [[nodiscard]] auto getTitle() const -> QString;
    [[nodiscard]] auto getArtist() const -> QString;
    [[nodiscard]] auto getSubtitle() const -> QString;
    [[nodiscard]] auto getSubartist() const -> QString;
    [[nodiscard]] auto getGenre() const -> QString;
    [[nodiscard]] auto getNoteCount() const -> int;
    [[nodiscard]] auto getLength() const -> int64_t;
    [[nodiscard]] auto getPath() const -> QString;
    [[nodiscard]] auto getDirectoryInDb() const -> QString;
    [[nodiscard]] auto getNoteData() const -> BmsNotes*;
    [[nodiscard]] auto getRank() const -> int;
    [[nodiscard]] auto getTotal() const -> double;
    [[nodiscard]] auto getPlayLevel() const -> int;
    [[nodiscard]] auto getDifficulty() const -> int;
    [[nodiscard]] auto getSha256() const -> QString;

    struct DTO
    {
        int id;
        std::string title;
        std::string artist;
        std::string subtitle;
        std::string subartist;
        std::string genre;
        int rank;
        double total;
        int playLevel;
        int difficulty;
        int noteCount;
        int64_t length;
        std::string path;
        std::string directoryInDb;
        std::string sha256;
        std::string noteData;
    };

    auto save(db::SqliteCppDb& db) const -> void;
    static auto load(DTO chartDataDto) -> ChartData*;

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
    int64_t length;
    QString path;
    QString directoryInDb;
    QString sha256;
    BmsNotes* noteData;
};

} // namespace gameplay_logic

#endif // RHYTHMGAME_CHARTDATA_H
