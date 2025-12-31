//
// Created by bobini on 14.08.23.
//

#ifndef RHYTHMGAME_CHARTDATA_H
#define RHYTHMGAME_CHARTDATA_H

#include <QQmlEngine>
#include "db/SqliteCppDb.h"
namespace gameplay_logic {
class BpmChange;

/**
 * @brief Metadata and basic stats of a chart.
 * @details When loading a chart, the ChartData might be different than what was
 * cached in the database, because the chart file might have changed or it
 * contained [#RANDOM](https://hitkey.nekokan.dyndns.info/cmds.htm#RANDOM),
 * which generates a different chart each time the chart is parsed.
 * In the latter case you can use the randomSequence property to recreate the
 * same chart.
 */
class ChartData : public QObject
{
    Q_OBJECT

  public:
    /**
     * @details The keymode of the chart, i.e. how many keys/columns it has.
     * If the DP option "Battle" is enabled, an SP chart will have 14 keys
     * regardless, so pay attention to that.
     * @warning K5 and K10 are unimplemented and treated as K7 and K14
     * respectively.
     */
    enum class Keymode
    {
        K5 = 5,
        K7 = 7,
        K10 = 10,
        K14 = 14
    };
    Q_ENUM(Keymode)

  private:
    /** @brief The title of the chart. */
    Q_PROPERTY(QString title READ getTitle CONSTANT)
    /** @brief The artist of the chart. */
    Q_PROPERTY(QString artist READ getArtist CONSTANT)
    /** @brief The subtitle of the chart. */
    Q_PROPERTY(QString subtitle READ getSubtitle CONSTANT)
    /** @brief The subartist of the chart. */
    Q_PROPERTY(QString subartist READ getSubartist CONSTANT)
    /** @brief The genre of the chart. */
    Q_PROPERTY(QString genre READ getGenre CONSTANT)
    /** @brief The path to the chart's stagefile. */
    Q_PROPERTY(QString stageFile READ getStageFile CONSTANT)
    /** @brief The path to the chart's banner. */
    Q_PROPERTY(QString banner READ getBanner CONSTANT)
    /** @brief The path to the chart's backbmp. */
    Q_PROPERTY(QString backBmp READ getBackBmp CONSTANT)
    /**
     * @brief The rank of the chart.
     * @details Determines the timing windows for judgements.
     * @see https://hitkey.nekokan.dyndns.info/cmds.htm#RANK
     * @see rules::lr2_timing_windows::Lr2TimingWindows::getTimingWindows()
     */
    Q_PROPERTY(int rank READ getRank CONSTANT)
    /**
     * @brief The total value of the chart.
     * @details RhythmGame's gauges use the same total value system as LR2.
     * @see https://iidx.org/misc/iidx_lr2_beatoraja_diff#gauge-types
     */
    Q_PROPERTY(double total READ getTotal CONSTANT)
    /**
     * @brief The play level of the chart.
     * @details This is purely visual. How you display it is up to you.
     * @see https://hitkey.nekokan.dyndns.info/cmds.htm#PLAYLEVEL
     */
    Q_PROPERTY(int playLevel READ getPlayLevel CONSTANT)
    /**
     * @brief The difficulty of the chart.
     * @details This is purely visual. How you display it is up to you.
     * @see https://hitkey.nekokan.dyndns.info/cmds.htm#DIFFICULTY
     */
    Q_PROPERTY(int difficulty READ getDifficulty CONSTANT)
    /**
     * @brief The number of normal notes in the chart.
     * @details Normal means not long notes, not mines, not invisible notes.
     */
    Q_PROPERTY(int normalNoteCount READ getNormalNoteCount CONSTANT)
    /**
     * @brief The number of long notes in the chart excluding BSS (scratch lns).
     * @details A long note consists of an LN start and LN end. Such a pair
     * counts as one long note.
     */
    Q_PROPERTY(int lnCount READ getLnCount CONSTANT)
    /**
     * @brief The number of BSS (scratch long notes) in the chart.
     * @details A BSS consists of an ln start and ln end. Such a pair
     * counts as one BSS.
     */
    Q_PROPERTY(int bssCount READ getBssCount CONSTANT)
    /**
     * @brief The number of mines (landmines) in the chart.
     */
    Q_PROPERTY(int mineCount READ getMineCount CONSTANT)
    /**
     * @brief The length of the chart in nanoseconds.
     * @details This property indicates the total duration of the chart from
     * start to finish, measured in nanoseconds.
     * @note This is based on the timestamp of the last visible note.
     */
    Q_PROPERTY(int64_t length READ getLength CONSTANT)
    /**
     * @brief The path to the chart file.
     * @details This is an absolute path.
     */
    Q_PROPERTY(QString path READ getPath CONSTANT)
    /**
     * @brief The parent directory of the directory containing the chart file.
     * @details This is an absolute path.
     */
    Q_PROPERTY(QString directory READ getDirectory CONSTANT)
    /**
     * @brief The directory containing the chart file.
     * @details This is an absolute path.
     */
    Q_PROPERTY(QString chartDirectory READ getChartDirectory CONSTANT)
    /**
     * @brief The SHA-256 hash of the chart file, in hexadecimal upper-case
     * format.
     * @details This can be used to uniquely identify a chart file.
     */
    Q_PROPERTY(QString sha256 READ getSha256 CONSTANT)
    /**
     * @brief The MD5 hash of the chart file, in hexadecimal upper-case format.
     * @details This can be used to uniquely identify a chart file.
     */
    Q_PROPERTY(QString md5 READ getMd5 CONSTANT)
    /**
     * @brief Whether the chart uses
     * [#RANDOM](https://hitkey.nekokan.dyndns.info/cmds.htm#RANDOM).
     * @details If true, the randomSequence property can be used to recreate
     * the same chart.
     */
    Q_PROPERTY(bool isRandom READ getIsRandom CONSTANT)
    /**
     * @brief The random sequence used for parsing the chart.
     * @details If the chart uses
     * [#RANDOM](https://hitkey.nekokan.dyndns.info/cmds.htm#RANDOM),
     * this property provides the sequence of random values used to determine
     * the note order. If the chart does not use randomization,
     * this list will be empty.
     */
    Q_PROPERTY(QList<qint64> randomSequence READ getRandomSequence CONSTANT)
    /**
     * @brief The keymode of the chart, i.e. how many keys/columns it has.
     * If the DP option "Battle" is enabled, an SP chart will have 14 keys
     * regardless, so pay attention to that.
     * @warning K5 and K10 are unimplemented and treated as K7 and K14,
     * respectively.
     * @see Keymode
     */
    Q_PROPERTY(Keymode keymode READ getKeymode CONSTANT)
    /**
     * @brief The initial BPM of the chart.
     * @details This is the BPM at the start of the chart, before any BPM
     * changes.
     */
    Q_PROPERTY(double initialBpm READ getInitialBpm CONSTANT)
    /**
     * @brief The maximum BPM of the chart.
     */
    Q_PROPERTY(double maxBpm READ getMaxBpm CONSTANT)
    /**
     * @brief The minimum BPM of the chart.
     */
    Q_PROPERTY(double minBpm READ getMinBpm CONSTANT)
    /**
     * @brief The main BPM of the chart.
     * @details The main BPM is the BPM that is active for the longest time
     * during the chart.
     */
    Q_PROPERTY(double mainBpm READ getMainBpm CONSTANT)
    /**
     * @brief The average BPM of the chart.
     * @details The average BPM is the time-weighted average BPM of the chart.
     */
    Q_PROPERTY(double avgBpm READ getAvgBpm CONSTANT)
    /**
     * @brief The histogram data representing the distribution of hit timings.
     */
    Q_PROPERTY(QList<QList<int>> histogramData READ getHistogramData CONSTANT)

    auto getHistogramData() const -> const QList<QList<int>>&
    {
        return histogramData;
    }

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
              int bssCount,
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
    [[nodiscard]] auto getBssCount() const -> int;
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
    [[nodiscard]] auto getHistogramData() -> QList<QList<int>>&;

    auto clone() const -> std::unique_ptr<ChartData>;

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
        int bssCount;
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
        std::string histogramData;
        std::string bpmData;
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
    int bssCount;
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
    QList<QList<int64_t>> histogramData;
    QList<BpmChange> bpmChanges;
};

auto
isDp(ChartData::Keymode keymode) -> bool;

} // namespace gameplay_logic

#endif // RHYTHMGAME_CHARTDATA_H
