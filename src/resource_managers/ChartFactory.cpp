//
// Created by bobini on 24.08.23.
//

#include <QtConcurrent>
#include "ChartFactory.h"
#include "support/QStringToPath.h"
#include "support/PathToQString.h"
#include "support/UtfStringToPath.h"
#include <QImageReader>
#include <QVideoFrame>
#include <QApplication>

namespace resource_managers {

void
blackToTransparency(QImage& image)
{
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            auto pixel = image.pixel(x, y);
            if (qRed(pixel) == 0 && qGreen(pixel) == 0 && qBlue(pixel) == 0) {
                image.setPixel(x, y, qRgba(0, 0, 0, 0));
            }
        }
    }
}

auto
convertImageToFrame(const QImage& image) -> std::unique_ptr<QVideoFrame>
{
    auto frame = std::make_unique<QVideoFrame>(
      QVideoFrameFormat(QSize(256, 256), QVideoFrameFormat::Format_RGBA8888));
    frame->map(QVideoFrame::WriteOnly);
    std::copy(image.bits(), image.bits() + image.sizeInBytes(), frame->bits(0));
    frame->unmap();
    return frame;
}

auto
loadBmp(const std::filesystem::path& path) -> QImage
{
    if (!std::filesystem::exists(path)) {
        spdlog::warn("Bga file does not exist: {}", path.string());
        return {};
    }
    // check if it is a valid image
    auto pathQString = support::pathToQString(path);
    auto image = QImage(pathQString);
    if (image.isNull()) {
        spdlog::warn("Failed to load bga file: {}", path.string());
        return {};
    }
    image.convertTo(QImage::Format_RGBA8888);
    return image;
}

auto
loadBmpVideo(const std::filesystem::path& path) -> std::unique_ptr<QMediaPlayer>
{
    auto player = std::make_unique<QMediaPlayer>();
    auto pathQString = support::pathToQString(path);
    player->setSource(QUrl::fromLocalFile(pathQString));
    if (player->mediaStatus() == QMediaPlayer::InvalidMedia) {
        return nullptr;
    }
    return player;
};

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
      chartData->getNoteCount(), maxHitValue, std::move(gauges));
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
        return gameplay_logic::BmsGameReferee(std::move(visibleNotes),
                                              std::move(invisibleNotes),
                                              std::move(bgmNotes),
                                              std::move(bpmChanges),
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
        struct Request
        {
            std::string path;
            bool normal;
            bool makeTransparent;
        };

        auto requested = std::unordered_map<std::string, Request>{};
        for (auto& bmp : bmps) {
            requested.emplace(bmp.first,
                              Request(std::move(bmp.second), false, false));
        }

        for (const auto& bga : bgaBase) {
            if (auto entry = requested.find(bga.second);
                entry != requested.end()) {
                entry->second.normal = true;
            }
        }
        for (const auto& bga : bgaLayer) {
            if (auto entry = requested.find(bga.second);
                entry != requested.end()) {
                entry->second.makeTransparent = true;
            }
        }
        for (const auto& bga : bgaLayer2) {
            if (auto entry = requested.find(bga.second);
                entry != requested.end()) {
                entry->second.makeTransparent = true;
            }
        }
        for (const auto& bga : bgaPoor) {
            if (auto entry = requested.find(bga.second);
                entry != requested.end()) {
                entry->second.normal = true;
            }
        }

        struct FrameLoadingResult
        {
            std::string id;
            std::filesystem::path path;
            std::array<QVideoFrame*, 2> frames;
            bool normalRequested;
        };

        // load all images first
        auto loadedBgaFrames =
          QtConcurrent::blockingMapped<QList<FrameLoadingResult>>(
            requested, [path](auto bmp) -> FrameLoadingResult {
                auto filePath =
                  path / support::utfStringToPath(bmp.second.path);
                auto image = loadBmp(filePath);
                auto ret = FrameLoadingResult{
                    bmp.first, filePath, {}, bmp.second.normal
                };
                if (image.isNull()) {
                    return ret;
                }
                if (bmp.second.normal) {
                    ret.frames[0] = convertImageToFrame(image).release();
                }
                if (bmp.second.makeTransparent) {
                    auto image2 = QImage{};
                    if (bmp.second.normal) {
                        image2 = image.copy();
                    } else {
                        image2 = image;
                    }
                    blackToTransparency(image2);
                    ret.frames[1] = convertImageToFrame(image2).release();
                }
                return ret;
            });
        // create unordered_maps
        auto frames =
          std::unordered_map<std::string,
                             std::array<std::unique_ptr<QVideoFrame>, 2>>{};
        auto videos = std::unordered_map<std::string, QMediaPlayer*>{};
        for (auto& frame : loadedBgaFrames) {
            if (frame.frames[0] || frame.frames[1]) {
                frames.emplace(
                  frame.id,
                  std::array{ std::unique_ptr<QVideoFrame>(frame.frames[0]),
                              std::unique_ptr<QVideoFrame>(frame.frames[1]) });
            } else if (frame.normalRequested) {
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
                                        entry->second[0].get());
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
                                        entry->second[0].get());
            } else if (auto entry = videos.find(bga.second);
                       entry != videos.end()) {
                poorVideos.emplace_back(bga.first.timestamp, entry->second);
            } else {
                poorFrames.emplace_back(bga.first.timestamp, nullptr);
            }
        }

        auto layerFrames =
          std::vector<std::pair<std::chrono::nanoseconds, QVideoFrame*>>{};
        for (const auto& bga : bgaLayer) {
            if (auto entry = frames.find(bga.second); entry != frames.end()) {
                layerFrames.emplace_back(bga.first.timestamp,
                                         entry->second[1].get());
            } else {
                layerFrames.emplace_back(bga.first.timestamp, nullptr);
            }
        }
        auto layer2Frames =
          std::vector<std::pair<std::chrono::nanoseconds, QVideoFrame*>>{};
        for (const auto& bga : bgaLayer2) {
            if (auto entry = frames.find(bga.second); entry != frames.end()) {
                layer2Frames.emplace_back(bga.first.timestamp,
                                          entry->second[1].get());
            } else {
                layer2Frames.emplace_back(bga.first.timestamp, nullptr);
            }
        }

        auto bgas = QList<qml_components::Bga*>{};
        bgas.emplace_back(new qml_components::Bga(
          std::move(baseVideos), std::move(baseFrames), /*flushOnError=*/true));
        bgas.emplace_back(new qml_components::Bga(
          {}, std::move(layerFrames), /*flushOnError=*/false));
        bgas.emplace_back(new qml_components::Bga(
          {}, std::move(layer2Frames), /*flushOnError=*/false));
        bgas.emplace_back(new qml_components::Bga(
          std::move(poorVideos), std::move(poorFrames), /*flushOnError=*/true));

        // move resources to vectors
        auto videosVector = std::vector<QMediaPlayer*>{};
        videosVector.reserve(videos.size());
        for (auto& video : videos) {
            videosVector.emplace_back(video.second);
        }
        auto framesVector = std::vector<std::unique_ptr<QVideoFrame>>{};
        for (auto& frame : frames) {
            if (frame.second[0]) {
                framesVector.emplace_back(std::move(frame.second[0]));
            }
            if (frame.second[1]) {
                framesVector.emplace_back(std::move(frame.second[1]));
            }
        }
        auto bgaContainer = std::make_unique<qml_components::BgaContainer>(
          std::move(bgas), std::move(videosVector), std::move(framesVector));
        bgaContainer->moveToThread(thread);
        return bgaContainer;
    };

    auto bga = QtConcurrent::run(std::move(bgaTask));
    auto referee = QtConcurrent::run(std::move(task));
    return new gameplay_logic::Chart(std::move(referee),
                                     std::move(bga),
                                     chartData.release(),
                                     notes.release(),
                                     score,
                                     scoreDb);
}
ChartFactory::ChartFactory(std::function<db::SqliteCppDb&()> scoreDb)
  : scoreDb(std::move(scoreDb))
{
}
} // namespace resource_managers