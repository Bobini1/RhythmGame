//
// Created by bobini on 24.08.23.
//

#include <QtConcurrent>
#include <QObject>
#include "ChartFactory.h"

#include "charts/helper_functions/loadBmsSounds.h"
#include "support/QStringToPath.h"
#include "support/PathToQString.h"
#include "support/UtfStringToPath.h"
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
ChartFactory::createChart(
  ChartDataFactory::ChartComponents chartComponents,
  std::unique_ptr<gameplay_logic::rules::BmsHitRules> hitRules,
  QList<gameplay_logic::rules::BmsGauge*> gauges,
  double maxHitValue) -> gameplay_logic::Chart*
{
    auto& [chartData, notes, notesData, wavs, bmps] = chartComponents;
    auto path = support::qStringToPath(chartData->getPath()).parent_path();
    auto* score = new gameplay_logic::BmsScore(
      chartData->getNormalNoteCount(),
      chartData->getLnCount(),
      chartData->getMineCount(),
      chartData->getLnCount() + chartData->getNormalNoteCount(),
      maxHitValue,
      std::move(gauges));
    auto task = [path,
                 wavs = std::move(wavs),
                 visibleNotes = std::move(notesData.visibleNotes),
                 invisibleNotes = std::move(notesData.invisibleNotes),
                 bgmNotes = std::move(notesData.bgmNotes),
                 bpmChanges = std::move(notesData.bpmChanges),
                 score,
                 hitRules = std::move(hitRules)]() mutable {
        auto sounds =
          charts::helper_functions::loadBmsSounds(wavs, std::move(path));
        sounds::OpenALSound* mineHitSound = nullptr;
        if (auto sound = sounds.find(0); sound != sounds.end()) {
            mineHitSound = &sound->second;
        }
        return gameplay_logic::BmsGameReferee(std::move(visibleNotes),
                                              std::move(invisibleNotes),
                                              std::move(bgmNotes),
                                              std::move(bpmChanges),
                                              mineHitSound,
                                              score,
                                              std::move(sounds),
                                              std::move(hitRules));
    };
    auto bgaTask = [bgaBase = std::move(notesData.bgaBase),
                    bgaPoor = std::move(notesData.bgaPoor),
                    bgaLayer = std::move(notesData.bgaLayer),
                    bgaLayer2 = std::move(notesData.bgaLayer2),
                    bmps = std::move(bmps),
                    thread = QApplication::instance()->thread(),
                    path]() {
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
            if (auto entry = requested.find(bga.second);
                entry != requested.end()) {
                entry->second.requested = true;
            }
        }
        for (const auto& bga : bgaLayer) {
            if (auto entry = requested.find(bga.second);
                entry != requested.end()) {
                entry->second.requested = true;
            }
        }
        for (const auto& bga : bgaLayer2) {
            if (auto entry = requested.find(bga.second);
                entry != requested.end()) {
                entry->second.requested = true;
            }
        }
        for (const auto& bga : bgaPoor) {
            if (auto entry = requested.find(bga.second);
                entry != requested.end()) {
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
                auto filePath =
                  path / bmp.second.path;
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
        auto frames =
          std::unordered_map<uint16_t, std::unique_ptr<QVideoFrame>>{};
        auto videos = std::unordered_map<uint16_t, QMediaPlayer*>{};
        for (auto& frame : loadedBgaFrames) {
            if (frame.frame) {
                frames.emplace(frame.id,
                               std::unique_ptr<QVideoFrame>(frame.frame));
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
                baseFrames.emplace_back(bga.first.timestamp,
                                        entry->second.get());
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
                poorFrames.emplace_back(bga.first.timestamp,
                                        entry->second.get());
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
                layerFrames.emplace_back(bga.first.timestamp,
                                         entry->second.get());
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
                layer2Frames.emplace_back(bga.first.timestamp,
                                          entry->second.get());
            } else if (auto entry = videos.find(bga.second);
                       entry != videos.end()) {
                layer2Videos.emplace_back(bga.first.timestamp, entry->second);
            } else {
                layer2Frames.emplace_back(bga.first.timestamp, nullptr);
            }
        }

        auto bgas = QList<qml_components::Bga*>{};
        bgas.emplace_back(new qml_components::Bga(std::move(baseVideos),
                                                  std::move(baseFrames)));
        bgas.emplace_back(new qml_components::Bga(std::move(layerVideos),
                                                  std::move(layerFrames)));
        bgas.emplace_back(new qml_components::Bga(std::move(layer2Videos),
                                                  std::move(layer2Frames)));
        bgas.emplace_back(new qml_components::Bga(std::move(poorVideos),
                                                  std::move(poorFrames)));

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
    };

    auto bga = QtConcurrent::run(std::move(bgaTask));
    auto referee = QtConcurrent::run(std::move(task));
    auto* chart = new gameplay_logic::Chart(std::move(referee),
                                            std::move(bga),
                                            chartData.release(),
                                            notes.release(),
                                            score,
                                            scoreDb);
    QObject::connect(
      inputTranslator,
      &input::InputTranslator::buttonPressed,
      chart,
      [chart](input::BmsKey button, double /*value*/, int64_t time) {
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
ChartFactory::
ChartFactory(std::function<db::SqliteCppDb&()> scoreDb,
             input::InputTranslator* inputTranslator)
  : scoreDb(std::move(scoreDb))
  , inputTranslator(inputTranslator)
{
}
} // namespace resource_managers