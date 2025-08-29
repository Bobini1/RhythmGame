//
// Created by bobini on 10.10.23.
//

#ifndef RHYTHMGAME_BGA_H
#define RHYTHMGAME_BGA_H

#include <QVideoSink>
#include <QMediaPlayer>
#include <QPixmap>
#include <QVideoFrame>
#include <qpointer.h>

/**
 * @brief Contains classes that wrap some functionality for use in QML.
 */
namespace qml_components {

/**
 * @brief A single BGA layer.
 * @details https://hitkey.nekokan.dyndns.info/cmds.htm#BMPXX
 * In QML, assign a QVideoSink to the videoSink property. When a chart is
 * played, it will update the bga and that will update the pixels in the
 * provided videoSink.
 */
class Bga : public QObject
{
    Q_OBJECT

    /**
     * @brief The video sink to output the BGA to.
     * @details Playback can be started by calling\
     * gameplay_logic::ChartRunner::start() on the ChartRunner
     * that owns theBGA.
     */
    Q_PROPERTY(QVideoSink* videoSink READ getVideoSink WRITE setVideoSink NOTIFY
                 videoSinkChanged)
    QPointer<QVideoSink> videoSink = nullptr;
    std::vector<std::pair<std::chrono::nanoseconds, QMediaPlayer*>> videoFiles;
    decltype(videoFiles)::iterator currentVideoFile = videoFiles.begin();
    std::vector<std::pair<std::chrono::nanoseconds, QVideoFrame*>> images;
    decltype(images)::iterator currentImage = images.begin();
    QMediaPlayer* playedVideo = nullptr;

  public:
    explicit Bga(
      std::vector<std::pair<std::chrono::nanoseconds, QMediaPlayer*>>
        videoFiles,
      std::vector<std::pair<std::chrono::nanoseconds, QVideoFrame*>> images,
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
