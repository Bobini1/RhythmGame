#pragma once

#include <QObject>
#include <QPointer>
#include <QString>
#include <QStringView>
#include <QtTypes>

#include <atomic>
#include <chrono>
#include <deque>
#include <functional>
#include <mutex>

class QQuickWindow;
class QTimer;

struct RenderFrameMilestone
{
    QString capture;
    quint64 generation = 0;
    quint64 frameSequence = 0;
    quintptr contextHandle = 0;
    int contextAttributesResult = -1;
    int majorVersion = 0;
    int graphicsApi = 0;
    int captureFrameCount = 0;

    [[nodiscard]] bool hasWebGl2Evidence() const;
    [[nodiscard]] QString graphicsApiName() const;
};

class RenderProbe final : public QObject
{
public:
    using MilestoneCallback =
        std::function<void(const RenderFrameMilestone&)>;
    using CaptureCompletedCallback =
        std::function<void(const RenderFrameMilestone&)>;
    using FailureCallback =
        std::function<void(QStringView, QStringView)>;

    explicit RenderProbe(
        MilestoneCallback milestoneCallback,
        CaptureCompletedCallback captureCompletedCallback,
        FailureCallback failureCallback,
        QObject *parent = nullptr);

    void attachWindow(QQuickWindow *window);
    [[nodiscard]] quint64 beginPostMainCapture();
    [[nodiscard]] quint64 beginPhaseCapture(quint64 generation);
    [[nodiscard]] quint64 latestFrameSequence() const;

private:
    enum class CaptureKind : int
    {
        None = 0,
        PostMain = 1,
        Phase = 2,
    };

    struct AtomicFrame
    {
        quint64 sequence = 0;
        quint64 generation = 0;
        quintptr contextHandle = 0;
        int contextAttributesResult = -1;
        int majorVersion = 0;
        int graphicsApi = 0;
        int captureKind = 0;
    };

    [[nodiscard]] quint64 beginCapture(
        CaptureKind kind,
        quint64 generation);
    void recordRenderFrame(QQuickWindow *window);
    void consumeLatestFrame();
    void captureTimedOut();
    void scheduleNextCaptureFrame();
    [[nodiscard]] bool hasQueuedFrameForCaptureLocked(
        int captureKind,
        quint64 generation) const;
    [[nodiscard]] bool readNextFrame(AtomicFrame &frame);

    QPointer<QQuickWindow> m_window;
    QTimer *m_pollTimer = nullptr;
    QTimer *m_captureDeadlineTimer = nullptr;
    MilestoneCallback m_milestoneCallback;
    CaptureCompletedCallback m_captureCompletedCallback;
    FailureCallback m_failureCallback;

    static constexpr auto captureDeadline =
        std::chrono::seconds{5};

    CaptureKind m_guiCaptureKind = CaptureKind::None;
    quint64 m_guiCaptureGeneration = 0;
    quint64 m_guiCaptureBaseline = 0;
    quint64 m_lastPublishedFrame = 0;
    int m_guiCaptureFrameCount = 0;
    bool m_discardNextCaptureFrame = false;

    std::atomic<quint64> m_frameCounter{0};
    std::mutex m_publishedFramesMutex;
    std::deque<AtomicFrame> m_publishedFrames;
    std::atomic<quint64> m_activeCaptureGeneration{0};
    std::atomic<int> m_activeCaptureKind{
        static_cast<int>(CaptureKind::None)};
};
