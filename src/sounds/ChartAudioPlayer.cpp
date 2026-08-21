#include "ChartAudioPlayer.h"

#include "AudioEngine.h"
#include "Sound.h"
#include "resource_managers/ChartDataFactory.h"
#include "resource_managers/SongAssetStore.h"
#include "resource_managers/loadBmsSounds.h"
#include "support/PathToQString.h"
#include "support/QStringToPath.h"

#include <QFutureWatcher>
#include <QtConcurrentRun>

#include <algorithm>
#include <filesystem>
#include <limits>
#include <random>
#include <ranges>
#include <stdexcept>
#include <string_view>
#include <utility>

#include <spdlog/spdlog.h>

using namespace std::chrono_literals;

namespace sounds {
namespace {

struct LoadedChartAudio
{
    std::vector<ChartAudioEvent> events;
    std::unordered_map<std::uint64_t, std::shared_ptr<Sound>> sounds;
    std::chrono::nanoseconds loopLength;
};

void
throwIfCancelled(const std::shared_ptr<std::atomic_bool>& cancellation)
{
    if (cancellation->load()) {
        throw std::runtime_error("Chart audio loading was cancelled");
    }
}

auto
scheduledSoundIds(const std::span<const ChartAudioEvent> events)
  -> std::unordered_set<std::uint64_t>
{
    auto ids = std::unordered_set<std::uint64_t>{};
    ids.reserve(events.size());
    for (const auto& event : events) {
        ids.insert(event.soundId);
    }
    return ids;
}

void
retainScheduledSoundResources(
  resource_managers::ChartDataFactory::ChartComponents& components,
  const std::unordered_set<std::uint64_t>& scheduledIds)
{
    auto& notes = components.notesData;
    if (notes.bmsonSlices.empty()) {
        std::erase_if(components.wavs, [&scheduledIds](const auto& entry) {
            return !scheduledIds.contains(entry.first);
        });
        return;
    }

    auto requiredSliceIds = scheduledIds;
    for (const auto soundId : scheduledIds) {
        if (const auto fusion = notes.bmsonFusions.find(soundId);
            fusion != notes.bmsonFusions.end()) {
            requiredSliceIds.insert(fusion->second.begin(),
                                    fusion->second.end());
        }
    }
    std::erase_if(notes.bmsonSlices, [&requiredSliceIds](const auto& slice) {
        return !requiredSliceIds.contains(slice.soundId);
    });
    std::erase_if(notes.bmsonFusions, [&scheduledIds](const auto& entry) {
        return !scheduledIds.contains(entry.first);
    });

    auto requiredChannels = std::unordered_set<std::uint64_t>{};
    requiredChannels.reserve(notes.bmsonSlices.size());
    for (const auto& slice : notes.bmsonSlices) {
        requiredChannels.insert(slice.channelIndex);
    }
    std::erase_if(components.wavs, [&requiredChannels](const auto& entry) {
        return !requiredChannels.contains(entry.first);
    });
}

auto
loadChartComponents(resource_managers::SongAssetStore* assetStore,
                    const std::filesystem::path& path,
                    const std::shared_ptr<std::atomic_bool>& cancellation)
  -> resource_managers::ChartDataFactory::ChartComponents
{
    auto randomGenerator = [](const charts::ParsedBmsChart::RandomRange range) {
        thread_local auto randomEngine =
          std::default_random_engine{ std::random_device{}() };
        if (range <= 1) {
            return charts::ParsedBmsChart::RandomRange{ 1 };
        }
        return std::uniform_int_distribution{
            charts::ParsedBmsChart::RandomRange{ 1 }, range
        }(randomEngine);
    };
    auto factory = resource_managers::ChartDataFactory{};
    const auto extension = support::pathToQString(path.extension()).toLower();
    if (!assetStore->isArchived(path)) {
        return extension == QStringLiteral(".bmson")
                 ? factory.loadBmsonChartData(path)
                 : factory.loadChartData(path, std::move(randomGenerator));
    }

    const auto contents = assetStore->read(path, cancellation.get());
    throwIfCancelled(cancellation);
    const auto view =
      std::string_view{ contents.constData(),
                        static_cast<std::size_t>(contents.size()) };
    return extension == QStringLiteral(".bmson")
             ? factory.loadBmsonChartData(view, path)
             : factory.loadChartData(view, path, std::move(randomGenerator));
}

auto
loadArchivedSounds(
  AudioEngine* engine,
  resource_managers::SongAssetStore* assetStore,
  const std::filesystem::path& chartDirectory,
  const std::unordered_map<std::uint64_t, std::filesystem::path>& paths,
  const charts::BmsNotesData& notes,
  const std::shared_ptr<std::atomic_bool>& cancellation)
  -> std::unordered_map<std::uint64_t, std::shared_ptr<Sound>>
{
    auto encodedSounds = charts::loadArchivedSoundData(
      assetStore, chartDirectory, paths, cancellation.get());
    throwIfCancelled(cancellation);
    if (!notes.bmsonSlices.empty()) {
        return charts::loadBmsonSounds(engine,
                                       encodedSounds,
                                       notes.bmsonSlices,
                                       notes.bmsonFusions,
                                       cancellation.get());
    }
    return charts::loadBmsSounds(engine, encodedSounds, cancellation.get());
}

auto
loadChartAudio(const QString& source,
               AudioEngine* engine,
               resource_managers::SongAssetStore* assetStore,
               const std::shared_ptr<std::atomic_bool>& cancellation)
  -> LoadedChartAudio
{
    throwIfCancelled(cancellation);
    const auto path = support::qStringToPath(source);
    auto components = loadChartComponents(assetStore, path, cancellation);
    throwIfCancelled(cancellation);

    auto events = createChartAudioEvents(components.notesData);
    auto scheduledIds = scheduledSoundIds(events);
    const auto loopLength = chartAudioLoopLength(
      events, std::chrono::nanoseconds{ components.chartData->getLength() });
    retainScheduledSoundResources(components, scheduledIds);
    components.bmps.clear();
    throwIfCancelled(cancellation);
    const auto chartDirectory = path.parent_path();
    auto loadedSounds =
      assetStore->isArchived(path)
        ? loadArchivedSounds(engine,
                             assetStore,
                             chartDirectory,
                             components.wavs,
                             components.notesData,
                             cancellation)
        : (!components.notesData.bmsonSlices.empty()
             ? charts::loadBmsonSounds(engine,
                                       components.wavs,
                                       components.notesData.bmsonSlices,
                                       components.notesData.bmsonFusions,
                                       chartDirectory,
                                       cancellation.get())
             : charts::loadBmsSounds(
                 engine, components.wavs, chartDirectory, cancellation.get()));
    throwIfCancelled(cancellation);
    std::erase_if(events, [&loadedSounds](const auto& event) {
        return !loadedSounds.contains(event.soundId);
    });
    std::erase_if(loadedSounds, [&scheduledIds](const auto& entry) {
        return !scheduledIds.contains(entry.first);
    });
    return { std::move(events), std::move(loadedSounds), loopLength };
}

} // namespace

auto
createChartAudioEvents(const charts::BmsNotesData& notes)
  -> std::vector<ChartAudioEvent>
{
    auto events = std::vector<ChartAudioEvent>{};
    auto noteCount = std::size_t{};
    for (const auto& column : notes.notes) {
        noteCount += column.size();
    }
    events.reserve(notes.bgmNotes.size() + noteCount);
    for (const auto& [time, soundId] : notes.bgmNotes) {
        events.push_back({ time.timestamp, soundId });
    }
    for (const auto& column : notes.notes) {
        for (const auto& note : column) {
            if (note.noteType == charts::BmsNotesData::NoteType::Normal ||
                note.noteType ==
                  charts::BmsNotesData::NoteType::LongNoteBegin) {
                events.push_back({ note.time.timestamp, note.sound });
            }
        }
    }
    std::ranges::stable_sort(events, {}, &ChartAudioEvent::timestamp);
    return events;
}

auto
chartAudioLoopLength(const std::span<const ChartAudioEvent> events,
                     const std::chrono::nanoseconds chartLength)
  -> std::chrono::nanoseconds
{
    auto lastEvent = 0ns;
    for (const auto& event : events) {
        lastEvent = std::max(lastEvent, event.timestamp);
    }
    return std::max({ 0ns, chartLength, lastEvent }) + 5s;
}

ChartAudioPlayer::ChartAudioPlayer(QObject* parent)
  : QObject(parent)
{
    playbackTimer.setSingleShot(true);
    playbackTimer.setTimerType(Qt::PreciseTimer);
    connect(&playbackTimer,
            &QTimer::timeout,
            this,
            &ChartAudioPlayer::updatePlayback);
}

ChartAudioPlayer::~ChartAudioPlayer()
{
    cancelLoading();
    stopPlayback();
    retireLoadedSounds();
}

auto
ChartAudioPlayer::getSource() const -> QString
{
    return source;
}

void
ChartAudioPlayer::setSource(const QString& value)
{
    if (source == value) {
        return;
    }
    cancelLoading();
    stopPlayback();
    clearLoadedChart();
    source = value;
    ++sourceGeneration;
    emit sourceChanged();
    if (!source.isEmpty()) {
        beginLoading();
    }
}

void
ChartAudioPlayer::resetSource()
{
    setSource({});
}

void
ChartAudioPlayer::beginLoading()
{
    if (!engine || !assetStore || !threadPool || source.isEmpty()) {
        spdlog::error(
          "Cannot load chart audio before its dependencies and source are set");
        return;
    }
    setLoading(true);
    const auto generation = sourceGeneration;
    const auto requestedSource = source;
    auto* const audioEngine = engine;
    auto* const songAssetStore = assetStore;
    auto* const loaderThreadPool = threadPool;
    auto cancellation = std::make_shared<std::atomic_bool>(false);
    sourceCancellation = cancellation;
    auto* watcher = new QFutureWatcher<LoadedChartAudio>{ this };
    connect(
      watcher,
      &QFutureWatcher<LoadedChartAudio>::finished,
      this,
      [this, watcher, generation, requestedSource, cancellation]() mutable {
          try {
              auto future = watcher->future();
              auto result = future.takeResult();
              if (generation == sourceGeneration && requestedSource == source &&
                  !cancellation->load()) {
                  events = std::move(result.events);
                  sounds = std::move(result.sounds);
                  loopLength = result.loopLength;
                  setLoaded(!events.empty());
                  setLoading(false);
                  if (playing && loaded) {
                      startPlayback();
                  }
              } else {
                  retireSounds(std::move(result.sounds));
              }
          } catch (const std::exception& error) {
              if (generation == sourceGeneration && requestedSource == source &&
                  !cancellation->load()) {
                  spdlog::error("Failed to load chart audio {}: {}",
                                requestedSource.toStdString(),
                                error.what());
                  setLoaded(false);
                  setLoading(false);
              }
          }
          if (sourceCancellation == cancellation) {
              sourceCancellation.reset();
          }
          watcher->deleteLater();
      });
    watcher->setFuture(QtConcurrent::run(
      loaderThreadPool,
      [requestedSource, audioEngine, songAssetStore, cancellation] {
          return loadChartAudio(
            requestedSource, audioEngine, songAssetStore, cancellation);
      }));
}

void
ChartAudioPlayer::cancelLoading()
{
    if (sourceCancellation) {
        sourceCancellation->store(true);
        sourceCancellation.reset();
    }
    setLoading(false);
}

void
ChartAudioPlayer::clearLoadedChart()
{
    events.clear();
    retireLoadedSounds();
    loopLength = {};
    nextEvent = 0;
    setLoaded(false);
}

void
ChartAudioPlayer::retireLoadedSounds()
{
    retireSounds(std::move(sounds));
}

void
ChartAudioPlayer::retireSounds(SoundMap retiredSounds)
{
    if (retiredSounds.empty()) {
        return;
    }
    if (!threadPool) {
        return;
    }
    auto soundsToDestroy = std::make_shared<SoundMap>(std::move(retiredSounds));
    static_cast<void>(QtConcurrent::run(
      threadPool, [soundsToDestroy] { soundsToDestroy->clear(); }));
}

auto
ChartAudioPlayer::getVolume() const -> float
{
    return volume;
}

void
ChartAudioPlayer::setVolume(const float value)
{
    if (volume == value) {
        return;
    }
    volume = value;
    applyActiveVolume();
    emit volumeChanged();
}

auto
ChartAudioPlayer::isLooping() const -> bool
{
    return looping;
}

void
ChartAudioPlayer::setLooping(const bool value)
{
    if (looping == value) {
        return;
    }
    looping = value;
    emit loopingChanged();
}

auto
ChartAudioPlayer::getFadeInMillis() const -> quint64
{
    return fadeInMillis;
}

void
ChartAudioPlayer::setFadeInMillis(const quint64 value)
{
    if (fadeInMillis == value) {
        return;
    }
    fadeInMillis = value;
    emit fadeInMillisChanged();
}

auto
ChartAudioPlayer::isPlaying() const -> bool
{
    return playing;
}

void
ChartAudioPlayer::setPlaying(const bool value)
{
    if (value) {
        play();
    } else {
        stop();
    }
}

auto
ChartAudioPlayer::isLoaded() const -> bool
{
    return loaded;
}

auto
ChartAudioPlayer::isLoading() const -> bool
{
    return loading;
}

void
ChartAudioPlayer::play()
{
    if (playing) {
        return;
    }
    playing = true;
    emit playingChanged();
    if (loaded) {
        startPlayback();
    }
}

void
ChartAudioPlayer::stop()
{
    stopPlayback();
    if (!playing) {
        return;
    }
    playing = false;
    emit playingChanged();
}

void
ChartAudioPlayer::startPlayback()
{
    stopPlayback();
    nextEvent = 0;
    playbackStartedAt = std::chrono::steady_clock::now();
    lastFadeUpdate = {};
    playbackGain = fadeInMillis == 0 ? 1.0F : 0.0F;
    updatePlayback();
}

void
ChartAudioPlayer::stopPlayback()
{
    playbackTimer.stop();
    stopActiveSounds();
    nextEvent = 0;
    playbackStartedAt = {};
    lastFadeUpdate = {};
    playbackGain = 1.0F;
}

void
ChartAudioPlayer::updatePlayback()
{
    if (!playing || !loaded ||
        playbackStartedAt == decltype(playbackStartedAt){}) {
        playbackTimer.stop();
        return;
    }
    const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(
      std::chrono::steady_clock::now() - playbackStartedAt);

    if (fadeInMillis > 0) {
        const auto fadeLength =
          std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::milliseconds{
              static_cast<std::chrono::milliseconds::rep>(fadeInMillis) });
        playbackGain = fadeLength > 0ns
                         ? std::clamp(static_cast<float>(elapsed.count()) /
                                        static_cast<float>(fadeLength.count()),
                                      0.0F,
                                      1.0F)
                         : 1.0F;
        if (elapsed - lastFadeUpdate >= 16ms || elapsed >= fadeLength) {
            applyActiveVolume();
            lastFadeUpdate = std::min(elapsed, fadeLength);
        }
    } else {
        if (playbackGain != 1.0F) {
            playbackGain = 1.0F;
            applyActiveVolume();
        }
    }

    while (nextEvent < events.size() &&
           events[nextEvent].timestamp <= elapsed) {
        if (const auto found = sounds.find(events[nextEvent].soundId);
            found != sounds.end()) {
            found->second->setVolume(volume * playbackGain);
            found->second->play();
            activeSoundIds.insert(events[nextEvent].soundId);
        }
        ++nextEvent;
    }
    if (elapsed < loopLength) {
        scheduleNextUpdate(elapsed);
        return;
    }
    if (looping) {
        restartLoop();
        return;
    }
    stopPlayback();
    playing = false;
    emit playingChanged();
}

void
ChartAudioPlayer::restartLoop()
{
    stopActiveSounds();
    nextEvent = 0;
    playbackStartedAt = std::chrono::steady_clock::now();
    lastFadeUpdate = {};
    playbackGain = fadeInMillis == 0 ? 1.0F : 0.0F;
    updatePlayback();
}

void
ChartAudioPlayer::scheduleNextUpdate(const std::chrono::nanoseconds elapsed)
{
    auto wakeAt = loopLength;
    if (nextEvent < events.size()) {
        wakeAt = std::min(wakeAt, events[nextEvent].timestamp);
    }
    if (fadeInMillis > 0) {
        const auto fadeLength =
          std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::milliseconds{
              static_cast<std::chrono::milliseconds::rep>(fadeInMillis) });
        if (elapsed < fadeLength) {
            wakeAt =
              std::min(wakeAt, std::min(fadeLength, lastFadeUpdate + 16ms));
        }
    }

    const auto delay = std::chrono::ceil<std::chrono::milliseconds>(
      std::max(0ns, wakeAt - elapsed));
    const auto maxDelay = static_cast<std::chrono::milliseconds::rep>(
      std::numeric_limits<int>::max());
    playbackTimer.start(static_cast<int>(std::clamp(
      delay.count(), std::chrono::milliseconds::rep{ 1 }, maxDelay)));
}

void
ChartAudioPlayer::stopActiveSounds()
{
    for (const auto soundId : activeSoundIds) {
        if (const auto found = sounds.find(soundId); found != sounds.end()) {
            found->second->stop();
        }
    }
    activeSoundIds.clear();
}

void
ChartAudioPlayer::applyActiveVolume()
{
    for (auto soundId = activeSoundIds.begin();
         soundId != activeSoundIds.end();) {
        const auto found = sounds.find(*soundId);
        if (found == sounds.end() || !found->second->isPlaying()) {
            soundId = activeSoundIds.erase(soundId);
            continue;
        }
        found->second->setVolume(volume * playbackGain);
        ++soundId;
    }
}

void
ChartAudioPlayer::setLoaded(const bool value)
{
    if (loaded == value) {
        return;
    }
    loaded = value;
    emit loadedChanged();
}

void
ChartAudioPlayer::setLoading(const bool value)
{
    if (loading == value) {
        return;
    }
    loading = value;
    emit loadingChanged();
}

} // namespace sounds
