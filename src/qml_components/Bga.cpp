//
// Created by bobini on 10.10.23.
//

#include "Bga.h"
#include <QVideoFrameFormat>
qml_components::Bga::Bga(
  std::vector<std::pair<std::chrono::nanoseconds, QMediaPlayer*>> videoFiles,
  std::vector<std::pair<std::chrono::nanoseconds, QVideoFrame*>> images,
  bool flushOnError,
  QObject* parent)
  : QObject(parent)
  , videoFiles(std::move(videoFiles))
  , images(std::move(images))
  , flushOnError(flushOnError)
{
}

auto
getEmptyVideoFrame() -> QVideoFrame*
{
    static auto frame = []() {
        auto format = QVideoFrameFormat{ QSize{ 256, 256 },
                                         QVideoFrameFormat::Format_RGBA8888 };
        auto frame = QVideoFrame{ format };
        frame.map(QVideoFrame::WriteOnly);
        auto* data = frame.bits(0);
        for (int i = 0; i < 256 * 256; i++) {
            data[i * 4 + 0] = 0;
            data[i * 4 + 1] = 0;
            data[i * 4 + 2] = 0;
            data[i * 4 + 3] = 255;
        }
        frame.unmap();
        return frame;
    }();
    return &frame;
}

auto
getTransparentVideoFrame() -> QVideoFrame*
{
    static auto frame = []() {
        auto format = QVideoFrameFormat{ QSize{ 256, 256 },
                                         QVideoFrameFormat::Format_RGBA8888 };
        auto frame = QVideoFrame{ format };
        frame.map(QVideoFrame::WriteOnly);
        auto* data = frame.bits(0);
        for (int i = 0; i < 256 * 256; i++) {
            data[i * 4 + 0] = 0;
            data[i * 4 + 1] = 0;
            data[i * 4 + 2] = 0;
            data[i * 4 + 3] = 0;
        }
        frame.unmap();
        return frame;
    }();
    return &frame;
}

auto
qml_components::Bga::getVideoSink() const -> QVideoSink*
{
    return videoSink;
}
void
qml_components::Bga::setVideoSink(QVideoSink* newVideoSink)
{
    if (videoSink != newVideoSink) {
        videoSink = newVideoSink;
        emit videoSinkChanged();
    }
}
void
qml_components::Bga::update(std::chrono::nanoseconds offsetFromStart)
{
    if (videoSink == nullptr) {
        return;
    }
    auto offset = offsetFromStart;
    while (currentVideoFile != videoFiles.end() &&
           currentVideoFile->first < offset) {
        if (currentVideoFile->second->isSeekable()) {
            currentVideoFile->second->setPosition(
              (currentVideoFile->first - offset).count() / 1000000);
        }
        if (playedVideo != nullptr) {
            playedVideo->stop();
            playedVideo->setVideoOutput(nullptr);
        }
        currentVideoFile->second->setVideoOutput(videoSink);
        currentVideoFile->second->play();
        playedVideo = currentVideoFile->second;
        currentVideoFile++;
    }
    while (currentImage != images.end() && currentImage->first < offset) {
        if (playedVideo != nullptr) {
            playedVideo->stop();
            playedVideo->setVideoOutput(nullptr);
        }
        if (currentImage->second) {
            videoSink->setVideoFrame(*currentImage->second);
        } else if (flushOnError) {
            videoSink->setVideoFrame(*getEmptyVideoFrame());
        } else {
            videoSink->setVideoFrame(*getTransparentVideoFrame());
        }

        currentImage++;
    }
}
qml_components::BgaContainer::BgaContainer(
  QList<Bga*> layers,
  std::vector<QMediaPlayer*> videoFiles,
  std::vector<std::unique_ptr<QVideoFrame>> images,
  QObject* parent)
  : QObject(parent)
  , layers(std::move(layers))
  , videoFiles(std::move(videoFiles))
  , images(std::move(images))
{
    for (auto& bga : this->layers) {
        bga->setParent(this);
    }
    for (auto& videoFile : this->videoFiles) {
        videoFile->setParent(this);
    }
    getEmptyVideoFrame();
}
auto
qml_components::BgaContainer::getLayers() const -> const QList<Bga*>&
{
    return layers;
}
void
qml_components::BgaContainer::update(std::chrono::nanoseconds offsetFromStart)
{
    for (auto& bga : layers) {
        bga->update(offsetFromStart);
    }
}
