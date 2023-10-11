//
// Created by bobini on 10.10.23.
//

#ifndef RHYTHMGAME_BGA_H
#define RHYTHMGAME_BGA_H

#include <QObject>
#include <QtQmlIntegration>
#include <QVideoSink>
#include <QMediaPlayer>
#include <QPixmap>
#include <QVideoFrame>

namespace qml_components {

class Bga : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QVideoSink* videoSink READ getVideoSink WRITE setVideoSink NOTIFY
                 videoSinkChanged)
    QPointer<QVideoSink> videoSink = nullptr;
    std::vector<std::pair<std::chrono::nanoseconds, QMediaPlayer*>> videoFiles;
    decltype(videoFiles)::iterator currentVideoFile = videoFiles.begin();
    std::vector<std::pair<std::chrono::nanoseconds, QVideoFrame*>> images;
    decltype(images)::iterator currentImage = images.begin();
    QMediaPlayer* playedVideo = nullptr;
    bool flushOnError;

  public:
    explicit Bga(
      std::vector<std::pair<std::chrono::nanoseconds, QMediaPlayer*>>
        videoFiles,
      std::vector<std::pair<std::chrono::nanoseconds, QVideoFrame*>> images,
      bool flushOnError,
      QObject* parent = nullptr);
    auto getVideoSink() const -> QVideoSink*;
    void setVideoSink(QVideoSink* newVideoSink);
    void update(std::chrono::nanoseconds offsetFromStart);

  signals:
    void videoSinkChanged();
};

class BgaContainer : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QList<Bga*> layers READ getLayers CONSTANT)
    QList<Bga*> layers;

    std::vector<QMediaPlayer*> videoFiles;
    std::vector<std::unique_ptr<QVideoFrame>> images;

  public:
    BgaContainer(QList<Bga*> layers,
                 std::vector<QMediaPlayer*> videoFiles,
                 std::vector<std::unique_ptr<QVideoFrame>> images,
                 QObject* parent = nullptr);

    auto getLayers() const -> const QList<Bga*>&;
    void update(std::chrono::nanoseconds offsetFromStart);
};
} // namespace qml_components

#endif // RHYTHMGAME_BGA_H
