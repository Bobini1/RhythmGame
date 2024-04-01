//
// Created by bobini on 14.08.23.
//

#ifndef RHYTHMGAME_CHARTDATA_H
#define RHYTHMGAME_CHARTDATA_H

#include <QObject>
#include <QQmlEngine>
#include <QJSEngine>
#include "BmsScore.h"
#include "BmsNotes.h"
#include "db/SqliteCppDb.h"
namespace gameplay_logic {

class ChartData : public QObject
{
    Q_OBJECT

  public:
    enum class Keymode
    {
        K7 = 7,
        K14 = 14
    };
    Q_ENUM(Keymode)

  private:
    Q_PROPERTY(QString title READ getTitle CONSTANT)
    Q_PROPERTY(QString artist READ getArtist CONSTANT)
    Q_PROPERTY(QString subtitle READ getSubtitle CONSTANT)
    Q_PROPERTY(QString subartist READ getSubartist CONSTANT)
    Q_PROPERTY(QString genre READ getGenre CONSTANT)
    Q_PROPERTY(QString stageFile READ getStageFile CONSTANT)
    Q_PROPERTY(QString banner READ getBanner CONSTANT)
    Q_PROPERTY(QString backBmp READ getBackBmp CONSTANT)
    Q_PROPERTY(int rank READ getRank CONSTANT)
    Q_PROPERTY(double total READ getTotal CONSTANT)
    Q_PROPERTY(int playLevel READ getPlayLevel CONSTANT)
    Q_PROPERTY(int difficulty READ getDifficulty CONSTANT)
    Q_PROPERTY(int normalNoteCount READ getNormalNoteCount CONSTANT)
    Q_PROPERTY(int lnCount READ getLnCount CONSTANT)
    Q_PROPERTY(int mineCount READ getMineCount CONSTANT)
    Q_PROPERTY(int64_t length READ getLength CONSTANT)
    Q_PROPERTY(QString path READ getPath CONSTANT)
    Q_PROPERTY(QString directoryInDb READ getDirectoryInDb CONSTANT)
    Q_PROPERTY(QString sha256 READ getSha256 CONSTANT)
    Q_PROPERTY(bool isRandom READ getIsRandom CONSTANT)
    Q_PROPERTY(Keymode keymode READ getKeymode CONSTANT)
    Q_PROPERTY(double initialBpm READ getInitialBpm CONSTANT)
    Q_PROPERTY(double maxBpm READ getMaxBpm CONSTANT)
    Q_PROPERTY(double minBpm READ getMinBpm CONSTANT)

    Q_PROPERTY(QString directory READ getDirectory CONSTANT)

    ChartData() = default;

  public:
    ChartData(QString title,
              QString artist,
              QString subtitle,
              QString subartist,
              QString genre,
              QString stageFile,
              QString banner,
              QString backBmp,
              int rank,
              double total,
              int playLevel,
              int difficulty,
              bool isRandom,
              int normalNoteCount,
              int lnCount,
              int mineCount,
              int64_t length,
              double initialBpm,
              double maxBpm,
              double minBpm,
              QString path,
              QString directoryInDb,
              QString sha256,
              Keymode keymode,
              QObject* parent = nullptr);

    [[nodiscard]] auto getTitle() const -> QString;
    [[nodiscard]] auto getArtist() const -> QString;
    [[nodiscard]] auto getSubtitle() const -> QString;
    [[nodiscard]] auto getSubartist() const -> QString;
    [[nodiscard]] auto getGenre() const -> QString;
    [[nodiscard]] auto getStageFile() const -> QString;
    [[nodiscard]] auto getBanner() const -> QString;
    [[nodiscard]] auto getBackBmp() const -> QString;
    [[nodiscard]] auto getNormalNoteCount() const -> int;
    [[nodiscard]] auto getLnCount() const -> int;
    [[nodiscard]] auto getMineCount() const -> int;
    [[nodiscard]] auto getLength() const -> int64_t;
    [[nodiscard]] auto getInitialBpm() const -> double;
    [[nodiscard]] auto getMaxBpm() const -> double;
    [[nodiscard]] auto getMinBpm() const -> double;
    [[nodiscard]] auto getPath() const -> QString;
    [[nodiscard]] auto getDirectoryInDb() const -> QString;
    [[nodiscard]] auto getRank() const -> int;
    [[nodiscard]] auto getTotal() const -> double;
    [[nodiscard]] auto getPlayLevel() const -> int;
    [[nodiscard]] auto getDifficulty() const -> int;
    [[nodiscard]] auto getSha256() const -> QString;
    [[nodiscard]] auto getIsRandom() const -> bool;
    [[nodiscard]] auto getKeymode() const -> Keymode;

    [[nodiscard]] auto getDirectory() const -> QString;

    struct DTO
    {
        int id;
        std::string title;
        std::string artist;
        std::string subtitle;
        std::string subartist;
        std::string genre;
        std::string stageFile;
        std::string banner;
        std::string backBmp;
        int rank;
        double total;
        int playLevel;
        int difficulty;
        int isRandom;
        int normalNoteCount;
        int lnCount;
        int mineCount;
        int64_t length;
        double initialBpm;
        double maxBpm;
        double minBpm;
        std::string path;
        std::string directory;
        std::string sha256;
        int keymode;
    };

    auto save(db::SqliteCppDb& db) const -> void;
    static auto load(const DTO& chartDataDto) -> std::unique_ptr<ChartData>;

  private:
    QString title;
    QString artist;
    QString level;
    QString subtitle;
    QString subartist;
    QString genre;
    QString stageFile;
    QString banner;
    QString backBmp;
    int rank;
    double total;
    int playLevel;
    int difficulty;
    bool isRandom;
    int normalNoteCount;
    int lnCount;
    int mineCount;
    int64_t length;
    double initialBpm;
    double maxBpm;
    double minBpm;
    QString path;
    QString directoryInDb;
    QString sha256;
    Keymode keymode;
};

} // namespace gameplay_logic

#endif // RHYTHMGAME_CHARTDATA_H
