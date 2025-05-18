//
// Created by bobini on 14.08.23.
//

#ifndef RHYTHMGAME_CHARTDATA_H
#define RHYTHMGAME_CHARTDATA_H

#include <QQmlEngine>
#include <QJSEngine>
#include "BmsNotes.h"
#include "db/SqliteCppDb.h"
namespace gameplay_logic {

class ChartData : public QObject
{
    Q_OBJECT

  public:
    enum class Keymode
    {
        K5 = 5,
        K7 = 7,
        K10 = 10,
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
    Q_PROPERTY(QString directory READ getDirectory CONSTANT)
    Q_PROPERTY(QString chartDirectory READ getChartDirectory CONSTANT)
    Q_PROPERTY(QString sha256 READ getSha256 CONSTANT)
    Q_PROPERTY(QString md5 READ getMd5 CONSTANT)
    Q_PROPERTY(bool isRandom READ getIsRandom CONSTANT)
    Q_PROPERTY(QList<qint64> randomSequence READ getRandomSequence CONSTANT)
    Q_PROPERTY(Keymode keymode READ getKeymode CONSTANT)
    Q_PROPERTY(double initialBpm READ getInitialBpm CONSTANT)
    Q_PROPERTY(double maxBpm READ getMaxBpm CONSTANT)
    Q_PROPERTY(double minBpm READ getMinBpm CONSTANT)
    Q_PROPERTY(double mainBpm READ getMainBpm CONSTANT)
    Q_PROPERTY(double avgBpm READ getAvgBpm CONSTANT)

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
              QList<qint64> randomSequence,
              int normalNoteCount,
              int lnCount,
              int mineCount,
              int64_t length,
              double initialBpm,
              double maxBpm,
              double minBpm,
              double mainBpm,
              double avgBpm,
              QString path,
              int64_t directory,
              QString sha256,
              QString md5,
              Keymode keymode,
              QObject* parent = nullptr);

    [[nodiscard]] auto getTitle() const -> const QString&;
    [[nodiscard]] auto getArtist() const -> const QString&;
    [[nodiscard]] auto getSubtitle() const -> const QString&;
    [[nodiscard]] auto getSubartist() const -> const QString&;
    [[nodiscard]] auto getGenre() const -> const QString&;
    [[nodiscard]] auto getStageFile() const -> const QString&;
    [[nodiscard]] auto getBanner() const -> const QString&;
    [[nodiscard]] auto getBackBmp() const -> const QString&;
    [[nodiscard]] auto getNormalNoteCount() const -> int;
    [[nodiscard]] auto getLnCount() const -> int;
    [[nodiscard]] auto getMineCount() const -> int;
    [[nodiscard]] auto getLength() const -> int64_t;
    [[nodiscard]] auto getInitialBpm() const -> double;
    [[nodiscard]] auto getMaxBpm() const -> double;
    [[nodiscard]] auto getMinBpm() const -> double;
    [[nodiscard]] auto getMainBpm() const -> double;
    [[nodiscard]] auto getAvgBpm() const -> double;
    [[nodiscard]] auto getPath() const -> QString;
    [[nodiscard]] auto getRank() const -> int;
    [[nodiscard]] auto getTotal() const -> double;
    [[nodiscard]] auto getPlayLevel() const -> int;
    [[nodiscard]] auto getDifficulty() const -> int;
    [[nodiscard]] auto getSha256() const -> const QString&;
    [[nodiscard]] auto getMd5() const -> const QString&;
    [[nodiscard]] auto getIsRandom() const -> bool;
    [[nodiscard]] auto getRandomSequence() const -> const QList<qint64>&;
    [[nodiscard]] auto getKeymode() const -> Keymode;
    [[nodiscard]] auto getDirectory() const -> QString;
    [[nodiscard]] auto getChartDirectory() const -> QString;

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
        std::string randomSequence;
        int normalNoteCount;
        int lnCount;
        int mineCount;
        int64_t length;
        double initialBpm;
        double maxBpm;
        double minBpm;
        double mainBpm;
        double avgBpm;
        std::string path;
        int64_t directory;
        std::string sha256;
        std::string md5;
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
    QList<qint64> randomSequence;
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
    double mainBpm;
    double avgBpm;
    QString path;
    int64_t directory;
    QString sha256;
    QString md5;
    Keymode keymode;
};

auto
isDp(ChartData::Keymode keymode) -> bool;

} // namespace gameplay_logic

#endif // RHYTHMGAME_CHARTDATA_H
