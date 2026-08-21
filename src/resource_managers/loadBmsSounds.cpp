//
// Created by bobini on 16.06.23.
//

#include "loadBmsSounds.h"

#include "sounds/MultiSound.h"
#include "sounds/NormalSound.h"
#include "sounds/NormalSoundBuffer.h"
#include "sounds/SlicedSoundBuffer.h"
#include "sounds/SoundBuffer.h"
#include "resource_managers/SongAssetStore.h"
#include "support/PathToQString.h"
#include "support/PathToUtfString.h"

#include <QDir>
#include <QHash>

#include <optional>
#include <unordered_set>
#include <QtConcurrent>

namespace charts {

namespace {

auto
isCancelled(const std::atomic_bool* cancellation) -> bool
{
    return cancellation && cancellation->load();
}

} // namespace

auto
loadArchivedSoundData(
  resource_managers::SongAssetStore* assetStore,
  const std::filesystem::path& chartDirectory,
  const std::unordered_map<uint64_t, std::filesystem::path>& paths,
  const std::atomic_bool* cancellation) -> EncodedSounds
{
    auto encodedSounds = EncodedSounds{};
    auto uniqueSounds = QHash<QString, EncodedSound>{};
    for (const auto& [id, relativePath] : paths) {
        if (isCancelled(cancellation)) {
            return {};
        }
        auto key = support::pathToQString(relativePath);
        key.replace('\\', '/');
        key = QDir::cleanPath(key).toCaseFolded();
        if (const auto found = uniqueSounds.constFind(key);
            found != uniqueSounds.cend()) {
            encodedSounds.emplace(id, *found);
            continue;
        }
        try {
            auto encoded = std::make_shared<const QByteArray>(
              assetStore->read(chartDirectory / relativePath, cancellation));
            if (isCancelled(cancellation)) {
                return {};
            }
            uniqueSounds.insert(key, encoded);
            encodedSounds.emplace(id, std::move(encoded));
        } catch (const std::exception& error) {
            if (isCancelled(cancellation)) {
                return {};
            }
            spdlog::warn(
              "Failed to read archived sound {}: {}",
              support::pathToUtfString(chartDirectory / relativePath),
              error.what());
        }
    }
    return encodedSounds;
}

#if _WIN32
auto
getActualPathWindows(std::filesystem::path filePath)
  -> std::optional<std::filesystem::path>
{
    if (exists(filePath)) {
        return filePath;
    }
    filePath.replace_extension(".wav");
    if (exists(filePath)) {
        return filePath;
    }
    filePath.replace_extension(".flac");
    if (exists(filePath)) {
        return filePath;
    }
    filePath.replace_extension(".ogg");
    if (exists(filePath)) {
        return filePath;
    }
    filePath.replace_extension(".mp3");
    if (exists(filePath)) {
        return filePath;
    }
    return std::nullopt;
}
#endif

auto
getActualPath(
  std::unordered_map<std::string, std::filesystem::path>& lowerCaseFilesMap,
  std::string filePath) -> std::optional<std::filesystem::path>
{
    if (auto it = lowerCaseFilesMap.find(filePath);
        it != lowerCaseFilesMap.end()) {
        return std::filesystem::path{ it->second };
    }
    filePath.replace(filePath.end() - 3, filePath.end(), "wav");
    if (auto it = lowerCaseFilesMap.find(filePath);
        it != lowerCaseFilesMap.end()) {
        return std::filesystem::path{ it->second };
    }
    filePath.replace(filePath.end() - 3, filePath.end(), "flac");
    if (auto it = lowerCaseFilesMap.find(filePath);
        it != lowerCaseFilesMap.end()) {
        return std::filesystem::path{ it->second };
    }
    filePath.replace(filePath.end() - 4, filePath.end(), "ogg");
    if (auto it = lowerCaseFilesMap.find(filePath);
        it != lowerCaseFilesMap.end()) {
        return std::filesystem::path{ it->second };
    }
    filePath.replace(filePath.end() - 3, filePath.end(), "mp3");
    if (auto it = lowerCaseFilesMap.find(filePath);
        it != lowerCaseFilesMap.end()) {
        return std::filesystem::path{ it->second };
    }
    return std::nullopt;
}

auto
createLowerCaseFilesMap(std::filesystem::path dirToSearch)
  -> std::unordered_map<std::string, std::filesystem::path>
{
    auto lowerCaseFilesMap =
      std::unordered_map<std::string, std::filesystem::path>{};
    for (const auto& entry :
         std::filesystem::recursive_directory_iterator(dirToSearch)) {
        if (entry.is_regular_file()) {
            auto path = entry.path();
            auto pathString = support::pathToUtfString(path.filename());
            std::ranges::transform(
              pathString, pathString.begin(), [](unsigned char c) {
                  return std::tolower(c);
              });
            lowerCaseFilesMap.emplace(pathString, path);
        }
    }
    return lowerCaseFilesMap;
}

auto
loadEncodedBuffers(sounds::AudioEngine* engine,
                   const EncodedSounds& encodedSounds,
                   const std::atomic_bool* cancellation)
  -> std::unordered_map<EncodedSound,
                        std::shared_ptr<const sounds::SoundBuffer>>
{
    auto uniqueSounds = std::unordered_set<EncodedSound>{};
    uniqueSounds.reserve(encodedSounds.size());
    for (const auto& [id, encoded] : encodedSounds) {
        Q_UNUSED(id);
        if (isCancelled(cancellation)) {
            return {};
        }
        if (encoded) {
            uniqueSounds.insert(encoded);
        }
    }
    return QtConcurrent::blockingMappedReduced<
      std::unordered_map<EncodedSound,
                         std::shared_ptr<const sounds::SoundBuffer>>>(
      uniqueSounds,
      [engine, cancellation](const EncodedSound& encoded)
        -> std::optional<
          std::pair<EncodedSound, std::shared_ptr<const sounds::SoundBuffer>>> {
          if (isCancelled(cancellation)) {
              return std::nullopt;
          }
          try {
              auto decoded =
                std::pair{ encoded,
                           std::make_shared<const sounds::NormalSoundBuffer>(
                             engine, QByteArrayView{ *encoded }) };
              if (isCancelled(cancellation)) {
                  return std::nullopt;
              }
              return decoded;
          } catch (const std::exception& error) {
              spdlog::warn("Failed to load archived sound: {}", error.what());
              return std::nullopt;
          }
      },
      [cancellation](auto& buffers, const auto& decoded) {
          if (decoded && !isCancelled(cancellation)) {
              buffers.emplace(decoded->first, decoded->second);
          }
      });
}

auto
createBmsonSounds(
  sounds::AudioEngine* engine,
  const std::unordered_map<uint64_t,
                           std::shared_ptr<const sounds::SoundBuffer>>&
    channelBuffers,
  const std::vector<BmsNotesData::BmsonSliceInfo>& slices,
  const std::unordered_map<uint64_t, std::vector<uint64_t>>& fusions,
  const std::atomic_bool* cancellation)
  -> std::unordered_map<uint64_t, std::shared_ptr<sounds::Sound>>
{
    if (isCancelled(cancellation)) {
        return {};
    }
    const auto sampleRate = static_cast<double>(engine->getSampleRate());
    auto result =
      std::unordered_map<uint64_t, std::shared_ptr<sounds::Sound>>{};

    for (const auto& slice : slices) {
        if (isCancelled(cancellation)) {
            return {};
        }
        const auto bufIt = channelBuffers.find(slice.channelIndex);
        if (bufIt == channelBuffers.end()) {
            continue;
        }
        const auto& fullBuffer = bufIt->second;
        const auto totalFrames = fullBuffer->getFrames();

        auto startFrame =
          static_cast<ma_uint64>(slice.startSeconds * sampleRate);
        auto endFrame =
          slice.endSeconds < 0.0
            ? totalFrames
            : static_cast<ma_uint64>(slice.endSeconds * sampleRate);
        startFrame = std::min(startFrame, totalFrames);
        endFrame = std::clamp(endFrame, startFrame, totalFrames);

        auto slicedBuffer = std::make_shared<sounds::SlicedSoundBuffer>(
          fullBuffer, startFrame, endFrame - startFrame);
        result.emplace(slice.soundId,
                       std::make_shared<sounds::NormalSound>(
                         engine, std::move(slicedBuffer)));
    }

    for (const auto& [fusedId, sliceIds] : fusions) {
        if (isCancelled(cancellation)) {
            return {};
        }
        auto children = std::vector<std::shared_ptr<sounds::Sound>>{};
        for (const auto sliceId : sliceIds) {
            if (const auto found = result.find(sliceId);
                found != result.end()) {
                children.push_back(found->second);
            }
        }
        if (!children.empty()) {
            result.emplace(
              fusedId,
              std::make_shared<sounds::MultiSound>(std::move(children)));
        }
    }
    return result;
}

auto
loadBmsSounds(sounds::AudioEngine* engine,
              const std::unordered_map<uint64_t, std::filesystem::path>& wavs,
              const std::filesystem::path& path,
              const std::atomic_bool* cancellation)
  -> std::unordered_map<uint64_t, std::shared_ptr<sounds::Sound>>
{
    auto start = std::chrono::high_resolution_clock::now();
    if (isCancelled(cancellation)) {
        return {};
    }
    auto wavsActualPaths =
      std::unordered_map<uint64_t, std::filesystem::path>{};
    wavsActualPaths.reserve(wavs.size());
    auto uniqueSoundPaths = std::unordered_set<std::filesystem::path>{};
#ifndef _WIN32
    auto lowerCaseFilesMap = createLowerCaseFilesMap(path);
#endif
    for (const auto& [key, value] : wavs) {
        if (isCancelled(cancellation)) {
            return {};
        }
        {
#ifdef _WIN32
            auto filePath = path / value;
            auto actualPath = getActualPathWindows(filePath);
#else
            auto valueLower = support::pathToUtfString(value);
            std::ranges::transform(
              valueLower, valueLower.begin(), [](unsigned char c) {
                  return std::tolower(c);
              });

            auto filePath = path / value;
            auto actualPath = getActualPath(lowerCaseFilesMap, valueLower);
#endif
            if (!actualPath) {
                spdlog::warn("File {} not found.",
                             support::pathToUtfString(filePath));
                continue;
            }
            wavsActualPaths.emplace(key, *actualPath);
            uniqueSoundPaths.emplace(std::move(*actualPath));
        }
    }
    std::unordered_map<std::filesystem::path,
                       std::shared_ptr<const sounds::SoundBuffer>>
      buffers;
    buffers.reserve(uniqueSoundPaths.size());

    buffers = QtConcurrent::blockingMappedReduced<
      std::unordered_map<std::filesystem::path,
                         std::shared_ptr<const sounds::SoundBuffer>>>(
      uniqueSoundPaths,
      [engine, cancellation](const auto& path)
        -> std::optional<
          std::pair<std::filesystem::path,
                    std::shared_ptr<const sounds::SoundBuffer>>> {
          if (isCancelled(cancellation)) {
              return std::nullopt;
          }
          try {
              auto decoded =
                std::pair{ path,
                           std::make_shared<const sounds::NormalSoundBuffer>(
                             engine, path) };
              if (isCancelled(cancellation)) {
                  return std::nullopt;
              }
              return decoded;
          } catch (const std::exception& e) {
              spdlog::warn("Failed to load sound {}: {}",
                           support::pathToUtfString(path),
                           e.what());
              return std::nullopt;
          }
      },
      [cancellation](auto& container, const auto& pair) -> void {
          if (pair && !isCancelled(cancellation)) {
              container.emplace(pair->first, pair->second);
          }
      },
      std::move(buffers));

    auto sounds =
      std::unordered_map<uint64_t, std::shared_ptr<sounds::Sound>>();
    sounds.reserve(wavsActualPaths.size());
    for (const auto& [key, actualPath] : wavsActualPaths) {
        if (isCancelled(cancellation)) {
            return {};
        }
        auto buffer = buffers.find(actualPath);
        if (buffer != buffers.end()) {
            sounds.emplace(
              key,
              std::make_shared<sounds::NormalSound>(engine, buffer->second));
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    spdlog::info(
      "Loading {} sounds took: {} ms",
      buffers.size(),
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
        .count());
    return sounds;
}

auto
loadBmsSounds(sounds::AudioEngine* engine,
              const EncodedSounds& wavs,
              const std::atomic_bool* cancellation)
  -> std::unordered_map<uint64_t, std::shared_ptr<sounds::Sound>>
{
    const auto start = std::chrono::high_resolution_clock::now();
    const auto buffers = loadEncodedBuffers(engine, wavs, cancellation);
    auto result =
      std::unordered_map<uint64_t, std::shared_ptr<sounds::Sound>>{};
    result.reserve(wavs.size());
    for (const auto& [id, encoded] : wavs) {
        if (isCancelled(cancellation)) {
            return {};
        }
        if (const auto found = buffers.find(encoded); found != buffers.end()) {
            result.emplace(
              id, std::make_shared<sounds::NormalSound>(engine, found->second));
        }
    }
    const auto end = std::chrono::high_resolution_clock::now();
    spdlog::info(
      "Loading {} archived sounds took: {} ms",
      buffers.size(),
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
        .count());
    return result;
}

auto
loadBmsonSounds(
  sounds::AudioEngine* engine,
  const std::unordered_map<uint64_t, std::filesystem::path>& channelPaths,
  const std::vector<BmsNotesData::BmsonSliceInfo>& slices,
  const std::unordered_map<uint64_t, std::vector<uint64_t>>& fusions,
  const std::filesystem::path& basePath,
  const std::atomic_bool* cancellation)
  -> std::unordered_map<uint64_t, std::shared_ptr<sounds::Sound>>
{
    auto start = std::chrono::high_resolution_clock::now();
    if (isCancelled(cancellation)) {
        return {};
    }

    // 1. Resolve actual file paths for each channel
    auto channelActualPaths =
      std::unordered_map<uint64_t, std::filesystem::path>{};
    auto uniquePaths = std::unordered_set<std::filesystem::path>{};
#ifndef _WIN32
    auto lowerCaseFilesMap = createLowerCaseFilesMap(basePath);
#endif
    for (const auto& [idx, relPath] : channelPaths) {
        if (isCancelled(cancellation)) {
            return {};
        }
#ifdef _WIN32
        auto actualPath = getActualPathWindows(basePath / relPath);
#else
        auto valueLower = support::pathToUtfString(relPath);
        std::ranges::transform(valueLower,
                               valueLower.begin(),
                               [](unsigned char c) { return std::tolower(c); });
        auto actualPath = getActualPath(lowerCaseFilesMap, valueLower);
#endif
        if (!actualPath) {
            spdlog::warn("Bmson sound not found: {}",
                         support::pathToUtfString(basePath / relPath));
            continue;
        }
        channelActualPaths.emplace(idx, *actualPath);
        uniquePaths.emplace(std::move(*actualPath));
    }

    // 2. Load full buffers in parallel
    auto fullBuffers = QtConcurrent::blockingMappedReduced<
      std::unordered_map<std::filesystem::path,
                         std::shared_ptr<const sounds::SoundBuffer>>>(
      uniquePaths,
      [engine, cancellation](const auto& path)
        -> std::optional<
          std::pair<std::filesystem::path,
                    std::shared_ptr<const sounds::SoundBuffer>>> {
          if (isCancelled(cancellation)) {
              return std::nullopt;
          }
          try {
              auto decoded =
                std::pair{ path,
                           std::make_shared<const sounds::NormalSoundBuffer>(
                             engine, path) };
              if (isCancelled(cancellation)) {
                  return std::nullopt;
              }
              return decoded;
          } catch (const std::exception& e) {
              spdlog::warn("Failed to load bmson sound {}: {}",
                           support::pathToUtfString(path),
                           e.what());
              return std::nullopt;
          }
      },
      [cancellation](auto& container, const auto& pair) {
          if (pair && !isCancelled(cancellation)) {
              container.emplace(pair->first, pair->second);
          }
      });

    // Map channel index -> full buffer
    auto channelBuffers =
      std::unordered_map<uint64_t,
                         std::shared_ptr<const sounds::SoundBuffer>>{};
    for (const auto& [idx, actualPath] : channelActualPaths) {
        if (isCancelled(cancellation)) {
            return {};
        }
        if (auto it = fullBuffers.find(actualPath); it != fullBuffers.end()) {
            channelBuffers[idx] = it->second;
        }
    }

    auto result =
      createBmsonSounds(engine, channelBuffers, slices, fusions, cancellation);

    auto end = std::chrono::high_resolution_clock::now();
    spdlog::info(
      "Loading {} bmson sound slices took: {} ms",
      result.size(),
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
        .count());
    return result;
}

auto
loadBmsonSounds(
  sounds::AudioEngine* engine,
  const EncodedSounds& channels,
  const std::vector<BmsNotesData::BmsonSliceInfo>& slices,
  const std::unordered_map<uint64_t, std::vector<uint64_t>>& fusions,
  const std::atomic_bool* cancellation)
  -> std::unordered_map<uint64_t, std::shared_ptr<sounds::Sound>>
{
    const auto start = std::chrono::high_resolution_clock::now();
    const auto buffers = loadEncodedBuffers(engine, channels, cancellation);
    auto channelBuffers =
      std::unordered_map<uint64_t,
                         std::shared_ptr<const sounds::SoundBuffer>>{};
    channelBuffers.reserve(channels.size());
    for (const auto& [id, encoded] : channels) {
        if (isCancelled(cancellation)) {
            return {};
        }
        if (const auto found = buffers.find(encoded); found != buffers.end()) {
            channelBuffers.emplace(id, found->second);
        }
    }
    auto result =
      createBmsonSounds(engine, channelBuffers, slices, fusions, cancellation);
    const auto end = std::chrono::high_resolution_clock::now();
    spdlog::info(
      "Loading {} archived bmson sound slices took: {} ms",
      result.size(),
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
        .count());
    return result;
}

} // namespace charts
