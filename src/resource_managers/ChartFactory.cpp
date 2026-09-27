//
// Created by bobini on 24.08.23.
//

#include <QtConcurrent>
#include <QObject>
#include "ChartFactory.h"

#include "loadBmsSounds.h"
#include "SongAssetStore.h"
#include "qml_components/ProfileList.h"
#include "support/GeneratePermutation.h"
#include "support/QStringToPath.h"
#include "support/PathToQString.h"
#include "support/PathToUtfString.h"
#include <QByteArrayView>
#include <QImageReader>
#include <QVideoFrame>
#include <QGuiApplication>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace resource_managers {

auto
convertImageToFrame(const QImage& image) -> std::unique_ptr<QVideoFrame>
{
    auto frame = std::make_unique<QVideoFrame>(
      QVideoFrameFormat(image.size(), QVideoFrameFormat::Format_RGBA8888));
    frame->map(QVideoFrame::WriteOnly);
    std::copy(image.bits(), image.bits() + image.sizeInBytes(), frame->bits(0));
    frame->unmap();
    return frame;
}

auto
prepareBmp(QImage image) -> QImage
{
    if (image.isNull()) {
        return {};
    }
    image.convertTo(QImage::Format_RGBA8888);
    auto size = image.size();
    // if smaller than 256x256, center on x axis and put on top
    if (size.height() < 256) {
        size.setHeight(256);
    }
    auto widthDiff = size.width() - image.width();
    if (widthDiff < 0) {
        widthDiff = 0;
    }
    if (size.width() < 256) {
        size.setWidth(256);
    }
    if (size != image.size()) {
        return image.copy(
          QRect{ -widthDiff / 2, 0, size.width(), size.height() });
    }
    return image;
}

auto
loadBmp(std::filesystem::path path) -> QImage
{
    auto original = path;
    // remove extension
    const auto pathQString = support::pathToQString(path.replace_extension());
    // QImage will try out a few different extensions
    auto image = QImage(pathQString);
    if (image.isNull()) {
        // try the EXACT extension that was declared. Helps with uppercase
        // extensions on Linux
        image = QImage(support::pathToQString(original));
        if (image.isNull()) {
            return {};
        }
    }
    return prepareBmp(std::move(image));
}

auto
loadBmp(const QByteArrayView encoded) -> QImage
{
    return prepareBmp(QImage::fromData(
      reinterpret_cast<const uchar*>(encoded.data()), encoded.size()));
}

auto
loadBmpVideo(const std::filesystem::path& path, std::stop_token stop)
  -> std::unique_ptr<QMediaPlayer>
{
    if (stop.stop_requested()) {
        return nullptr;
    }
    auto player = std::make_unique<QMediaPlayer>();
    QObject::connect(player.get(),
                     &QMediaPlayer::errorOccurred,
                     player.get(),
                     [](QMediaPlayer::Error error, const QString& message) {
                         spdlog::warn("Error loading video: ({}) {}",
                                      static_cast<int>(error),
                                      message.toStdString());
                     });
    QEventLoop loop;
    QObject::connect(player.get(),
                     &QMediaPlayer::mediaStatusChanged,
                     &loop,
                     &QEventLoop::quit);
    std::stop_callback cancelled(stop, [&loop] {
        QMetaObject::invokeMethod(
          &loop, &QEventLoop::quit, Qt::QueuedConnection);
    });
    player->setSource(QUrl::fromLocalFile(support::pathToQString(path)));
    if (!stop.stop_requested() &&
        player->mediaStatus() == QMediaPlayer::LoadingMedia) {
        loop.exec();
    }
    if (stop.stop_requested() ||
        player->mediaStatus() == QMediaPlayer::InvalidMedia ||
        player->mediaStatus() == QMediaPlayer::LoadingMedia) {
        return nullptr;
    }
    player->pause();
    return player;
}

struct BgaResources
{
    using Track = std::vector<std::pair<charts::BmsNotesData::Time, uint64_t>>;
    struct Frame
    {
        uint64_t id;
        QVideoFrame image;
        std::filesystem::path videoPath;
    };
    std::array<Track, 4> tracks;
    QList<Frame> frames;
};

auto
prepareBga(std::array<BgaResources::Track, 4> tracks,
           std::unordered_map<uint64_t, std::filesystem::path> bmps,
           const std::filesystem::path& path,
           SongAssetStore* assetStore,
           std::stop_token stop) -> BgaResources
{
    if (stop.stop_requested()) {
        return {};
    }
    auto cancelled = std::atomic_bool{ stop.stop_requested() };
    std::stop_callback onStop(stop, [&] { cancelled.store(true); });
    auto requested = std::unordered_map<uint64_t, std::filesystem::path>{};
    for (const auto& track : tracks) {
        for (const auto& [time, id] : track) {
            if (const auto found = bmps.find(id); found != bmps.end()) {
                requested.emplace(id, found->second);
            }
        }
    }
    auto frames = QtConcurrent::blockingMapped<QList<BgaResources::Frame>>(
      requested, [&, path, assetStore](const auto& bmp) -> BgaResources::Frame {
          auto result = BgaResources::Frame{ bmp.first, {}, {} };
          if (cancelled.load()) {
              return result;
          }
          const auto filePath = path / bmp.second;
          try {
              const auto archived =
                assetStore && assetStore->isVirtual(filePath);
              auto image = archived
                             ? loadBmp(assetStore->read(filePath, &cancelled))
                             : loadBmp(filePath);
              if (cancelled.load()) {
                  return result;
              }
              if (!image.isNull()) {
                  result.image = *convertImageToFrame(image);
              } else {
                  result.videoPath =
                    archived ? assetStore->materialize(filePath, &cancelled)
                             : filePath;
              }
          } catch (const std::exception& error) {
              if (!cancelled.load()) {
                  spdlog::warn("Could not load BGA {}: {}",
                               support::pathToUtfString(filePath),
                               error.what());
              }
          }
          return result;
      });
    return { std::move(tracks), std::move(frames) };
}

auto
finishBga(BgaResources resources, std::stop_token stop)
  -> std::unique_ptr<qml_components::BgaContainer>
{
    // Only this GUI-thread continuation constructs video players and QObjects.
    // Workers never wait for the GUI, including during shutdown.
    auto frames = std::vector<std::unique_ptr<QVideoFrame>>{};
    auto videos = std::vector<std::unique_ptr<QMediaPlayer>>{};
    auto frameById = std::unordered_map<uint64_t, QVideoFrame*>{};
    auto videoById = std::unordered_map<uint64_t, QMediaPlayer*>{};
    for (auto& resource : resources.frames) {
        if (stop.stop_requested()) {
            return nullptr;
        }
        if (resource.image.isValid()) {
            auto frame =
              std::make_unique<QVideoFrame>(std::move(resource.image));
            frameById.emplace(resource.id, frame.get());
            frames.push_back(std::move(frame));
        } else if (!resource.videoPath.empty()) {
            if (auto video = loadBmpVideo(resource.videoPath, stop)) {
                videoById.emplace(resource.id, video.get());
                videos.push_back(std::move(video));
            }
        }
    }
    if (stop.stop_requested()) {
        return nullptr;
    }
    auto layers = QList<qml_components::Bga*>{};
    for (const auto& track : resources.tracks) {
        auto images =
          std::vector<std::pair<std::chrono::nanoseconds, QVideoFrame*>>{};
        auto movies =
          std::vector<std::pair<std::chrono::nanoseconds, QMediaPlayer*>>{};
        for (const auto& [time, id] : track) {
            if (const auto frame = frameById.find(id);
                frame != frameById.end()) {
                images.emplace_back(time.timestamp, frame->second);
            } else if (const auto video = videoById.find(id);
                       video != videoById.end()) {
                movies.emplace_back(time.timestamp, video->second);
            } else {
                images.emplace_back(time.timestamp, nullptr);
            }
        }
        layers.append(
          new qml_components::Bga(std::move(movies), std::move(images)));
    }
    auto videoPointers = std::vector<QMediaPlayer*>{};
    for (const auto& video : videos) {
        videoPointers.push_back(video.get());
    }
    auto container = std::make_unique<qml_components::BgaContainer>(
      std::move(layers), std::move(videoPointers), std::move(frames));
    for (auto& video : videos) {
        video.release(); // Parented to the container.
    }
    return container;
}

struct RandomizedData
{
    std::unique_ptr<gameplay_logic::BmsNotes> notes;
    std::unique_ptr<gameplay_logic::GameplayState> state;
    std::array<support::ShuffleResult, 2> shuffleResults;
    std::unique_ptr<gameplay_logic::BmsLiveScore> score;
    std::array<std::vector<charts::BmsNotesData::Note>, 16> rawNotes;
};

auto
createAutoplayFromNotes(const gameplay_logic::BmsNotes& notes)
  -> std::vector<gameplay_logic::HitEvent>
{
    auto events = std::vector<gameplay_logic::HitEvent>{};
    const auto& noteArr = notes.getNotes();
    for (const auto& [columnIndex, column] :
         std::ranges::views::enumerate(noteArr)) {
        for (const auto& [noteIndex, note] :
             std::ranges::views::enumerate(column)) {
            if (note.type == gameplay_logic::Note::Type::Normal) {
                events.emplace_back(
                  columnIndex,
                  noteIndex,
                  note.time.timestamp,
                  gameplay_logic::BmsPoints{
                    0.0, gameplay_logic::Judgement::Perfect, 0 },
                  gameplay_logic::HitEvent::Action::Press,
                  /*noteRemoved=*/true);

                auto heldTime = 50'000'000;
                auto releaseTime = note.time.timestamp + heldTime;

                if (auto nextNote = std::next(column.begin(), noteIndex + 1);
                    nextNote != column.end()) {
                    if (nextNote->time.timestamp < releaseTime) {
                        heldTime =
                          (nextNote->time.timestamp - note.time.timestamp) / 2;
                        releaseTime = note.time.timestamp + heldTime;
                    }
                    if (nextNote->type ==
                        gameplay_logic::Note::Type::Landmine) {
                        releaseTime = note.time.timestamp;
                    }
                }
                events.emplace_back(columnIndex,
                                    -1,
                                    releaseTime,
                                    std::nullopt,
                                    gameplay_logic::HitEvent::Action::Release,
                                    /*noteRemoved=*/true);
            } else if (note.type == gameplay_logic::Note::Type::LongNoteBegin) {
                events.emplace_back(
                  columnIndex,
                  noteIndex,
                  note.time.timestamp,
                  gameplay_logic::BmsPoints{
                    0.0, gameplay_logic::Judgement::Perfect, 0 },
                  gameplay_logic::HitEvent::Action::Press,
                  /*noteRemoved=*/false);
            } else if (note.type == gameplay_logic::Note::Type::LongNoteEnd) {
                events.emplace_back(
                  columnIndex,
                  noteIndex,
                  note.time.timestamp,
                  gameplay_logic::BmsPoints{
                    0.0, gameplay_logic::Judgement::Perfect, 0 },
                  gameplay_logic::HitEvent::Action::Release,
                  /*noteRemoved=*/true);
            }
        }
    }
    std::ranges::stable_sort(events, [](const auto& left, const auto& right) {
        return left.getOffsetFromStart() < right.getOffsetFromStart();
    });
    return events;
}
namespace {
auto
applyBeatorajaOrder(std::span<std::vector<charts::BmsNotesData::Note>>& notes,
                    NoteOrderAlgorithm algorithm,
                    uint64_t seed,
                    bool k5) -> support::ShuffleResult
{
    auto originalSpan = notes;
    auto workingNotes = notes;
    if (k5) {
        notes[5].swap(notes[7]);
        workingNotes = notes.subspan(0, 6);
    }

    const auto result = support::generateBeatorajaLanePermutation(
      workingNotes, algorithm, static_cast<int64_t>(seed));

    if (k5) {
        notes[5].swap(notes[7]);
        notes = originalSpan;
    }
    return result;
}

auto
applyLr2Order(std::span<std::vector<charts::BmsNotesData::Note>>& notes,
              NoteOrderAlgorithm algorithm,
              support::Lr2Random& randomGenerator,
              bool k5) -> support::ShuffleResult
{
    auto originalSpan = notes;
    auto workingNotes = notes;
    if (k5) {
        notes[5].swap(notes[7]);
        workingNotes = notes.subspan(0, 6);
    }

    const auto result = support::generateLr2LanePermutation(
      workingNotes, algorithm, randomGenerator);

    if (k5) {
        notes[5].swap(notes[7]);
        notes = originalSpan;
    }
    return result;
}

auto
applyOrder(std::span<std::vector<charts::BmsNotesData::Note>>& notes,
           NoteOrderAlgorithm algorithm,
           uint64_t seed,
           bool k5,
           bool usePre130,
           support::Lr2Random* lr2RandomGenerator) -> support::ShuffleResult
{
    if (support::isBeatorajaNoteOrderAlgorithm(algorithm)) {
        return applyBeatorajaOrder(notes, algorithm, seed, k5);
    }
    if (support::isLr2NoteOrderAlgorithm(algorithm)) {
        if (lr2RandomGenerator == nullptr) {
            auto randomGenerator =
              support::Lr2Random{ static_cast<uint32_t>(seed) };
            return applyLr2Order(notes, algorithm, randomGenerator, k5);
        }
        return applyLr2Order(notes, algorithm, *lr2RandomGenerator, k5);
    }
    return support::generatePermutation(notes, algorithm, seed, k5, usePre130);
}

auto
getComponentsForPlayer(const ChartFactory::PlayerSpecificData& player,
                       const charts::BmsNotesData& notesData,
                       const gameplay_logic::ChartData& chartData,
                       const double maxHitValue,
                       DpOptions dpOptions,
                       const bool usePre130) -> RandomizedData
{
    auto visibleNotes = notesData.notes;
    const auto isDpFlip =
      dpOptions == DpOptions::Flip || dpOptions == DpOptions::Lr2Flip;
    if ((dpOptions == DpOptions::Battle && isDp(chartData.getKeymode())) ||
        (isDpFlip && !isDp(chartData.getKeymode()))) {
        dpOptions = DpOptions::Off;
    }
    auto keymode = chartData.getKeymode();
    if (dpOptions == DpOptions::Battle) {
        switch (keymode) {
            case gameplay_logic::ChartData::Keymode::K5:
                keymode = gameplay_logic::ChartData::Keymode::K10;
                break;
            case gameplay_logic::ChartData::Keymode::K7:
                keymode = gameplay_logic::ChartData::Keymode::K14;
                break;
        }
    }
    // We used to treat 5k as 7k. Reproduce that when generating replays.
    auto randomIs5k =
      !usePre130 && (keymode == gameplay_logic::ChartData::Keymode::K5 ||
                     keymode == gameplay_logic::ChartData::Keymode::K10);

    if (dpOptions == DpOptions::Flip) {
        support::flipBeatorajaDpPlayfields(visibleNotes);
    }
    if (dpOptions == DpOptions::Lr2Flip) {
        support::flipLr2DpPlayfields(visibleNotes);
    }
    if (dpOptions == DpOptions::Battle) {
        for (int i = 0; i < 7; i += 1) {
            visibleNotes[14 - i] = visibleNotes[i];
        }
        visibleNotes[15] = visibleNotes[7];
    }
    auto results = [&]() -> std::array<support::ShuffleResult, 2> {
        auto lr2RandomGenerator =
          support::isLr2NoteOrderAlgorithm(player.noteOrderAlgorithm) ||
              support::isLr2NoteOrderAlgorithm(player.noteOrderAlgorithmP2)
            ? std::optional<support::Lr2Random>{ static_cast<uint32_t>(
                player.randomSeed) }
            : std::nullopt;
        if (lr2RandomGenerator) {
            lr2RandomGenerator->discard(
              static_cast<std::size_t>(chartData.getRandomSequence().size()));
        }
        auto* lr2RandomGeneratorPtr =
          lr2RandomGenerator ? &*lr2RandomGenerator : nullptr;
        if (isDp(keymode)) {
            auto notes1 =
              std::span{ visibleNotes.data(), visibleNotes.size() / 2 };
            auto result1 = applyOrder(notes1,
                                      player.noteOrderAlgorithm,
                                      player.randomSeed,
                                      randomIs5k,
                                      usePre130,
                                      lr2RandomGeneratorPtr);
            auto notes2 =
              std::span{ visibleNotes.data() + visibleNotes.size() / 2,
                         visibleNotes.size() / 2 };
            auto result2 = applyOrder(notes2,
                                      player.noteOrderAlgorithmP2,
                                      result1.seed + 1,
                                      randomIs5k,
                                      usePre130,
                                      lr2RandomGeneratorPtr);
            return { result1, result2 };
        }
        auto notes1 = std::span{ visibleNotes.data(), visibleNotes.size() / 2 };
        return { applyOrder(notes1,
                            player.noteOrderAlgorithm,
                            player.randomSeed,
                            randomIs5k,
                            usePre130,
                            lr2RandomGeneratorPtr),
                 support::ShuffleResult{} };
    }();
    auto notes = ChartDataFactory::makeNotes(visibleNotes, notesData.barLines);
    auto guid = [&] {
        if (player.replayedScore) {
            return player.replayedScore->getResult()->getGuid();
        }
        if (player.autoPlay) {
            return QStringLiteral("");
        }
        return QUuid::createUuid().toString();
    }();
    auto multiplier = 1;
    if (dpOptions == DpOptions::Battle) {
        multiplier = 2;
    }
    auto score = std::make_unique<gameplay_logic::BmsLiveScore>(
      chartData.getNormalNoteCount() * multiplier,
      chartData.getScratchCount() * multiplier,
      chartData.getLnCount() * multiplier,
      chartData.getBssCount() * multiplier,
      chartData.getMineCount() * multiplier,
      (chartData.getLnCount() + chartData.getNormalNoteCount() +
       chartData.getBssCount() + chartData.getScratchCount()) *
        multiplier,
      maxHitValue,
      player.gauges,
      chartData.getRandomSequence(),
      player.noteOrderAlgorithm,
      isDp(keymode) ? player.noteOrderAlgorithmP2 : NoteOrderAlgorithm::Normal,
      dpOptions,
      results[0].columns + results[1].columns,
      results[0].seed,
      chartData.getLength(),
      chartData.getSha256(),
      chartData.getMd5(),
      keymode,
      player.replayedScore != nullptr
        ? player.replayedScore->getResult()->getUnixTimestamp()
        : 0,
      guid,
      player.replayedScore != nullptr
        ? player.replayedScore->getSubmissionState()
        : gameplay_logic::BmsScore::SubmissionState::NotSubmitted);
    auto notesStates = QList<gameplay_logic::ColumnState*>{};
    for (const auto& column : notes->getNotes()) {
        auto notes = QList<gameplay_logic::NoteState>{};
        notes.reserve(column.size());
        for (const auto& [i, note] : std::ranges::views::enumerate(column)) {
            notes.append({ note, i });
        }
        notesStates.append(new gameplay_logic::ColumnState(std::move(notes)));
    }
    auto barLineStates = QList<gameplay_logic::BarLineState>{};
    barLineStates.reserve(notes->getBarLines().size());
    for (const auto& [i, barLine] :
         std::ranges::views::enumerate(notes->getBarLines())) {
        barLineStates.append({ barLine, i });
    }
    auto* barLinesState =
      new gameplay_logic::BarLinesState(std::move(barLineStates));
    auto state = std::make_unique<gameplay_logic::GameplayState>(
      std::move(notesStates), barLinesState);
    return { std::move(notes),
             std::move(state),
             results,
             std::move(score),
             std::move(visibleNotes) };
}

auto
getLength(const gameplay_logic::BmsNotes& notes) -> std::chrono::nanoseconds
{
    auto max = int64_t{ 0 };
    for (const auto& column : notes.getNotes()) {
        for (const auto& note : column) {
            if (note.time.timestamp > max) {
                max = note.time.timestamp;
            }
        }
    }
    return std::chrono::nanoseconds{ max };
}
} // namespace

auto
ChartFactory::createChart(ChartDataFactory::ChartComponents chartComponents,
                          PlayerSpecificData player1,
                          std::optional<PlayerSpecificData> player2,
                          const double maxHitValue)
  -> std::unique_ptr<gameplay_logic::ChartRunner>
{
    auto& [chartData, notesData, wavs, bmps] = chartComponents;
    auto path = support::qStringToPath(chartData->getChartDirectory());
    pendingLoads.removeIf([](const auto& load) { return load.expired(); });
    auto cancellation = std::make_shared<std::stop_source>();
    pendingLoads.append(cancellation);
    const auto stop = cancellation->get_token();
    auto components1 = getComponentsForPlayer(player1,
                                              notesData,
                                              *chartData,
                                              maxHitValue,
                                              player1.dpOptions,
                                              player1.usePre130);
    auto components2 = player2.transform([&](auto& player) {
        return getComponentsForPlayer(player,
                                      notesData,
                                      *chartData,
                                      maxHitValue,
                                      DpOptions::Off,
                                      player.usePre130);
    });
    auto keymode = chartData->getKeymode();
    if (player1.dpOptions == DpOptions::Battle && !player2) {
        if (keymode == gameplay_logic::ChartData::Keymode::K5) {
            keymode = gameplay_logic::ChartData::Keymode::K10;
        } else if (keymode == gameplay_logic::ChartData::Keymode::K7) {
            keymode = gameplay_logic::ChartData::Keymode::K14;
        }
    }

    auto soundTask =
      std::make_unique<SoundTask>(engine,
                                  assetStore,
                                  path,
                                  std::move(wavs),
                                  std::move(notesData.bmsonSlices),
                                  std::move(notesData.bmsonFusions),
                                  stop);
    soundTask->moveToThread(nullptr);
    auto bgaTask = [bgaBase = std::move(notesData.bgaBase),
                    bgaPoor = std::move(notesData.bgaPoor),
                    bgaLayer = std::move(notesData.bgaLayer),
                    bgaLayer2 = std::move(notesData.bgaLayer2),
                    bmps = std::move(bmps),
                    path,
                    stop,
                    assetStore = assetStore]() mutable {
        try {
            return prepareBga({ std::move(bgaBase),
                                std::move(bgaLayer),
                                std::move(bgaLayer2),
                                std::move(bgaPoor) },
                              std::move(bmps),
                              path,
                              assetStore,
                              stop);
        } catch (const std::exception& error) {
            if (!stop.stop_requested()) {
                spdlog::warn("Could not load chart BGA: {}", error.what());
            }
            return BgaResources{};
        }
    };
    auto bga = QtConcurrent::run(&loadingPool, std::move(bgaTask))
                 .then(this, [stop](BgaResources resources) {
                     return finishBga(std::move(resources), stop);
                 });
    auto* player1Object = [&]() -> gameplay_logic::Player* {
        auto soundFuture =
          QtFuture::connect(soundTask.get(), &SoundTask::soundsLoaded);
        auto refereeFuture = soundFuture.then(
          [rawNotes = std::move(components1.rawNotes),
           hitRules = std::move(player1.hitRules),
           score = components1.score.get(),
           bpmChanges = notesData.bpmChanges,
           bgmNotes = std::move(notesData.bgmNotes)](
            std::unordered_map<uint64_t, std::shared_ptr<sounds::Sound>>
              sounds) mutable {
              std::shared_ptr<sounds::Sound> mineHitSound = nullptr;
              if (const auto sound = sounds.find(0); sound != sounds.end()) {
                  mineHitSound = sound->second;
              }
              return gameplay_logic::BmsGameReferee{
                  std::move(rawNotes), bgmNotes, bpmChanges,
                  mineHitSound,        score,    std::move(sounds),
                  std::move(hitRules)
              };
          });
        auto chartLength = getLength(*components1.notes);
        if (player1.replayedScore) {
            return new gameplay_logic::RePlayer{
                components1.notes.release(), components1.score.release(),
                components1.state.release(), player1.profile,
                std::move(refereeFuture),    chartLength,
                notesData.bpmChanges[0].bpm, player1.replayedScore,
            };
        }
        if (player1.autoPlay) {
            auto events = createAutoplayFromNotes(*components1.notes);
            return new gameplay_logic::AutoPlayer{
                components1.notes.release(), components1.score.release(),
                components1.state.release(), player1.profile,
                std::move(refereeFuture),    chartLength,
                notesData.bpmChanges[0].bpm, std::move(events),
            };
        }
        return new gameplay_logic::Player{
            components1.notes.release(), components1.score.release(),
            components1.state.release(), player1.profile,
            std::move(refereeFuture),    chartLength,
            notesData.bpmChanges[0].bpm,
        };
    }();
    auto player2Object =
      components2.transform([&](auto& player) -> gameplay_logic::Player* {
          auto soundFuture =
            QtFuture::connect(soundTask.get(), &SoundTask::soundsLoaded);
          auto refereeFuture = soundFuture.then(
            [rawNotes = std::move(player.rawNotes),
             hitRules = std::move(player2->hitRules),
             score = player.score.get(),
             bpmChanges = notesData.bpmChanges,
             bgmNotes = std::move(notesData.bgmNotes)](
              std::unordered_map<uint64_t, std::shared_ptr<sounds::Sound>>
                sounds) mutable {
                std::shared_ptr<sounds::Sound> mineHitSound = nullptr;
                if (const auto sound = sounds.find(0); sound != sounds.end()) {
                    mineHitSound = sound->second;
                }
                return gameplay_logic::BmsGameReferee{
                    std::move(rawNotes), {},    bpmChanges,
                    mineHitSound,        score, std::move(sounds),
                    std::move(hitRules)
                };
            });
          auto chartLength = getLength(*player.notes);
          if (player2->replayedScore) {
              return new gameplay_logic::RePlayer{
                  player.notes.release(),      player.score.release(),
                  player.state.release(),      player2->profile,
                  std::move(refereeFuture),    chartLength,
                  notesData.bpmChanges[0].bpm, player2->replayedScore,
              };
          }
          if (player2->autoPlay) {
              auto events = createAutoplayFromNotes(*player.notes);
              return new gameplay_logic::AutoPlayer{
                  player.notes.release(),      player.score.release(),
                  player.state.release(),      player2->profile,
                  std::move(refereeFuture),    chartLength,
                  notesData.bpmChanges[0].bpm, std::move(events),
              };
          }
          return new gameplay_logic::Player{
              player.notes.release(),     player.score.release(),
              player.state.release(),     player2->profile,
              std::move(refereeFuture),   chartLength,
              notesData.bpmChanges[0].bpm
          };
      });
    loadingPool.start([task = std::move(soundTask)] {
        task->moveToThread(QThread::currentThread());
        task->run();
    });
    auto chart = std::make_unique<gameplay_logic::ChartRunner>(
      chartData.release(),
      std::move(bga),
      keymode,
      player1Object,
      player2Object.value_or(nullptr));
    QObject::connect(chart.get(), &QObject::destroyed, [cancellation] {
        cancellation->request_stop();
    });
    QObject::connect(chart.get(),
                     &gameplay_logic::ChartRunner::statusChanged,
                     chart.get(),
                     [cancellation, runner = chart.get()] {
                         if (runner->getStatus() ==
                             gameplay_logic::ChartRunner::Finished) {
                             cancellation->request_stop();
                         }
                     });
    QObject::connect(
      inputTranslator,
      &input::InputTranslator::buttonPressed,
      chart.get(),
      [chart = chart.get()](const input::BmsKey button, const int64_t time) {
          chart->passKey(
            button, gameplay_logic::ChartRunner::EventType::KeyPress, time);
      });
    QObject::connect(
      inputTranslator,
      &input::InputTranslator::buttonReleased,
      chart.get(),
      [chart = chart.get()](input::BmsKey button, int64_t time) {
          chart->passKey(
            button, gameplay_logic::ChartRunner::EventType::KeyRelease, time);
      });
    return chart;
}
SoundTask::SoundTask(
  sounds::AudioEngine* engine,
  SongAssetStore* assetStore,
  std::filesystem::path path,
  std::unordered_map<uint64_t, std::filesystem::path> channelPaths,
  std::vector<charts::BmsNotesData::BmsonSliceInfo> slices,
  std::unordered_map<uint64_t, std::vector<uint64_t>> fusions,
  std::stop_token stop)
  : path(std::move(path))
  , wavs(std::move(channelPaths))
  , engine(engine)
  , assetStore(assetStore)
  , stop(stop)
  , bmsonSlices(std::move(slices))
  , bmsonFusions(std::move(fusions))
{
}

void
SoundTask::run()
{
    auto cancelled = std::atomic_bool{ stop.stop_requested() };
    std::stop_callback onStop(stop, [&] { cancelled.store(true); });
    auto sounds =
      std::unordered_map<uint64_t, std::shared_ptr<sounds::Sound>>{};
    try {
        if (!cancelled.load()) {
            if (assetStore->isVirtual(path)) {
                const auto encoded = charts::loadArchivedSoundData(
                  assetStore, path, wavs, &cancelled);
                sounds =
                  bmsonSlices.empty()
                    ? charts::loadBmsSounds(engine, encoded, &cancelled)
                    : charts::loadBmsonSounds(
                        engine, encoded, bmsonSlices, bmsonFusions, &cancelled);
            } else {
                sounds =
                  bmsonSlices.empty()
                    ? charts::loadBmsSounds(engine, wavs, path, &cancelled)
                    : charts::loadBmsonSounds(engine,
                                              wavs,
                                              bmsonSlices,
                                              bmsonFusions,
                                              path,
                                              &cancelled);
            }
        }
    } catch (const std::exception& error) {
        if (!cancelled.load()) {
            spdlog::warn("Could not load chart sounds: {}", error.what());
        }
    }
    if (cancelled.load()) {
        sounds.clear();
    }
    emit soundsLoaded(std::move(sounds));
}

ChartFactory::ChartFactory(sounds::AudioEngine* engine,
                           input::InputTranslator* inputTranslator,
                           SongAssetStore* assetStore)
  : engine(engine)
  , inputTranslator(inputTranslator)
  , assetStore(assetStore)
{
    connect(QCoreApplication::instance(),
            &QCoreApplication::aboutToQuit,
            this,
            &ChartFactory::cancelLoading);
}

ChartFactory::~ChartFactory()
{
    cancelLoading();
    loadingPool.waitForDone();
}

void
ChartFactory::cancelLoading()
{
    for (const auto& pending : pendingLoads) {
        if (const auto stop = pending.lock()) {
            stop->request_stop();
        }
    }
}
} // namespace resource_managers
