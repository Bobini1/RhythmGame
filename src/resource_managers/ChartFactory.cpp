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

struct LoadedImages
{
    std::unordered_map<std::string, QMediaPlayer*> videos;
    std::unordered_map<std::string, std::unique_ptr<QVideoFrame>> images;
};

auto
loadImages(const std::map<std::string, std::string>& imagePaths,
           const std::filesystem::path& path,
           bool blackToTransparency) -> LoadedImages
{
    auto videos = std::unordered_map<std::string, QMediaPlayer*>{};
    auto images =
      std::unordered_map<std::string, std::unique_ptr<QVideoFrame>>{};
    for (const auto& [key, value] : imagePaths) {
        auto valuePath = support::utfStringToPath(value);
        auto fullPath = path / valuePath;
        // check if the file exists
        if (!std::filesystem::exists(fullPath)) {
            continue;
        }
        // check if it is a valid image
        auto pathQString = support::pathToQString(fullPath);
        auto image = QImage(pathQString);
        if (!image.isNull()) {
            auto frame = std::make_unique<QVideoFrame>(QVideoFrameFormat(
              QSize(256, 256), QVideoFrameFormat::Format_RGBA8888));
            frame->map(QVideoFrame::WriteOnly);
            image.convertTo(QImage::Format_RGBA8888);
            if (blackToTransparency) {
                for (int y = 0; y < image.height(); ++y) {
                    for (int x = 0; x < image.width(); ++x) {
                        auto pixel = image.pixel(x, y);
                        if (qRed(pixel) == 0 && qGreen(pixel) == 0 &&
                            qBlue(pixel) == 0) {
                            image.setPixel(x, y, qRgba(0, 0, 0, 0));
                        }
                    }
                }
            }
            std::copy(
              image.bits(), image.bits() + image.sizeInBytes(), frame->bits(0));
            frame->unmap();
            images.emplace(key, std::move(frame));
        } else {
            auto* player = new QMediaPlayer;
            player->setSource(QUrl::fromLocalFile(pathQString));
            videos.emplace(key, player);
        }
    }
    return { std::move(videos), std::move(images) };
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
        auto bga = QList<qml_components::Bga*>{};
        auto [videos, images] = loadImages(bmps, path, false);
        auto bgaBaseImages =
          std::vector<std::pair<std::chrono::nanoseconds, QVideoFrame*>>{};
        auto bgaBaseVideos =
          std::vector<std::pair<std::chrono::nanoseconds, QMediaPlayer*>>{};
        for (const auto& [time, key] : bgaBase) {
            if (auto it = images.find(key); it != images.end()) {
                bgaBaseImages.emplace_back(time.timestamp, it->second.get());
            } else if (auto it = videos.find(key); it != videos.end()) {
                bgaBaseVideos.emplace_back(time.timestamp, it->second);
            }
        }
        auto bgaPoorImages =
          std::vector<std::pair<std::chrono::nanoseconds, QVideoFrame*>>{};
        auto bgaPoorVideos =
          std::vector<std::pair<std::chrono::nanoseconds, QMediaPlayer*>>{};
        for (const auto& [time, key] : bgaPoor) {
            if (auto it = images.find(key); it != images.end()) {
                bgaPoorImages.emplace_back(time.timestamp, it->second.get());
            } else if (auto it = videos.find(key); it != videos.end()) {
                bgaPoorVideos.emplace_back(time.timestamp, it->second);
            }
        }
        auto bgaLayerImages =
          std::vector<std::pair<std::chrono::nanoseconds, QVideoFrame*>>{};
        auto bgaLayerVideos =
          std::vector<std::pair<std::chrono::nanoseconds, QMediaPlayer*>>{};
        for (const auto& [time, key] : bgaLayer) {
            if (auto it = images.find(key); it != images.end()) {
                bgaLayerImages.emplace_back(time.timestamp, it->second.get());
            } else if (auto it = videos.find(key); it != videos.end()) {
                bgaLayerVideos.emplace_back(time.timestamp, it->second);
            }
        }
        auto bgaLayer2Images =
          std::vector<std::pair<std::chrono::nanoseconds, QVideoFrame*>>{};
        auto bgaLayer2Videos =
          std::vector<std::pair<std::chrono::nanoseconds, QMediaPlayer*>>{};
        for (const auto& [time, key] : bgaLayer2) {
            if (auto it = images.find(key); it != images.end()) {
                bgaLayer2Images.emplace_back(time.timestamp, it->second.get());
            } else if (auto it = videos.find(key); it != videos.end()) {
                bgaLayer2Videos.emplace_back(time.timestamp, it->second);
            }
        }
        bga.append(new qml_components::Bga(std::move(bgaBaseVideos),
                                           std::move(bgaBaseImages)));
        bga.append(new qml_components::Bga(std::move(bgaPoorVideos),
                                           std::move(bgaPoorImages)));
        bga.append(new qml_components::Bga(std::move(bgaLayerVideos),
                                           std::move(bgaLayerImages)));
        bga.append(new qml_components::Bga(std::move(bgaLayer2Videos),
                                           std::move(bgaLayer2Images)));
        auto videos2 = std::vector<QMediaPlayer*>{};
        auto images2 = std::vector<std::unique_ptr<QVideoFrame>>{};
        for (auto& [key, value] : images) {
            images2.emplace_back(std::move(value));
        }
        for (const auto& [key, value] : videos) {
            videos2.emplace_back(value);
        }
        auto* bgaContainer = new qml_components::BgaContainer(
          bga, std::move(videos2), std::move(images2));
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