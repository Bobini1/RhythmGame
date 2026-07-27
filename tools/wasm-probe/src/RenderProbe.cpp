#include "RenderProbe.h"

#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QTimer>

#ifdef __EMSCRIPTEN__
#include <emscripten/html5.h>
#include <emscripten/html5_webgl.h>
#endif

#include <algorithm>
#include <utility>

bool RenderFrameMilestone::hasWebGl2Evidence() const
{
#ifdef __EMSCRIPTEN__
    return graphicsApi == static_cast<int>(QSGRendererInterface::OpenGL)
        && contextHandle != 0
        && contextAttributesResult == EMSCRIPTEN_RESULT_SUCCESS
        && majorVersion == 2;
#else
    return false;
#endif
}

QString RenderFrameMilestone::graphicsApiName() const
{
    return graphicsApi == static_cast<int>(QSGRendererInterface::OpenGL)
        ? QStringLiteral("OpenGL")
        : QStringLiteral("Other");
}

RenderProbe::RenderProbe(
    MilestoneCallback milestoneCallback,
    CaptureCompletedCallback captureCompletedCallback,
    FailureCallback failureCallback,
    QObject *parent)
    : QObject{parent}
    , m_pollTimer{new QTimer{this}}
    , m_captureDeadlineTimer{new QTimer{this}}
    , m_milestoneCallback{std::move(milestoneCallback)}
    , m_captureCompletedCallback{
          std::move(captureCompletedCallback)}
    , m_failureCallback{std::move(failureCallback)}
{
    m_pollTimer->setInterval(std::chrono::milliseconds{1});
    connect(
        m_pollTimer,
        &QTimer::timeout,
        this,
        &RenderProbe::consumeLatestFrame);
    m_captureDeadlineTimer->setSingleShot(true);
    connect(
        m_captureDeadlineTimer,
        &QTimer::timeout,
        this,
        &RenderProbe::captureTimedOut);
}

void RenderProbe::attachWindow(QQuickWindow *window)
{
    if (window == nullptr || !m_window.isNull()) {
        return;
    }
    m_window = window;
    connect(
        window,
        &QQuickWindow::beforeRendering,
        this,
        [this, window] {
            recordRenderFrame(window);
        },
        Qt::DirectConnection);
    window->update();
}

quint64 RenderProbe::beginPostMainCapture()
{
    return beginCapture(CaptureKind::PostMain, 0);
}

quint64 RenderProbe::beginPhaseCapture(quint64 generation)
{
    return beginCapture(CaptureKind::Phase, generation);
}

quint64 RenderProbe::latestFrameSequence() const
{
    return m_frameCounter.load(std::memory_order_acquire);
}

quint64 RenderProbe::beginCapture(
    CaptureKind kind,
    quint64 generation)
{
    m_guiCaptureKind = kind;
    m_guiCaptureGeneration = generation;
    m_guiCaptureFrameCount = 0;
    m_discardNextCaptureFrame = true;
    m_guiCaptureBaseline = latestFrameSequence();
    m_lastPublishedFrame = m_guiCaptureBaseline;
    {
        const std::lock_guard lock{m_publishedFramesMutex};
        m_publishedFrames.clear();
        m_activeCaptureGeneration.store(
            generation,
            std::memory_order_release);
        m_activeCaptureKind.store(
            static_cast<int>(kind),
            std::memory_order_release);
    }
    m_pollTimer->start();
    m_captureDeadlineTimer->start(captureDeadline);
    scheduleNextCaptureFrame();
    return m_guiCaptureBaseline;
}

void RenderProbe::recordRenderFrame(QQuickWindow *window)
{
    const quint64 frameSequence =
        m_frameCounter.fetch_add(1, std::memory_order_relaxed) + 1;
    const int captureKind =
        m_activeCaptureKind.load(std::memory_order_acquire);
    if (captureKind == static_cast<int>(CaptureKind::None)) {
        return;
    }
    const quint64 generation =
        m_activeCaptureGeneration.load(std::memory_order_acquire);

    // QSGRendererInterface::graphicsApi() is sampled only from this direct
    // scene-graph callback, while Qt's actual graphics context is current.
    const QSGRendererInterface::GraphicsApi graphicsApi =
        window->rendererInterface()->graphicsApi();

    quintptr contextHandle = 0;
    int contextAttributesResult = -1;
    int majorVersion = 0;
#ifdef __EMSCRIPTEN__
    const EMSCRIPTEN_WEBGL_CONTEXT_HANDLE currentContext =
        emscripten_webgl_get_current_context();
    EmscriptenWebGLContextAttributes attributes = {};
    contextHandle = static_cast<quintptr>(currentContext);
    if (currentContext != 0) {
        contextAttributesResult =
            emscripten_webgl_get_context_attributes(
                currentContext,
                &attributes);
        majorVersion = attributes.majorVersion;
    }
#endif

    const AtomicFrame frame{
        .sequence = frameSequence,
        .generation = generation,
        .contextHandle = contextHandle,
        .contextAttributesResult = contextAttributesResult,
        .majorVersion = majorVersion,
        .graphicsApi = static_cast<int>(graphicsApi),
        .captureKind = captureKind,
    };
    const std::lock_guard lock{m_publishedFramesMutex};
    if (m_activeCaptureGeneration.load(std::memory_order_acquire)
            != generation
        || m_activeCaptureKind.load(std::memory_order_acquire)
            != captureKind) {
        return;
    }
    m_publishedFrames.push_back(frame);
}

void RenderProbe::consumeLatestFrame()
{
    AtomicFrame frame;
    if (!readNextFrame(frame)) {
        scheduleNextCaptureFrame();
        return;
    }
    if (m_guiCaptureKind == CaptureKind::None
        || frame.captureKind != static_cast<int>(m_guiCaptureKind)
        || frame.generation != m_guiCaptureGeneration
        || frame.sequence <= m_guiCaptureBaseline
        || frame.sequence <= m_lastPublishedFrame) {
        return;
    }

    m_lastPublishedFrame = frame.sequence;
    if (m_discardNextCaptureFrame) {
        m_discardNextCaptureFrame = false;
        scheduleNextCaptureFrame();
        return;
    }

    ++m_guiCaptureFrameCount;
    RenderFrameMilestone milestone = {
        .capture = m_guiCaptureKind == CaptureKind::PostMain
            ? QStringLiteral("post-main")
            : QStringLiteral("phase"),
        .generation = frame.generation,
        .frameSequence = frame.sequence,
        .contextHandle = frame.contextHandle,
        .contextAttributesResult = frame.contextAttributesResult,
        .majorVersion = frame.majorVersion,
        .graphicsApi = frame.graphicsApi,
        .captureFrameCount = m_guiCaptureFrameCount,
    };
    const bool captureFinished = m_guiCaptureFrameCount >= 2;
    if (!captureFinished) {
        scheduleNextCaptureFrame();
    } else {
        // Finalize the old generation before publishing its terminal
        // milestone. Browser commands can re-enter through the JSPI bridge
        // while callbacks publish to JavaScript; no tail of this capture may
        // clear a newer generation started from that observation.
        {
            const std::lock_guard lock{m_publishedFramesMutex};
            m_publishedFrames.clear();
            m_activeCaptureKind.store(
                static_cast<int>(CaptureKind::None),
                std::memory_order_release);
            m_activeCaptureGeneration.store(
                0,
                std::memory_order_release);
        }
        m_guiCaptureKind = CaptureKind::None;
        m_pollTimer->stop();
        m_captureDeadlineTimer->stop();
    }

    m_milestoneCallback(milestone);
    if (captureFinished) {
        m_captureCompletedCallback(milestone);
    }
}

void RenderProbe::captureTimedOut()
{
    if (m_guiCaptureKind == CaptureKind::None) {
        return;
    }
    {
        const std::lock_guard lock{m_publishedFramesMutex};
        m_publishedFrames.clear();
        m_activeCaptureKind.store(
            static_cast<int>(CaptureKind::None),
            std::memory_order_release);
        m_activeCaptureGeneration.store(
            0,
            std::memory_order_release);
    }
    m_guiCaptureKind = CaptureKind::None;
    m_pollTimer->stop();
    if (m_failureCallback) {
        m_failureCallback(
            u"qt-render-capture-timeout",
            u"render capture did not publish two bounded frames");
    }
}

void RenderProbe::scheduleNextCaptureFrame()
{
    const QPointer<QQuickWindow> scheduledWindow = m_window;
    if (scheduledWindow.isNull()
        || m_guiCaptureKind == CaptureKind::None) {
        return;
    }
    const CaptureKind scheduledCaptureKind = m_guiCaptureKind;
    const int scheduledCaptureKindValue =
        static_cast<int>(scheduledCaptureKind);
    const quint64 scheduledCaptureGeneration = m_guiCaptureGeneration;
    {
        const std::lock_guard lock{m_publishedFramesMutex};
        if (hasQueuedFrameForCaptureLocked(
                scheduledCaptureKindValue,
                scheduledCaptureGeneration)
            || m_activeCaptureKind.load(
                   std::memory_order_acquire)
                != scheduledCaptureKindValue
            || m_activeCaptureGeneration.load(
                   std::memory_order_acquire)
                != scheduledCaptureGeneration) {
            return;
        }
    }

    // main() has intentionally returned, so a zero-delay queued meta-call can
    // be stranded by a nested JSPI native-event drain. Request the Qt frame
    // directly on the GUI thread; the Wasm compositor coalesces poll retries.
    scheduledWindow->update();
}

bool RenderProbe::hasQueuedFrameForCaptureLocked(
    int captureKind,
    quint64 generation) const
{
    return std::any_of(
        m_publishedFrames.cbegin(),
        m_publishedFrames.cend(),
        [captureKind, generation](const AtomicFrame &frame) {
            return frame.captureKind == captureKind
                && frame.generation == generation;
        });
}

bool RenderProbe::readNextFrame(AtomicFrame &frame)
{
    const std::lock_guard lock{m_publishedFramesMutex};
    if (m_publishedFrames.empty()) {
        return false;
    }
    frame = m_publishedFrames.front();
    m_publishedFrames.pop_front();
    return true;
}
