//
// Created by bobini on 14.08.23.
//

#include "ChartData.h"
#include "support/Compress.h"
#include <QFileInfo>
#include "BmsNotes.h"
#include <memory>
#include <utility>
#include <algorithm>
#include <array>
#include <spdlog/spdlog.h>
#include <QJsonArray>

gameplay_logic::ChartData::ChartData(QString title,
                                     QString artist,
                                     QString subtitle,
                                     QString subartist,
                                     QString genre,
                                     QString stageFile,
                                     QString banner,
                                     QString backBmp,
                                     double rank,
                                     double total,
                                     int playLevel,
                                     int difficulty,
                                     bool isRandom,
                                     QList<qint64> randomSequence,
                                     int normalNoteCount,
                                     int scratchCount,
                                     int lnCount,
                                     int bssCount,
                                     int mineCount,
                                     int64_t length,
                                     double bpm,
                                     double maxBpm,
                                     double minBpm,
                                     double mainBpm,
                                     double avgBpm,
                                     double peakDensity,
                                     double avgDensity,
                                     double endDensity,
                                     QString path,
                                     int64_t directory,
                                     QString sha256,
                                     QString md5,
                                     Keymode keymode,
                                     QList<QList<qint64>> histogramData,
                                     QList<BpmChange> bpmChanges,
                                     quint64 gameVersion,
                                     QObject* parent)
  : QObject(parent)
  , title(std::move(title))
  , artist(std::move(artist))
  , subtitle(std::move(subtitle))
  , subartist(std::move(subartist))
  , genre(std::move(genre))
  , stageFile(std::move(stageFile))
  , banner(std::move(banner))
  , backBmp(std::move(backBmp))
  , randomSequence(std::move(randomSequence))
  , rank(rank)
  , total(total)
  , playLevel(playLevel)
  , difficulty(difficulty)
  , isRandom(isRandom)
  , normalNoteCount(normalNoteCount)
  , scratchCount(scratchCount)
  , lnCount(lnCount)
  , bssCount(bssCount)
  , mineCount(mineCount)
  , length(length)
  , initialBpm(bpm)
  , maxBpm(maxBpm)
  , minBpm(minBpm)
  , mainBpm(mainBpm)
  , avgBpm(avgBpm)
  , peakDensity(peakDensity)
  , avgDensity(avgDensity)
  , endDensity(endDensity)
  , path(std::move(path))
  , directory(directory)
  , sha256(std::move(sha256))
  , md5(std::move(md5))
  , keymode(keymode)
  , gameVersion(gameVersion)
  , histogramData(std::move(histogramData))
  , bpmChanges(std::move(bpmChanges))
{
}
auto
gameplay_logic::ChartData::getTitle() const -> const QString&
{
    return title;
}
auto
gameplay_logic::ChartData::getArtist() const -> const QString&
{
    return artist;
}
auto
gameplay_logic::ChartData::getNormalNoteCount() const -> int
{
    return normalNoteCount;
}
auto
gameplay_logic::ChartData::getScratchCount() const -> int
{
    return scratchCount;
}
auto
gameplay_logic::ChartData::getLength() const -> int64_t
{
    return length;
}
auto
gameplay_logic::ChartData::getPath() const -> QString
{
    return path;
}
auto
gameplay_logic::ChartData::getRank() const -> double
{
    return rank;
}
auto
gameplay_logic::ChartData::getTotal() const -> double
{
    return total;
}
auto
gameplay_logic::ChartData::getPlayLevel() const -> int
{
    return playLevel;
}
auto
gameplay_logic::ChartData::getDifficulty() const -> int
{
    return difficulty;
}
auto
gameplay_logic::ChartData::getSubtitle() const -> const QString&
{
    return subtitle;
}
auto
gameplay_logic::ChartData::getSubartist() const -> const QString&
{
    return subartist;
}
auto
gameplay_logic::ChartData::getGenre() const -> const QString&
{
    return genre;
}
auto
gameplay_logic::ChartData::save(db::SqliteCppDb& db) const -> void
{
    auto query = db.createStatement(
      "INSERT OR REPLACE INTO charts (title, artist, subtitle, subartist, "
      "genre, stage_file, banner, back_bmp, rank, total, play_level, "
      "difficulty, is_random, random_sequence, normal_note_count, "
      "scratch_count, ln_count, "
      "bss_count, mine_count, length, initial_bpm, max_bpm, "
      "min_bpm, main_bpm, avg_bpm, peak_density, avg_density, end_density, "
      "path, chart_directory, directory, sha256, "
      "md5, keymode, game_version) "
      "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, "
      "?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);");
    query.bind(1, title.toStdString());
    query.bind(2, artist.toStdString());
    query.bind(3, subtitle.toStdString());
    query.bind(4, subartist.toStdString());
    query.bind(5, genre.toStdString());
    query.bind(6, stageFile.toStdString());
    query.bind(7, banner.toStdString());
    query.bind(8, backBmp.toStdString());
    query.bind(9, rank);
    query.bind(10, total);
    query.bind(11, playLevel);
    query.bind(12, difficulty);
    query.bind(13, isRandom);
    auto compressed = support::compress(randomSequence);
    query.bind(14, compressed.data(), compressed.size());
    query.bind(15, normalNoteCount);
    query.bind(16, scratchCount);
    query.bind(17, lnCount);
    query.bind(18, bssCount);
    query.bind(19, mineCount);
    query.bind(20, length);
    query.bind(21, initialBpm);
    query.bind(22, maxBpm);
    query.bind(23, minBpm);
    query.bind(24, mainBpm);
    query.bind(25, avgBpm);
    query.bind(26, peakDensity);
    query.bind(27, avgDensity);
    query.bind(28, endDensity);
    query.bind(29, path.toStdString());
    query.bind(30, getChartDirectory().toStdString());
    if (directory == -1) {
        query.bind(31);
    } else {
        query.bind(31, directory);
    }
    query.bind(32, sha256.toStdString());
    query.bind(33, md5.toStdString());
    query.bind(34, static_cast<int>(keymode));
    query.bind(35, static_cast<int64_t>(gameVersion));
    auto id = query.execute();
    auto query2 =
      db.createStatement("INSERT OR REPLACE INTO histogram_data "
                         "(histogram_data, bpms, chart_id) VALUES (?, ?, ?);");

    auto compressedHistogram = support::compress(histogramData);
    query2.bind(1, compressedHistogram.data(), compressedHistogram.size());
    auto compressedBpmChanges = support::compress(bpmChanges);
    query2.bind(2, compressedBpmChanges.data(), compressedBpmChanges.size());
    query2.bind(3, id);
    query2.execute();
}
auto
gameplay_logic::ChartData::getSha256() const -> const QString&
{
    return sha256;
}
auto
gameplay_logic::ChartData::getMd5() const -> const QString&
{
    return md5;
}

auto
gameplay_logic::ChartData::load(const DTO& chartDataDto)
  -> std::unique_ptr<ChartData>
{
    auto histogramData = QList<QList<qint64>>{};
    if (!chartDataDto.histogramData.empty()) {
        histogramData = support::decompress<QList<QList<qint64>>>(
          QByteArray::fromStdString(chartDataDto.histogramData));
    }
    auto bpmChanges = QList<BpmChange>{};
    if (!chartDataDto.bpmChanges.empty()) {
        bpmChanges = support::decompress<QList<BpmChange>>(
          QByteArray::fromStdString(chartDataDto.bpmChanges));
    }
    return std::make_unique<ChartData>(
      QString::fromStdString(chartDataDto.title),
      QString::fromStdString(chartDataDto.artist),
      QString::fromStdString(chartDataDto.subtitle),
      QString::fromStdString(chartDataDto.subartist),
      QString::fromStdString(chartDataDto.genre),
      QString::fromStdString(chartDataDto.stageFile),
      QString::fromStdString(chartDataDto.banner),
      QString::fromStdString(chartDataDto.backBmp),
      chartDataDto.rank,
      chartDataDto.total,
      chartDataDto.playLevel,
      chartDataDto.difficulty,
      static_cast<bool>(chartDataDto.isRandom),
      support::decompress<QList<qint64>>(
        QByteArray::fromStdString(chartDataDto.randomSequence)),
      chartDataDto.normalNoteCount,
      chartDataDto.scratchCount,
      chartDataDto.lnCount,
      chartDataDto.bssCount,
      chartDataDto.mineCount,
      chartDataDto.length,
      chartDataDto.initialBpm,
      chartDataDto.maxBpm,
      chartDataDto.minBpm,
      chartDataDto.mainBpm,
      chartDataDto.avgBpm,
      chartDataDto.peakDensity,
      chartDataDto.avgDensity,
      chartDataDto.endDensity,
      QString::fromStdString(chartDataDto.path),
      chartDataDto.directory,
      QString::fromStdString(chartDataDto.sha256),
      QString::fromStdString(chartDataDto.md5),
      static_cast<Keymode>(chartDataDto.keymode),
      std::move(histogramData),
      std::move(bpmChanges),
      chartDataDto.gameVersion);
}
auto
gameplay_logic::isDp(ChartData::Keymode keymode) -> bool
{
    return keymode == ChartData::Keymode::K10 ||
           keymode == ChartData::Keymode::K14;
}
auto
gameplay_logic::ChartData::getIsRandom() const -> bool
{
    return isRandom;
}
auto
gameplay_logic::ChartData::getRandomSequence() const -> const QList<qint64>&
{
    return randomSequence;
}
auto
gameplay_logic::ChartData::getKeymode() const -> Keymode
{
    return keymode;
}
auto
gameplay_logic::ChartData::getStageFile() const -> const QString&
{
    return stageFile;
}
auto
gameplay_logic::ChartData::getBanner() const -> const QString&
{
    return banner;
}
auto
gameplay_logic::ChartData::getBackBmp() const -> const QString&
{
    return backBmp;
}
auto
gameplay_logic::ChartData::getDirectory() const -> QString
{
    if (directory == 0) {
        return "";
    }
    auto dir = getChartDirectory();
    dir = dir.left(dir.size() - 1);
    return dir.left(dir.lastIndexOf('/') + 1);
}
auto
gameplay_logic::ChartData::getChartDirectory() const -> QString
{
    return QFileInfo{ path }.absolutePath() + '/';
}
auto
gameplay_logic::ChartData::getHistogramData() -> QList<QList<qint64>>&
{
    return histogramData;
}
auto
gameplay_logic::ChartData::getBpmChanges() -> QList<BpmChange>&
{
    return bpmChanges;
}
auto
gameplay_logic::ChartData::getGameVersion() const -> quint64
{
    return gameVersion;
}
auto
gameplay_logic::ChartData::clone() const -> std::unique_ptr<ChartData>
{
    return std::make_unique<ChartData>(title,
                                       artist,
                                       subtitle,
                                       subartist,
                                       genre,
                                       stageFile,
                                       banner,
                                       backBmp,
                                       rank,
                                       total,
                                       playLevel,
                                       difficulty,
                                       isRandom,
                                       randomSequence,
                                       normalNoteCount,
                                       scratchCount,
                                       lnCount,
                                       bssCount,
                                       mineCount,
                                       length,
                                       initialBpm,
                                       maxBpm,
                                       minBpm,
                                       mainBpm,
                                       avgBpm,
                                       peakDensity,
                                       avgDensity,
                                       endDensity,
                                       path,
                                       directory,
                                       sha256,
                                       md5,
                                       keymode,
                                       histogramData,
                                       bpmChanges,
                                       gameVersion);
}
auto
gameplay_logic::ChartData::getInitialBpm() const -> double
{
    return initialBpm;
}
auto
gameplay_logic::ChartData::getMaxBpm() const -> double
{
    return maxBpm;
}
auto
gameplay_logic::ChartData::getMinBpm() const -> double
{
    return minBpm;
}
auto
gameplay_logic::ChartData::getMainBpm() const -> double
{
    return mainBpm;
}
auto
gameplay_logic::ChartData::getAvgBpm() const -> double
{
    return avgBpm;
}
auto
gameplay_logic::ChartData::getPeakDensity() const -> double
{
    return peakDensity;
}
auto
gameplay_logic::ChartData::getAvgDensity() const -> double
{
    return avgDensity;
}
auto
gameplay_logic::ChartData::getEndDensity() const -> double
{
    return endDensity;
}
auto
gameplay_logic::ChartData::getLnCount() const -> int
{
    return lnCount;
}
auto
gameplay_logic::ChartData::getBssCount() const -> int
{
    return bssCount;
}
auto
gameplay_logic::ChartData::getMineCount() const -> int
{
    return mineCount;
}
auto
gameplay_logic::ChartData::toJson() const -> QJsonObject
{
    QJsonObject obj;
    obj["title"] = title;
    obj["artist"] = artist;
    obj["subtitle"] = subtitle;
    obj["subartist"] = subartist;
    obj["genre"] = genre;
    obj["stageFile"] = stageFile;
    obj["banner"] = banner;
    obj["backBmp"] = backBmp;
    obj["rank"] = rank;
    obj["total"] = total;
    obj["playLevel"] = playLevel;
    obj["difficulty"] = difficulty;
    obj["normalNoteCount"] = normalNoteCount;
    obj["scratchCount"] = scratchCount;
    obj["lnCount"] = lnCount;
    obj["bssCount"] = bssCount;
    obj["mineCount"] = mineCount;
    obj["length"] = static_cast<qint64>(length);
    obj["sha256"] = sha256;
    obj["md5"] = md5;
    obj["initialBpm"] = initialBpm;
    obj["maxBpm"] = maxBpm;
    obj["minBpm"] = minBpm;
    obj["mainBpm"] = mainBpm;
    obj["avgBpm"] = avgBpm;
    obj["peakDensity"] = peakDensity;
    obj["avgDensity"] = avgDensity;
    obj["endDensity"] = endDensity;
    obj["isRandom"] = isRandom;
    QJsonArray seq;
    for (auto v : randomSequence) {
        seq.append(static_cast<qint64>(v));
    }
    obj["randomSequence"] = seq;
    obj["keymode"] = static_cast<int>(keymode);
    QJsonArray histogramJson;
    for (const auto& list : histogramData) {
        QJsonArray sub;
        for (auto v : list)
            sub.append(static_cast<qint64>(v));
        histogramJson.append(sub);
    }
    obj["histogramData"] = histogramJson;
    QJsonArray bpmJson;
    for (const auto& bpm : bpmChanges) {
        QJsonObject o;
        o["time"] = static_cast<qint64>(bpm.time.timestamp);
        o["position"] = bpm.time.position;
        o["beatPosition"] = bpm.time.beatPosition;
        o["bpm"] = bpm.bpm;
        o["scroll"] = bpm.scroll;
        bpmJson.append(o);
    }
    obj["bpmChanges"] = bpmJson;
    obj["gameVersion"] = static_cast<qint64>(gameVersion);
    return obj;
}

auto
gameplay_logic::ChartData::getTimingWindows() const -> QVariantList
{
    const auto hash = getTimingWindowsHash();
    QVariantList result;

    auto appendIf = [&](Judgement j, const QString& name) {
        if (!hash.contains(j))
            return;
        const auto p = hash.value(j);
        const qint64 early = p.first < 0 ? -p.first : p.first;
        const qint64 late = p.second;
        QVariantMap m;
        m[QStringLiteral("judgement")] = name;
        m[QStringLiteral("earlyNs")] =
          QVariant::fromValue(static_cast<qint64>(early));
        m[QStringLiteral("lateNs")] =
          QVariant::fromValue(static_cast<qint64>(late));
        result.append(m);
    };

    appendIf(Judgement::EmptyPoor, QString::fromLatin1("EmptyPoor"));
    appendIf(Judgement::Bad, QString::fromLatin1("Bad"));
    appendIf(Judgement::Good, QString::fromLatin1("Good"));
    appendIf(Judgement::Great, QString::fromLatin1("Great"));
    appendIf(Judgement::Perfect, QString::fromLatin1("Perfect"));

    return result;
}

auto
gameplay_logic::ChartData::getTimingWindowsHash() const
  -> QHash<Judgement, QPair<qint64, qint64>>
{
    QHash<Judgement, QPair<qint64, qint64>> ret;
    static constexpr qint64 kMs = 1'000'000LL;

    constexpr auto perfectMsBase = std::array{ 8.0, 15.0, 18.0, 21.0 };
    constexpr auto greatMsBase = std::array{ 24.0, 30.0, 40.0, 60.0 };
    constexpr auto goodMsBase = std::array{ 40.0, 60.0, 100.0, 120.0 };

    auto r = rank;
    auto interp = [](double a, double b, double t) { return a + (b - a) * t; };

    auto perfectMs = 0.0;
    auto greatMs = 0.0;
    auto goodMs = 0.0;

    if (r <= 0.0) {
        perfectMs = 0.0;
        greatMs = 0.0;
        goodMs = 0.0;
    } else if (r <= 25.0) {
        auto t = (r - 0.0) / 25.0;
        perfectMs = interp(0.0, perfectMsBase[0], t);
        greatMs = interp(0.0, greatMsBase[0], t);
        goodMs = interp(0.0, goodMsBase[0], t);
    } else if (r <= 50.0) {
        auto t = (r - 25.0) / 25.0;
        perfectMs = interp(perfectMsBase[0], perfectMsBase[1], t);
        greatMs = interp(greatMsBase[0], greatMsBase[1], t);
        goodMs = interp(goodMsBase[0], goodMsBase[1], t);
    } else if (r <= 75.0) {
        auto t = (r - 50.0) / 25.0;
        perfectMs = interp(perfectMsBase[1], perfectMsBase[2], t);
        greatMs = interp(greatMsBase[1], greatMsBase[2], t);
        goodMs = interp(goodMsBase[1], goodMsBase[2], t);
    } else if (r <= 100.0) {
        auto t = (r - 75.0) / 25.0;
        perfectMs = interp(perfectMsBase[2], perfectMsBase[3], t);
        greatMs = interp(greatMsBase[2], greatMsBase[3], t);
        goodMs = interp(goodMsBase[2], goodMsBase[3], t);
    } else {
        auto scale = r / 100.0;
        perfectMs = perfectMsBase[3] * scale;
        greatMs = greatMsBase[3] * scale;
        goodMs = goodMsBase[3] * scale;
    }

    constexpr double badMsBase = 200.0;
    constexpr double emptyPoorEarlyMs = 1000.0;

    auto effectiveBadMs = badMsBase;
    if (goodMs > effectiveBadMs)
        effectiveBadMs = goodMs;
    auto effectiveEmptyPoorEarlyMs = emptyPoorEarlyMs;
    if (effectiveBadMs > effectiveEmptyPoorEarlyMs)
        effectiveEmptyPoorEarlyMs = effectiveBadMs;

    // EmptyPoor: open(-effectiveEmptyPoorEarlyMs, 0ms) -> store as {-earlyNs,
    // 0}
    ret.insert(Judgement::EmptyPoor,
               { -static_cast<qint64>(effectiveEmptyPoorEarlyMs * kMs),
                 static_cast<qint64>(0) });

    // Bad
    ret.insert(Judgement::Bad,
               { -static_cast<qint64>(effectiveBadMs * kMs),
                 static_cast<qint64>(effectiveBadMs * kMs) });

    // Good
    ret.insert(Judgement::Good,
               { -static_cast<qint64>(goodMs * kMs),
                 static_cast<qint64>(goodMs * kMs) });

    // Great
    ret.insert(Judgement::Great,
               { -static_cast<qint64>(greatMs * kMs),
                 static_cast<qint64>(greatMs * kMs) });

    // Perfect
    ret.insert(Judgement::Perfect,
               { -static_cast<qint64>(perfectMs * kMs),
                 static_cast<qint64>(perfectMs * kMs) });

    return ret;
}
