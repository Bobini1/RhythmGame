//
// Created by bobini on 17.08.23.
//

#include "ProgramSettings.h"

#include <algorithm>
#include <utility>
#include <QGuiApplication>
#include <QClipboard>
#include <QImage>
#include <QQuickWindow>

namespace qml_components {
ProgramSettings::ProgramSettings(QString avatarFolder,
                                 QString screenshotsFolder,
                                 QObject* parent)
  : QObject(parent)
  , avatarFolder(std::move(avatarFolder))
  , screenshotsFolder(std::move(screenshotsFolder))
{
    constexpr auto maximumFps = 1000;
    maxFps.store(
      std::clamp(settings.value("graphics/maxFps", 0).toInt(), 0, maximumFps),
      std::memory_order_relaxed);
}
auto
ProgramSettings::getAvatarFolder() const -> QString
{
    return avatarFolder;
}
auto
ProgramSettings::getScreenshotsFolder() const -> QString
{
    return screenshotsFolder;
}
auto
ProgramSettings::getMaxFps() const -> int
{
    return maxFps.load(std::memory_order_relaxed);
}
auto
ProgramSettings::getPresentationFps() const -> int
{
    return presentationFps.load(std::memory_order_relaxed);
}
auto
ProgramSettings::getContinuousRendering() const -> bool
{
    return continuousRendering.load(std::memory_order_relaxed);
}
void
ProgramSettings::setMaxFps(int value)
{
    constexpr auto maximumFps = 1000;
    value = std::clamp(value, 0, maximumFps);
    if (maxFps.exchange(value, std::memory_order_relaxed) == value) {
        return;
    }
    settings.setValue("graphics/maxFps", value);
    emit maxFpsChanged();
}
void
ProgramSettings::setContinuousRendering(const bool value)
{
    if (continuousRendering.exchange(value, std::memory_order_release) ==
        value) {
        return;
    }
    if (value && frameRateWindow) {
        frameRateWindow->update();
    }
    emit continuousRenderingChanged();
}
void
ProgramSettings::attachFrameRateLimiter(QQuickWindow* window)
{
    frameRateWindow = window;
    QObject::disconnect(frameLimiterConnection);
    QObject::disconnect(presentationCounterConnection);
    QObject::disconnect(continuousFrameRequestConnection);
    frameLimiterConnection = QObject::connect(window,
                                              &QQuickWindow::beforeRendering,
                                              this,
                                              &ProgramSettings::limitFrameRate,
                                              Qt::DirectConnection);
    presentationCounterConnection =
      QObject::connect(window,
                       &QQuickWindow::frameSwapped,
                       this,
                       &ProgramSettings::countPresentedFrame,
                       Qt::DirectConnection);
    // In the threaded render loop frameSwapped is emitted on the render thread.
    // Queue the next request to the window's GUI thread so QML state is
    // synchronized before another frame is rendered. Calling update() directly
    // from the render thread only requests a repaint of the last synchronized
    // scene and can keep presenting stale gameplay state while the GUI thread
    // is delayed.
    continuousFrameRequestConnection = QObject::connect(
      window,
      &QQuickWindow::frameSwapped,
      window,
      [this, window] {
          if (continuousRendering.load(std::memory_order_acquire)) {
              window->update();
          }
      },
      Qt::QueuedConnection);
}
void
ProgramSettings::limitFrameRate()
{
    frameRateLimiter.wait(maxFps.load(std::memory_order_relaxed));
}
void
ProgramSettings::countPresentedFrame()
{
    constexpr auto sampleInterval = std::chrono::milliseconds{ 500 };
    const auto now = std::chrono::steady_clock::now();
    if (presentationSampleStart == std::chrono::steady_clock::time_point{}) {
        presentationSampleStart = now;
        presentedFrameCount = 0;
        return;
    }

    ++presentedFrameCount;
    const auto elapsed = now - presentationSampleStart;
    if (elapsed < sampleInterval) {
        return;
    }

    const auto elapsedNanoseconds =
      std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count();
    const auto nextFps = static_cast<int>(
      (static_cast<std::int64_t>(presentedFrameCount) * 1'000'000'000LL +
       elapsedNanoseconds / 2) /
      elapsedNanoseconds);
    presentedFrameCount = 0;
    presentationSampleStart = now;
    if (presentationFps.exchange(nextFps, std::memory_order_relaxed) ==
        nextFps) {
        return;
    }
    QMetaObject::invokeMethod(
      this, [this] { emit presentationFpsChanged(); }, Qt::QueuedConnection);
}

auto
ProgramSettings::copyImageToClipboard(const QString& path) -> void
{
    QImage image(path);
    if (!image.isNull()) {
        QGuiApplication::clipboard()->setImage(image);
    }
}
} // namespace qml_components
