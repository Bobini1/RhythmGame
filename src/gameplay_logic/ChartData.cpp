//
// Created by bobini on 14.08.23.
//

#include "ChartData.h"

#include <utility>
#include <spdlog/spdlog.h>
#include <zstd.h>

gameplay_logic::ChartData::ChartData(QString title,
                                     QString artist,
                                     QString subtitle,
                                     QString subartist,
                                     QString genre,
                                     int rank,
                                     double total,
                                     int playLevel,
                                     int difficulty,
                                     bool isRandom,
                                     int noteCount,
                                     int64_t length,
                                     QString path,
                                     QString directoryInDb,
                                     QString sha256,
                                     std::unique_ptr<BmsNotes> noteData,
                                     QObject* parent)
  : QObject(parent)
  , title(std::move(title))
  , artist(std::move(artist))
  , subtitle(std::move(subtitle))
  , subartist(std::move(subartist))
  , genre(std::move(genre))
  , rank(rank)
  , total(total)
  , playLevel(playLevel)
  , difficulty(difficulty)
  , isRandom(isRandom)
  , noteCount(noteCount)
  , length(length)
  , path(std::move(path))
  , directoryInDb(std::move(directoryInDb))
  , sha256(std::move(sha256))
  , noteData(noteData.release())
{
    this->noteData->setParent(this);
}
auto
gameplay_logic::ChartData::getTitle() const -> QString
{
    return title;
}
auto
gameplay_logic::ChartData::getArtist() const -> QString
{
    return artist;
}
auto
gameplay_logic::ChartData::getNoteCount() const -> int
{
    return noteCount;
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
gameplay_logic::ChartData::getNoteData() const -> gameplay_logic::BmsNotes*
{
    return noteData;
}
auto
gameplay_logic::ChartData::getRank() const -> int
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
gameplay_logic::ChartData::getSubtitle() const -> QString
{
    return subtitle;
}
auto
gameplay_logic::ChartData::getSubartist() const -> QString
{
    return subartist;
}
auto
gameplay_logic::ChartData::getGenre() const -> QString
{
    return genre;
}
auto
gameplay_logic::ChartData::save(db::SqliteCppDb& db) const -> void
{
    static thread_local auto query = db.createStatement(
      "INSERT OR REPLACE INTO charts (title, artist, subtitle, subartist, "
      "genre, rank, total, play_level, difficulty, is_random, note_count, "
      "length, path, directory_in_db, sha256, note_data) "
      "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);");
    query.reset();
    query.bind(1, title.toStdString());
    query.bind(2, artist.toStdString());
    query.bind(3, subtitle.toStdString());
    query.bind(4, subartist.toStdString());
    query.bind(5, genre.toStdString());
    query.bind(6, rank);
    query.bind(7, total);
    query.bind(8, playLevel);
    query.bind(9, difficulty);
    query.bind(10, isRandom);
    query.bind(11, noteCount);
    query.bind(12, length);
    query.bind(13, path.toStdString());
    query.bind(14, directoryInDb.toStdString());
    query.bind(15, sha256.toStdString());
    // serialize into a buffer
    auto buffer = QByteArray{};
    auto stream = QDataStream{ &buffer, QIODevice::WriteOnly };
    stream << *noteData;
    // compress it!
    auto compressedBuffer = QByteArray{};
    compressedBuffer.resize(ZSTD_compressBound(buffer.size()));
    auto compressedSize = ZSTD_compress(compressedBuffer.data(),
                                        compressedBuffer.size(),
                                        buffer.data(),
                                        buffer.size(),
                                        1);
    if (ZSTD_isError(compressedSize)) {
        throw std::runtime_error{ "Failed to compress chart data" };
    }
    query.bind(16, compressedBuffer.left(compressedSize).toStdString());

    query.execute();
}
auto
gameplay_logic::ChartData::getSha256() const -> QString
{
    return sha256;
}
auto
gameplay_logic::ChartData::getDirectoryInDb() const -> QString
{
    return directoryInDb;
}

auto
gameplay_logic::ChartData::load(gameplay_logic::ChartData::DTO chartDataDto)
  -> std::unique_ptr<gameplay_logic::ChartData>
{
    using namespace std::string_literals;
    auto decompressedBuffer = QByteArray{};
    auto decompressedSize = ZSTD_getFrameContentSize(
      chartDataDto.noteData.data(), chartDataDto.noteData.size());
    if (ZSTD_isError(decompressedSize)) {
        const auto* error = ZSTD_getErrorName(decompressedSize);
        throw std::runtime_error{ "Failed to decompress chart data: "s +
                                  error };
    }
    decompressedBuffer.resize(decompressedSize);
    decompressedSize = ZSTD_decompress(decompressedBuffer.data(),
                                       decompressedBuffer.size(),
                                       chartDataDto.noteData.data(),
                                       chartDataDto.noteData.size());
    if (ZSTD_isError(decompressedSize)) {
        // what error?
        const auto* error = ZSTD_getErrorName(decompressedSize);
        throw std::runtime_error{ "Failed to decompress chart data: "s +
                                  error };
    }
    decompressedBuffer.resize(decompressedSize);
    auto noteData = std::make_unique<BmsNotes>();
    auto stream = QDataStream{ &decompressedBuffer, QIODevice::ReadOnly };
    stream >> *noteData;

    return std::make_unique<ChartData>(
      QString::fromStdString(chartDataDto.title),
      QString::fromStdString(chartDataDto.artist),
      QString::fromStdString(chartDataDto.subtitle),
      QString::fromStdString(chartDataDto.subartist),
      QString::fromStdString(chartDataDto.genre),
      chartDataDto.rank,
      chartDataDto.total,
      chartDataDto.playLevel,
      chartDataDto.difficulty,
      static_cast<bool>(chartDataDto.isRandom),
      chartDataDto.noteCount,
      chartDataDto.length,
      QString::fromStdString(chartDataDto.path),
      QString::fromStdString(chartDataDto.directoryInDb),
      QString::fromStdString(chartDataDto.sha256),
      std::move(noteData));
}