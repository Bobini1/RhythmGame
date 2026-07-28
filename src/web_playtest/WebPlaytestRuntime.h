#pragma once

#include "GameplayWorker.h"
#include "WebPlaytestSnapshot.h"
#include "audio/EmscriptenAudioWorklet.h"

#include <QAbstractItemModel>
#include <QObject>
#include <QString>
#include <QTimer>

#include <cstdint>
#include <thread>

#if defined(__EMSCRIPTEN__)
#include <emscripten/html5.h>
#endif

namespace web_playtest {

class WebPlaytestRuntime final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int phase READ phase NOTIFY phaseChanged)
    Q_PROPERTY(QString phaseText READ phaseText NOTIFY phaseChanged)
    Q_PROPERTY(bool canStart READ canStart NOTIFY phaseChanged)
    Q_PROPERTY(bool canAbort READ canAbort NOTIFY phaseChanged)
    Q_PROPERTY(bool startPending READ startPending NOTIFY phaseChanged)
    Q_PROPERTY(int inputPreset READ inputPreset WRITE setInputPreset NOTIFY
                 inputPresetChanged)
    Q_PROPERTY(
      bool inputPresetLocked READ inputPresetLocked NOTIFY inputPresetChanged)
    Q_PROPERTY(QAbstractItemModel* noteModel READ noteModel CONSTANT)
    Q_PROPERTY(QString chartTitle READ chartTitle NOTIFY metadataChanged)
    Q_PROPERTY(QString chartArtist READ chartArtist NOTIFY metadataChanged)
    Q_PROPERTY(double chartBpm READ chartBpm NOTIFY metadataChanged)
    Q_PROPERTY(int decodedAssets READ decodedAssets NOTIFY loadProgressChanged)
    Q_PROPERTY(int totalAssets READ totalAssets NOTIFY loadProgressChanged)
    Q_PROPERTY(
      double countdownSeconds READ countdownSeconds NOTIFY snapshotChanged)
    Q_PROPERTY(double elapsedSeconds READ elapsedSeconds NOTIFY snapshotChanged)
    Q_PROPERTY(double currentScrollPosition READ currentScrollPosition NOTIFY
                 snapshotChanged)
    Q_PROPERTY(double score READ score NOTIFY snapshotChanged)
    Q_PROPERTY(double maxScoreNow READ maxScoreNow NOTIFY snapshotChanged)
    Q_PROPERTY(double gauge READ gauge NOTIFY snapshotChanged)
    Q_PROPERTY(int combo READ combo NOTIFY snapshotChanged)
    Q_PROPERTY(int pressedLaneMask READ pressedLaneMask NOTIFY snapshotChanged)
    Q_PROPERTY(
      QString latestJudgement READ latestJudgement NOTIFY snapshotChanged)
    Q_PROPERTY(
      double latestDeviationMs READ latestDeviationMs NOTIFY snapshotChanged)
    Q_PROPERTY(bool inputLatencyAvailable READ inputLatencyAvailable NOTIFY
                 snapshotChanged)
    Q_PROPERTY(double inputLatencyMs READ inputLatencyMs NOTIFY snapshotChanged)
    Q_PROPERTY(
      double lateInputClampMs READ lateInputClampMs NOTIFY snapshotChanged)
    Q_PROPERTY(qulonglong lateFrames READ lateFrames NOTIFY snapshotChanged)
    Q_PROPERTY(
      qulonglong droppedInputs READ droppedInputs NOTIFY snapshotChanged)
    Q_PROPERTY(int activeVoices READ activeVoices NOTIFY snapshotChanged)
    Q_PROPERTY(bool underrunTelemetryAvailable READ underrunTelemetryAvailable
                 NOTIFY telemetryChanged)
    Q_PROPERTY(qulonglong underruns READ underruns NOTIFY telemetryChanged)
    Q_PROPERTY(qulonglong heapBytes READ heapBytes NOTIFY telemetryChanged)
    Q_PROPERTY(
      qulonglong readyHeapBytes READ readyHeapBytes NOTIFY telemetryChanged)
    Q_PROPERTY(QString terminalError READ terminalError NOTIFY phaseChanged)

  public:
    [[nodiscard]] static auto createProcessLifetime(
      QString installedChartPath,
      QString initializationError,
      QObject* qmlParent = nullptr) -> WebPlaytestRuntime*;

    [[nodiscard]] auto phase() const noexcept -> int;
    [[nodiscard]] auto phaseText() const -> QString;
    [[nodiscard]] auto canStart() const noexcept -> bool;
    [[nodiscard]] auto canAbort() const noexcept -> bool;
    [[nodiscard]] auto startPending() const noexcept -> bool;
    [[nodiscard]] auto inputPreset() const noexcept -> int;
    void setInputPreset(int preset);
    [[nodiscard]] auto inputPresetLocked() const noexcept -> bool;
    [[nodiscard]] auto noteModel() noexcept -> QAbstractItemModel*;
    [[nodiscard]] auto chartTitle() const -> QString;
    [[nodiscard]] auto chartArtist() const -> QString;
    [[nodiscard]] auto chartBpm() const noexcept -> double;
    [[nodiscard]] auto decodedAssets() const noexcept -> int;
    [[nodiscard]] auto totalAssets() const noexcept -> int;
    [[nodiscard]] auto countdownSeconds() const noexcept -> double;
    [[nodiscard]] auto elapsedSeconds() const noexcept -> double;
    [[nodiscard]] auto currentScrollPosition() const noexcept -> double;
    [[nodiscard]] auto score() const noexcept -> double;
    [[nodiscard]] auto maxScoreNow() const noexcept -> double;
    [[nodiscard]] auto gauge() const noexcept -> double;
    [[nodiscard]] auto combo() const noexcept -> int;
    [[nodiscard]] auto pressedLaneMask() const noexcept -> int;
    [[nodiscard]] auto latestJudgement() const -> QString;
    [[nodiscard]] auto latestDeviationMs() const noexcept -> double;
    [[nodiscard]] auto inputLatencyAvailable() const noexcept -> bool;
    [[nodiscard]] auto inputLatencyMs() const noexcept -> double;
    [[nodiscard]] auto lateInputClampMs() const noexcept -> double;
    [[nodiscard]] auto lateFrames() const noexcept -> qulonglong;
    [[nodiscard]] auto droppedInputs() const noexcept -> qulonglong;
    [[nodiscard]] auto activeVoices() const noexcept -> int;
    [[nodiscard]] auto underrunTelemetryAvailable() const noexcept -> bool;
    [[nodiscard]] auto underruns() const noexcept -> qulonglong;
    [[nodiscard]] auto heapBytes() const noexcept -> qulonglong;
    [[nodiscard]] auto readyHeapBytes() const noexcept -> qulonglong;
    [[nodiscard]] auto terminalError() const -> QString;

    Q_INVOKABLE void startFromTrustedGesture();
    Q_INVOKABLE void abort();
    Q_INVOKABLE void refreshSnapshot();

  signals:
    void phaseChanged();
    void inputPresetChanged();
    void metadataChanged();
    void loadProgressChanged();
    void snapshotChanged();
    void telemetryChanged();

  private:
    explicit WebPlaytestRuntime(QString installedChartPath,
                                QString initializationError,
                                QObject* qmlParent);

    void poll();
    void initializeDecodedAudio();
    void sealReadyHeap();
    void synchronizePhase();
    void setTerminalError(QString error, bool notifyWorker);
    [[nodiscard]] auto enqueueTick() noexcept -> bool;
    [[nodiscard]] auto enqueueInput(const BrowserKeyTransition& transition,
                                    std::int64_t browserMonotonicUs) noexcept
      -> bool;
    void synthesizeReleases(std::int64_t browserMonotonicUs) noexcept;
    [[nodiscard]] auto browserMonotonicNowUs() const noexcept -> std::int64_t;
    [[nodiscard]] static auto judgementName(
      const std::optional<gameplay_logic::Judgement>& judgement) -> QString;

#if defined(__EMSCRIPTEN__)
    [[nodiscard]] static auto onKeyboard(int eventType,
                                         const EmscriptenKeyboardEvent* event,
                                         void* userData) noexcept -> EM_BOOL;
    [[nodiscard]] static auto onBlur(int eventType,
                                     const EmscriptenFocusEvent* event,
                                     void* userData) noexcept -> EM_BOOL;
    [[nodiscard]] static auto onVisibilityChange(
      int eventType,
      const EmscriptenVisibilityChangeEvent* event,
      void* userData) noexcept -> EM_BOOL;
    [[nodiscard]] auto handleKeyboard(
      int eventType,
      const EmscriptenKeyboardEvent& event) noexcept -> EM_BOOL;
    [[nodiscard]] auto installBrowserInputCallbacks() noexcept -> bool;
#endif

    EmscriptenAudioWorklet* worklet = {};
    GameplayWorker* worker = {};
    std::thread* gameplayThread = {};
    WebPlaytestNoteModel visibleNotes;
    QTimer pollTimer;
    BrowserInputDeduplicator inputDeduplicator;

    RuntimePhase displayedPhase = RuntimePhase::InstallingChart;
    InputPreset selectedInputPreset = InputPreset::Native;
    QString errorMessage;
    QString title;
    QString artist;
    double initialBpm = {};
    int lastDecodedAssets = {};
    int lastTotalAssets = {};

    double snapshotCountdownSeconds = {};
    double snapshotElapsedSeconds = {};
    double snapshotScrollPosition = {};
    double snapshotScore = {};
    double snapshotMaxScoreNow = {};
    double snapshotGauge = {};
    int snapshotCombo = {};
    int snapshotPressedLaneMask = {};
    QString snapshotLatestJudgement;
    double snapshotLatestDeviationMs = {};
    bool snapshotInputLatencyAvailable = {};
    double snapshotInputLatencyMs = {};
    double snapshotLateInputClampMs = {};
    qulonglong snapshotLateFrames = {};
    qulonglong snapshotDroppedInputs = {};
    int snapshotActiveVoices = {};

    bool audioUnderrunTelemetryAvailable = {};
    qulonglong audioUnderruns = {};
    qulonglong observedHeapBytes = {};
    qulonglong sealedHeapBytes = {};
    std::uint64_t nextInputSequence = 1;
    std::uint64_t nextSessionGeneration = 1;
    std::uint64_t pendingSessionGeneration = {};
    bool decodeRequested = {};
    bool audioInitializationRequested = {};
    bool heapSealed = {};
    bool waitingForAnchor = {};
    bool startCommandInFlight = {};
    bool abortCommandInFlight = {};
    bool presetIsLocked = {};
};

} // namespace web_playtest
