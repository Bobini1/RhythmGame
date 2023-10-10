//
// Created by bobini on 10.10.23.
//

#include "Bga.h"
#include <QVideoFrameFormat>
qml_components::Bga::Bga(
  std::vector<std::pair<std::chrono::nanoseconds, QMediaPlayer*>> videoFiles,
  std::vector<std::pair<std::chrono::nanoseconds, QVideoFrame*>> images,
  QObject* parent)
  : QObject(parent)
  , videoFiles(std::move(videoFiles))
  , images(std::move(images))
{
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
        currentVideoFile->second->setVideoOutput(videoSink);
        currentVideoFile->second->play();
        currentVideoFile++;
    }
    while (currentImage != images.end() && currentImage->first < offset) {
        videoSink->setVideoFrame(*currentImage->second);
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
