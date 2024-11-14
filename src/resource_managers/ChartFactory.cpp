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

auto
ChartFactory::createChart(
  ChartDataFactory::ChartComponents chartComponents,
  std::vector<std::unique_ptr<gameplay_logic::rules::BmsHitRules>> hitRules,
  std::vector<QList<gameplay_logic::rules::BmsGauge*>> gauges,
  const QList<Profile*>& profiles,
  double maxHitValue) -> gameplay_logic::Chart*
{
    auto& [chartData, notesData, wavs, bmps] = chartComponents;
    auto path = support::qStringToPath(chartData->getPath()).parent_path();
    auto scores = QList<gameplay_logic::BmsScore*>{};
    auto notes = QList<gameplay_logic::BmsNotes*>{};
    auto visibleNotes = std::vector<decltype(notesData.visibleNotes)>{};
    auto invisibleNotes = std::vector<decltype(notesData.invisibleNotes)>{};
    for (auto playerIndex = 0; playerIndex < profiles.size(); playerIndex++) {
        visibleNotes.emplace_back(notesData.visibleNotes);
        invisibleNotes.emplace_back(notesData.invisibleNotes);
        auto results = [&] {
            if (isDp(chartData->getKeymode())) {
                auto visibleNotes1 =
                  std::span{ visibleNotes.back().data(),
                             visibleNotes.back().size() / 2 };
                auto invisibleNotes1 =
                  std::span{ invisibleNotes.back().data(),
                             invisibleNotes.back().size() / 2 };
                auto result1 =
                  support::generatePermutation(visibleNotes1,
                                               invisibleNotes1,
                                               profiles[playerIndex]
                                                 ->getVars()
                                                 ->getGlobalVars()
                                                 ->getNoteOrderAlgorithm());
                auto visibleNotes2 =
                  std::span{ visibleNotes.back().data() +
                               visibleNotes.back().size() / 2,
                             visibleNotes.back().size() / 2 };
                auto invisibleNotes2 =
                  std::span{ invisibleNotes.back().data() +
                               invisibleNotes.back().size() / 2,
                             invisibleNotes.back().size() / 2 };
                auto result2 =
                  support::generatePermutation(visibleNotes2,
                                               invisibleNotes2,
                                               profiles[playerIndex]
                                                 ->getVars()
                                                 ->getGlobalVars()
                                                 ->getNoteOrderAlgorithmP2(),
                                               result1.seed + 1);
                return std::pair{ result1, result2 };
            }
            auto visibleNotes1 = std::span{ visibleNotes.back().data(),
                                            visibleNotes.back().size() };
            auto invisibleNotes1 = std::span{ invisibleNotes.back().data(),
                                              invisibleNotes.back().size() };
            return std::pair{ support::generatePermutation(
                                visibleNotes1,
                                invisibleNotes1,
                                profiles[playerIndex]
                                  ->getVars()
                                  ->getGlobalVars()
                                  ->getNoteOrderAlgorithm()),
                              support::ShuffleResult{} };
        }();
        // TODO: Simplify this. Don't convert bpmChanges in a loop.
        notes.emplace_back(ChartDataFactory::makeNotes(visibleNotes.back(),
                                                       invisibleNotes.back(),
                                                       notesData.bpmChanges,
                                                       notesData.barLines)
                             .release());
        scores.emplace_back(new gameplay_logic::BmsScore(
          chartData->getNormalNoteCount(),
          chartData->getLnCount(),
          chartData->getMineCount(),
          chartData->getLnCount() + chartData->getNormalNoteCount(),
          maxHitValue,
          std::move(gauges[playerIndex]),
          chartData->getRandomSequence(),
          profiles[playerIndex]
            ->getVars()
            ->getGlobalVars()
            ->getNoteOrderAlgorithm(),
          isDp(chartData->getKeymode()) ? profiles[playerIndex]
                                            ->getVars()
                                            ->getGlobalVars()
                                            ->getNoteOrderAlgorithmP2()
                                        : NoteOrderAlgorithm::Normal,
          results.first.columns += results.second.columns,
          results.first.seed,
          chartData->getSha256().toStdString()));
    }

    auto task = [path,
                 wavs = std::move(wavs),
                 visibleNotes = std::move(visibleNotes),
                 invisibleNotes = std::move(invisibleNotes),
                 bgmNotes = std::move(notesData.bgmNotes),
                 bpmChanges = std::move(notesData.bpmChanges),
                 scores,
                 hitRules = std::move(hitRules)]() mutable {
        auto sounds = charts::helper_functions::loadBmsSounds(wavs, path);
        sounds::OpenALSound* mineHitSound = nullptr;
        if (const auto sound = sounds.find(0); sound != sounds.end()) {
            mineHitSound = &sound->second;
        }
        auto referees = std::vector<gameplay_logic::BmsGameReferee>{};
        for (auto playerIndex = 0; playerIndex < scores.size(); playerIndex++) {
            referees.emplace_back(std::move(visibleNotes[playerIndex]),
                                  std::move(invisibleNotes[playerIndex]),
                                  playerIndex == 0 ? std::move(bgmNotes)
                                                   : decltype(bgmNotes){},
                                  bpmChanges,
                                  mineHitSound,
                                  scores[playerIndex],
                                  sounds,
                                  std::move(hitRules[playerIndex]));
        }
        return referees;
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
    auto referees = QtConcurrent::run(std::move(task));
    auto* chart = new gameplay_logic::Chart(std::move(referees),
                                            std::move(bga),
                                            chartData.release(),
                                            std::move(notes),
                                            scores,
                                            profiles);
    for (auto playerIndex = 0; playerIndex < profiles.size(); playerIndex++) {
        const auto* const inputTranslator =
          profiles[playerIndex]->getInputTranslator();
        QObject::connect(
          inputTranslator,
          &input::InputTranslator::buttonPressed,
          chart,
          [chart, playerIndex](
            const input::BmsKey button, double /*value*/, const int64_t time) {
              chart->passKey(playerIndex,
                             button,
                             gameplay_logic::Chart::EventType::KeyPress,
                             time);
          });
        QObject::connect(
          inputTranslator,
          &input::InputTranslator::buttonReleased,
          chart,
          [chart, playerIndex](input::BmsKey button, int64_t time) {
              chart->passKey(playerIndex,
                             button,
                             gameplay_logic::Chart::EventType::KeyRelease,
                             time);
          });
    }
    return chart;
}
ChartFactory::
ChartFactory(qml_components::ProfileList* profileList)
  : profileList(profileList)
{
}
} // namespace resource_managers