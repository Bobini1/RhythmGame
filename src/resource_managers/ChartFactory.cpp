//
// Created by bobini on 24.08.23.
//

#include <QtConcurrent>
#include <QObject>
#include "ChartFactory.h"

#include "charts/helper_functions/loadBmsSounds.h"
#include "qml_components/ProfileList.h"
#include "support/GeneratePermutation.h"
#include "support/QStringToPath.h"
#include "support/PathToQString.h"
#include <QImageReader>
#include <QVideoFrame>
#include <QApplication>

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
loadBmp(std::filesystem::path path) -> QImage
{
    auto original = path;
    // remove extension
    const auto pathQString = support::pathToQString(path.replace_extension());
    // QImage will try out a few different extensions
    auto image = QImage(pathQString);
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
loadBmpVideo(const std::filesystem::path& path) -> std::unique_ptr<QMediaPlayer>
{
    auto player = std::make_unique<QMediaPlayer>();
    QObject::connect(player.get(),
                     &QMediaPlayer::errorOccurred,
                     player.get(),
                     [](QMediaPlayer::Error error, const QString& errorString) {
                         spdlog::error("Error loading video: ({}) {}",
                                       (int)error,
                                       errorString.toStdString());
                     });
    const auto pathQString = support::pathToQString(path);
    player->setSource(QUrl::fromLocalFile(pathQString));
    if (player->mediaStatus() == QMediaPlayer::InvalidMedia) {
        return nullptr;
    }
    QEventLoop loop;
    QObject::connect(player.get(),
                     &QMediaPlayer::mediaStatusChanged,
                     &loop,
                     &QEventLoop::quit);
    if (player->mediaStatus() == QMediaPlayer::LoadingMedia) {
        loop.exec();
    }
    return player;
}

auto
loadBga(
  std::vector<std::pair<charts::gameplay_models::BmsNotesData::Time, uint16_t>>
    bgaBase,
  std::vector<std::pair<charts::gameplay_models::BmsNotesData::Time, uint16_t>>
    bgaPoor,
  std::vector<std::pair<charts::gameplay_models::BmsNotesData::Time, uint16_t>>
    bgaLayer,
  std::vector<std::pair<charts::gameplay_models::BmsNotesData::Time, uint16_t>>
    bgaLayer2,
  std::unordered_map<uint16_t, std::filesystem::path> bmps,
  QThread* thread,
  std::filesystem::path path) -> std::unique_ptr<qml_components::BgaContainer>
{
    auto start = std::chrono::high_resolution_clock::now();
    struct Request
    {
        std::filesystem::path path;
        bool requested;
    };

    auto requested = std::unordered_map<uint16_t, Request>{};
    for (auto& bmp : bmps) {
        requested.emplace(bmp.first, Request(std::move(bmp.second), false));
    }

    for (const auto& bga : bgaBase) {
        if (auto entry = requested.find(bga.second); entry != requested.end()) {
            entry->second.requested = true;
        }
    }
    for (const auto& bga : bgaLayer) {
        if (auto entry = requested.find(bga.second); entry != requested.end()) {
            entry->second.requested = true;
        }
    }
    for (const auto& bga : bgaLayer2) {
        if (auto entry = requested.find(bga.second); entry != requested.end()) {
            entry->second.requested = true;
        }
    }
    for (const auto& bga : bgaPoor) {
        if (auto entry = requested.find(bga.second); entry != requested.end()) {
            entry->second.requested = true;
        }
    }

    struct FrameLoadingResult
    {
        uint16_t id;
        std::filesystem::path path;
        QVideoFrame* frame;
    };

    // load all images first
    auto loadedBgaFrames =
      QtConcurrent::blockingMapped<QList<FrameLoadingResult>>(
        requested, [path](auto bmp) -> FrameLoadingResult {
            auto filePath = path / bmp.second.path;
            auto ret = FrameLoadingResult{ bmp.first, filePath, {} };
            if (!bmp.second.requested) {
                return ret;
            }
            auto image = loadBmp(filePath);
            if (image.isNull()) {
                return ret;
            }
            ret.frame = convertImageToFrame(image).release();
            return ret;
        });
    // create unordered_maps
    auto frames = std::unordered_map<uint16_t, std::unique_ptr<QVideoFrame>>{};
    auto videos = std::unordered_map<uint16_t, QMediaPlayer*>{};
    for (auto& frame : loadedBgaFrames) {
        if (frame.frame) {
            frames.emplace(frame.id, std::unique_ptr<QVideoFrame>(frame.frame));
        } else {
            auto video = loadBmpVideo(frame.path);
            if (video) {
                videos.emplace(frame.id, video.release());
            }
        }
    }

    auto baseFrames =
      std::vector<std::pair<std::chrono::nanoseconds, QVideoFrame*>>{};
    auto baseVideos =
      std::vector<std::pair<std::chrono::nanoseconds, QMediaPlayer*>>{};
    for (const auto& bga : bgaBase) {
        if (auto entry = frames.find(bga.second); entry != frames.end()) {
            baseFrames.emplace_back(bga.first.timestamp, entry->second.get());
        } else if (auto entry = videos.find(bga.second);
                   entry != videos.end()) {
            baseVideos.emplace_back(bga.first.timestamp, entry->second);
        } else {
            baseFrames.emplace_back(bga.first.timestamp, nullptr);
        }
    }

    auto poorFrames =
      std::vector<std::pair<std::chrono::nanoseconds, QVideoFrame*>>{};
    auto poorVideos =
      std::vector<std::pair<std::chrono::nanoseconds, QMediaPlayer*>>{};
    for (const auto& bga : bgaPoor) {
        if (auto entry = frames.find(bga.second); entry != frames.end()) {
            poorFrames.emplace_back(bga.first.timestamp, entry->second.get());
        } else if (auto entry = videos.find(bga.second);
                   entry != videos.end()) {
            poorVideos.emplace_back(bga.first.timestamp, entry->second);
        } else {
            poorFrames.emplace_back(bga.first.timestamp, nullptr);
        }
    }

    auto layerFrames =
      std::vector<std::pair<std::chrono::nanoseconds, QVideoFrame*>>{};
    auto layerVideos =
      std::vector<std::pair<std::chrono::nanoseconds, QMediaPlayer*>>{};
    for (const auto& bga : bgaLayer) {
        if (auto entry = frames.find(bga.second); entry != frames.end()) {
            layerFrames.emplace_back(bga.first.timestamp, entry->second.get());
        } else if (auto entry = videos.find(bga.second);
                   entry != videos.end()) {
            layerVideos.emplace_back(bga.first.timestamp, entry->second);
        } else {
            layerFrames.emplace_back(bga.first.timestamp, nullptr);
        }
    }
    auto layer2Frames =
      std::vector<std::pair<std::chrono::nanoseconds, QVideoFrame*>>{};
    auto layer2Videos =
      std::vector<std::pair<std::chrono::nanoseconds, QMediaPlayer*>>{};
    for (const auto& bga : bgaLayer2) {
        if (auto entry = frames.find(bga.second); entry != frames.end()) {
            layer2Frames.emplace_back(bga.first.timestamp, entry->second.get());
        } else if (auto entry = videos.find(bga.second);
                   entry != videos.end()) {
            layer2Videos.emplace_back(bga.first.timestamp, entry->second);
        } else {
            layer2Frames.emplace_back(bga.first.timestamp, nullptr);
        }
    }

    auto bgas = QList<qml_components::Bga*>{};
    bgas.emplace_back(
      new qml_components::Bga(std::move(baseVideos), std::move(baseFrames)));
    bgas.emplace_back(
      new qml_components::Bga(std::move(layerVideos), std::move(layerFrames)));
    bgas.emplace_back(new qml_components::Bga(std::move(layer2Videos),
                                              std::move(layer2Frames)));
    bgas.emplace_back(
      new qml_components::Bga(std::move(poorVideos), std::move(poorFrames)));

    // move resources to vectors
    auto videosVector = std::vector<QMediaPlayer*>{};
    videosVector.reserve(videos.size());
    for (auto& video : videos) {
        videosVector.emplace_back(video.second);
    }
    auto framesVector = std::vector<std::unique_ptr<QVideoFrame>>{};
    for (auto& frame : frames) {
        if (frame.second) {
            framesVector.emplace_back(std::move(frame.second));
        }
    }
    auto bgaContainer = std::make_unique<qml_components::BgaContainer>(
      std::move(bgas), std::move(videosVector), std::move(framesVector));
    bgaContainer->moveToThread(thread);

    auto end = std::chrono::high_resolution_clock::now();
    spdlog::info(
      "Loading {} images and {} videos took {} ms",
      frames.size(),
      videos.size(),
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
        .count());
    return bgaContainer;
}

struct RandomizedData
{
    std::unique_ptr<gameplay_logic::BmsNotes> notes;
    std::unique_ptr<gameplay_logic::GameplayState> state;
    std::array<support::ShuffleResult, 2> shuffleResults;
    std::unique_ptr<gameplay_logic::BmsScore> score;
    std::array<std::vector<charts::gameplay_models::BmsNotesData::Note>, 16>
      rawNotes;
};
namespace {
auto
getComponentsForPlayer(const ChartFactory::PlayerSpecificData& player,
                       const charts::gameplay_models::BmsNotesData& notesData,
                       const gameplay_logic::ChartData& chartData,
                       const double maxHitValue) -> RandomizedData
{
    auto visibleNotes = notesData.notes;
    auto results = [&]() -> std::array<support::ShuffleResult, 2> {
        if (isDp(chartData.getKeymode())) {
            auto notes1 =
              std::span{ visibleNotes.data(), visibleNotes.size() / 2 };
            auto result1 =
              support::generatePermutation(notes1,
                                           player.profile->getVars()
                                             ->getGlobalVars()
                                             ->getNoteOrderAlgorithm());
            auto notes2 =
              std::span{ visibleNotes.data() + visibleNotes.size() / 2,
                         visibleNotes.size() / 2 };
            auto result2 =
              support::generatePermutation(notes2,
                                           player.profile->getVars()
                                             ->getGlobalVars()
                                             ->getNoteOrderAlgorithmP2(),
                                           result1.seed + 1);
            return { result1, result2 };
        }
        auto notes1 = std::span{ visibleNotes.data(), visibleNotes.size() / 2 };
        return { support::generatePermutation(notes1,
                                              player.profile->getVars()
                                                ->getGlobalVars()
                                                ->getNoteOrderAlgorithm()),
                 support::ShuffleResult{} };
    }();
    // TODO: Simplify this. Don't convert bpmChanges twice for two players.
    auto notes = ChartDataFactory::makeNotes(
      visibleNotes, notesData.bpmChanges, notesData.barLines);
    auto score = std::make_unique<gameplay_logic::BmsScore>(
      chartData.getNormalNoteCount(),
      chartData.getLnCount(),
      chartData.getMineCount(),
      chartData.getLnCount() + chartData.getNormalNoteCount(),
      maxHitValue,
      player.gauges,
      chartData.getRandomSequence(),
      player.profile->getVars()->getGlobalVars()->getNoteOrderAlgorithm(),
      isDp(chartData.getKeymode())
        ? player.profile->getVars()->getGlobalVars()->getNoteOrderAlgorithmP2()
        : NoteOrderAlgorithm::Normal,
      results[0].columns += results[1].columns,
      results[0].seed,
      chartData.getSha256(),
      chartData.getMd5());
    auto notesStates = QList<gameplay_logic::ColumnState*>{};
    for (const auto& column : notes->getNotes()) {
        auto notes = QList<gameplay_logic::NoteState>{};
        notes.reserve(column.size());
        for (const auto& note : column) {
            notes.append({ note });
        }
        notesStates.append(new gameplay_logic::ColumnState(std::move(notes)));
    }
    auto barLineStates = QList<gameplay_logic::BarLineState>{};
    barLineStates.reserve(notes->getBarLines().size());
    for (const auto& barLine : notes->getBarLines()) {
        barLineStates.append({ barLine });
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
} // namespace

auto
ChartFactory::createChart(ChartDataFactory::ChartComponents chartComponents,
                          PlayerSpecificData player1,
                          std::optional<PlayerSpecificData> player2,
                          const double maxHitValue) -> gameplay_logic::Chart*
{
    auto& [chartData, notesData, wavs, bmps] = chartComponents;
    auto path = support::qStringToPath(chartData->getPath()).parent_path();
    auto components1 =
      getComponentsForPlayer(player1, notesData, *chartData, maxHitValue);
    auto components2 = player2.transform([&](auto& player) {
        return getComponentsForPlayer(
          player, notesData, *chartData, maxHitValue);
    });

    auto soundTask = [path, wavs = std::move(wavs)] {
        return charts::helper_functions::loadBmsSounds(wavs, path);
    };
    auto bgaTask = [bgaBase = std::move(notesData.bgaBase),
                    bgaPoor = std::move(notesData.bgaPoor),
                    bgaLayer = std::move(notesData.bgaLayer),
                    bgaLayer2 = std::move(notesData.bgaLayer2),
                    bmps = std::move(bmps),
                    thread = QApplication::instance()->thread(),
                    path]() mutable {
        return loadBga(std::move(bgaBase),
                       std::move(bgaPoor),
                       std::move(bgaLayer),
                       std::move(bgaLayer2),
                       std::move(bmps),
                       thread,
                       std::move(path));
    };
    auto bga = QtConcurrent::run(std::move(bgaTask));
    auto soundFuture = QtConcurrent::run(std::move(soundTask));
    auto* chart = new gameplay_logic::Chart(
      chartData.release(),
      std::move(bga),
      new gameplay_logic::Player{
        components1.notes.release(),
        components1.score.release(),
        components1.state.release(),
        player1.profile,
        soundFuture.then(
          [rawNotes = std::move(components1.rawNotes),
           hitRules = std::move(player1.hitRules),
           score = components1.score.get(),
           bpmChanges = notesData.bpmChanges,
           bgmNotes = std::move(notesData.bgmNotes)](
            std::unordered_map<uint16_t, sounds::OpenALSound> sounds) mutable {
              sounds::OpenALSound* mineHitSound = nullptr;
              if (const auto sound = sounds.find(0); sound != sounds.end()) {
                  // there will be awful bugs if you insert anything into the
                  // map after this point
                  mineHitSound = &sound->second;
              }
              return gameplay_logic::BmsGameReferee{ std::move(rawNotes),
                                                     std::move(bgmNotes),
                                                     bpmChanges,
                                                     mineHitSound,
                                                     score,
                                                     std::move(sounds),
                                                     std::move(hitRules) };
          }) },
      components2
        .transform([&](auto& player) {
            return new gameplay_logic::Player{
                player.notes.release(),
                player.score.release(),
                player.state.release(),
                player2->profile,
                soundFuture.then(
                  [rawNotes = std::move(components2->rawNotes),
                   hitRules = std::move(player2->hitRules),
                   score = components2->score.get(),
                   bpmChanges = notesData.bpmChanges,
                   bgmNotes = std::move(notesData.bgmNotes)](
                    std::unordered_map<uint16_t, sounds::OpenALSound>
                      sounds) mutable {
                      sounds::OpenALSound* mineHitSound = nullptr;
                      if (const auto sound = sounds.find(0);
                          sound != sounds.end()) {
                          mineHitSound = &sound->second;
                      }
                      return gameplay_logic::BmsGameReferee{
                          std::move(rawNotes), {},    bpmChanges,
                          mineHitSound,        score, std::move(sounds),
                          std::move(hitRules)
                      };
                  })
            };
        })
        .value_or(nullptr));
    QObject::connect(
      inputTranslator,
      &input::InputTranslator::buttonPressed,
      chart,
      [chart](
        const input::BmsKey button, double /*value*/, const int64_t time) {
          chart->passKey(
            button, gameplay_logic::Chart::EventType::KeyPress, time);
      });
    QObject::connect(
      inputTranslator,
      &input::InputTranslator::buttonReleased,
      chart,
      [chart](input::BmsKey button, int64_t time) {
          chart->passKey(
            button, gameplay_logic::Chart::EventType::KeyRelease, time);
      });
    return chart;
}
ChartFactory::ChartFactory(input::InputTranslator* inputTranslator)
  : inputTranslator(inputTranslator)
{
}
} // namespace resource_managers