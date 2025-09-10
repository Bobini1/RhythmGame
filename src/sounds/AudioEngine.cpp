//
// Created by PC on 04/09/2025.
//

#include "AudioEngine.h"
#include "Sound.h"

#include <qsettings.h>
#include <ranges>
#include <spdlog/spdlog.h>
#include <stdexcept>

//

namespace sounds {
namespace {

void dataCallback(ma_device* pDevice, void* pFramesOut, const void* pFramesIn, ma_uint32 frameCount)
{
    const auto pEngine = static_cast<ma_engine*>(pDevice->pUserData);
    ma_engine_read_pcm_frames(pEngine, pFramesOut, frameCount, NULL);
}
void
logCallback(void* /*userData*/, ma_uint32 level, const char* message)
{
    switch (level) {
        case MA_LOG_LEVEL_DEBUG:
            spdlog::debug("miniaudio: {}", message);
            break;
        case MA_LOG_LEVEL_INFO:
            spdlog::info("miniaudio: {}", message);
            break;
        case MA_LOG_LEVEL_WARNING:
            spdlog::warn("miniaudio: {}", message);
            break;
        case MA_LOG_LEVEL_ERROR:
            spdlog::error("miniaudio: {}", message);
            break;
        default:
            spdlog::warn(
              "miniaudio (unknown log level {}): {}", level, message);
            break;
    }
}
auto
getDevices(ma_context& context) -> QList<AudioEngine::DeviceId>
{
    ma_device_info* pPlaybackDeviceInfos{};
    ma_uint32 playbackDeviceCount{};
    ma_device_info* pCaptureDeviceInfos{};
    ma_uint32 captureDeviceCount{};
    auto result = ma_context_get_devices(&context,
                                         &pPlaybackDeviceInfos,
                                         &playbackDeviceCount,
                                         &pCaptureDeviceInfos,
                                         &captureDeviceCount);
    if (result != MA_SUCCESS) {
        throw std::runtime_error("Failed to retrieve device information.");
    }

    auto deviceNames = QList<AudioEngine::DeviceId>{};
    for (ma_uint32 i = 0; i < playbackDeviceCount; i++) {
        deviceNames.emplace_back(pPlaybackDeviceInfos[i].name,
                                 pPlaybackDeviceInfos[i].id);
    }
    return deviceNames;
}

} // namespace
void
AudioEngine::setDeviceImpl(const QString& deviceName)
{
    // get device by name
    auto deviceId = ma_device_id{};
    auto found = false;
    for (const auto& [id, name] :
         std::ranges::views::zip(deviceIds, deviceNames)) {
        if (name == deviceName) {
            deviceId = id;
            found = true;
            break;
        }
    }
    auto deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format = ma_format_f32;
    deviceConfig.playback.channels = 2;
    deviceConfig.playback.pDeviceID = found ? &deviceId : nullptr;
    deviceConfig.sampleRate = sampleRate;
    deviceConfig.periodSizeInFrames = 128;
    deviceConfig.noFixedSizedCallback = MA_TRUE;
    deviceConfig.wasapi.noAutoStreamRouting = static_cast<ma_bool8>(!deviceName.isEmpty());
    deviceConfig.wasapi.usage = ma_wasapi_usage_games;
    deviceConfig.performanceProfile = ma_performance_profile_low_latency;
    deviceConfig.dataCallback = dataCallback;
    deviceConfig.pUserData = engine.get();

    auto result = ma_device_init(context.get(), &deviceConfig, device.get());
    if (result != MA_SUCCESS) {
        throw std::runtime_error("Failed to initialize audio device.");
    }

    auto resourceManagerConfig = ma_resource_manager_config_init();
    resourceManagerConfig.jobThreadCount = std::thread::hardware_concurrency();

    result =
      ma_resource_manager_init(&resourceManagerConfig, resourceManager.get());
    if (result != MA_SUCCESS) {
        ma_device_uninit(device.get());
        throw std::runtime_error("Failed to initialize resource manager.");
    }

    // create engine
    auto engineConfig = ma_engine_config_init();
    engineConfig.pResourceManager = resourceManager.get();
    engineConfig.sampleRate = sampleRate;
    engineConfig.channels = channels;
    engineConfig.pDevice = device.get();
    engineConfig.pContext = context.get();
    result = ma_engine_init(&engineConfig, engine.get());
    if (result != MA_SUCCESS) {
        ma_device_uninit(device.get());
        ma_resource_manager_uninit(resourceManager.get());
        throw std::runtime_error("Failed to initialize audio engine.");
    }
    if (ma_engine_start(engine.get()) != MA_SUCCESS) {
        ma_engine_uninit(engine.get());
        ma_device_uninit(device.get());
        ma_resource_manager_uninit(resourceManager.get());
        throw std::runtime_error("Failed to start audio engine.");
    }
    if (ma_device_start(device.get()) != MA_SUCCESS) {
        ma_engine_uninit(engine.get());
        ma_device_uninit(device.get());
        ma_resource_manager_uninit(resourceManager.get());
        throw std::runtime_error("Failed to start audio device.");
    }

    if (!deviceName.isEmpty()) {
        auto nameBuffer = std::vector<char>(MA_MAX_DEVICE_NAME_LENGTH + 1);
        auto size = size_t{};
        if (ma_device_get_name(device.get(),
                               ma_device_type_playback,
                               nameBuffer.data(),
                               nameBuffer.size(),
                               &size) == MA_SUCCESS) {
            nameBuffer.resize(size);
            currentDevice = QString::fromUtf8(nameBuffer);
            settings.setValue("audio/" + currentBackend + "/device",
                              currentDevice);
        } else {
            spdlog::warn("Failed to get current device name.");
        }
    } else {
        currentDevice = "";
        settings.setValue("audio/" + currentBackend + "/device", "");
    }
}
void
AudioEngine::setBackendImpl(const QString& backendName)
{
    auto config = ma_context_config_init();
    config.jack.pClientName = "RhythmGame";
    config.jack.tryStartServer = MA_TRUE;
    config.pLog = &log;
    config.threadPriority = ma_thread_priority_highest;
    config.pulse.pApplicationName = "RhythmGame";
    config.pulse.tryAutoSpawn = MA_TRUE;

    auto currentBackendEnum = backends[0];
    if (backendName != "") {
        ma_get_backend_from_name(backendName.toStdString().c_str(),
                                 &currentBackendEnum);
        if (currentBackendEnum != backends[0]) {
            backends.erase(
              std::ranges::remove(backends, currentBackendEnum).begin(),
              backends.end());
            backends.insert(backends.begin(), currentBackendEnum);
        }
    }
    if (const auto result = ma_context_init(
          backends.data(), std::size(backends), &config, context.get());
        result != MA_SUCCESS) {
        throw std::runtime_error("Failed to initialize audio context.");
    }
    currentBackend = ma_get_backend_name(context->backend);
    settings.setValue("audio/backend", currentBackend);
    auto devices = getDevices(*context);
    deviceNames.clear();
    deviceIds.clear();
    for (const auto& deviceId : devices) {
        deviceNames.append(deviceId.name);
        deviceIds.append(deviceId.id);
    }

    currentDevice =
      settings.value("audio/" + currentBackend + "/device", "").toString();

    try {
        if (!deviceNames.contains(currentDevice)) {
            setDeviceImpl("");
        } else {
            setDeviceImpl(currentDevice);
        }
    } catch (const std::exception& e) {
        ma_context_uninit(context.get());
        throw;
    }
}
AudioEngine::AudioEngine()
{
    ma_log_init(nullptr, &log);
    ma_log_register_callback(&log, ma_log_callback_init(logCallback, nullptr));
    backends = std::vector<ma_backend>{ MA_BACKEND_COUNT };
    auto count = size_t{};
    const auto result =
      ma_get_enabled_backends(backends.data(), std::size(backends), &count);
    if (result != MA_SUCCESS) {
        throw std::runtime_error("Failed to get enabled backends.");
    }
    backends.resize(count);
    backends.erase(
      std::ranges::remove(backends, ma_backend_custom).begin(),
      backends.end());
    for (const auto backend : backends) {
        backendNames.append(ma_get_backend_name(backend));
    }
    currentBackend = settings.value("audio/backend", "").toString();
    if (!backendNames.contains(currentBackend)) {
        currentBackend = "";
    }
    setBackendImpl(currentBackend);
}

AudioEngine::~AudioEngine()
{
    ma_device_uninit(device.get());
    ma_resource_manager_uninit(resourceManager.get());
    ma_engine_uninit(engine.get());
    ma_context_uninit(context.get());
}
auto
AudioEngine::getBackend() const -> QString
{
    return currentBackend;
}
void
AudioEngine::setBackend(const QString& backend)
{
    if (backendNames.contains(backend) && backend != currentBackend) {
        auto oldBackend = currentBackend;
        auto oldDeviceNames = deviceNames;
        auto oldDeviceIds = deviceIds;
        auto oldDevice = currentDevice;
        auto tempContext = std::move(context);
        auto tempDevice = std::move(device);
        auto tempResourceManager = std::move(resourceManager);
        auto tempEngine = std::move(engine);
        context = std::make_unique<ma_context>();
        device = std::make_unique<ma_device>();
        resourceManager = std::make_unique<ma_resource_manager>();
        engine = std::make_unique<ma_engine>();
        try {
            setBackendImpl(backend);
            if (oldBackend != currentBackend) {
                emit backendChanged();
            }
            if (oldDeviceNames != deviceNames) {
                emit deviceNamesChanged();
            }
            if (oldDevice != currentDevice) {
                emit deviceChanged();
            }
            emit changeDeviceRequested();
        } catch (const std::exception& e) {
            spdlog::error("Failed to set audio backend: {}", e.what());
            context = std::move(tempContext);
            device = std::move(tempDevice);
            resourceManager = std::move(tempResourceManager);
            engine = std::move(tempEngine);
            deviceNames = std::move(oldDeviceNames);
            deviceIds = std::move(oldDeviceIds);
            currentBackend = std::move(oldBackend);
            currentDevice = std::move(oldDevice);
            return;
        }
        ma_device_uninit(tempDevice.get());
        ma_resource_manager_uninit(tempResourceManager.get());
        ma_engine_uninit(tempEngine.get());
        ma_context_uninit(tempContext.get());
    }
}
auto
AudioEngine::getDevice() const -> QString
{
    return currentDevice;
}
void
AudioEngine::setDevice(const QString& device)
{
    if ((deviceNames.contains(device) || device.isEmpty()) && device != currentDevice) {
        auto oldDevice = currentDevice;
        auto tempDevice = std::move(this->device);
        auto tempResourceManager = std::move(resourceManager);
        auto tempEngine = std::move(engine);
        this->device = std::make_unique<ma_device>();
        resourceManager = std::make_unique<ma_resource_manager>();
        engine = std::make_unique<ma_engine>();
        try {
            setDeviceImpl(device);
            if (oldDevice != currentDevice) {
                emit deviceChanged();
            }
            emit changeDeviceRequested();
        } catch (const std::exception& e) {
            spdlog::error("Failed to set audio device: {}", e.what());
            currentDevice = std::move(oldDevice);
            this->device = std::move(tempDevice);
            resourceManager = std::move(tempResourceManager);
            engine = std::move(tempEngine);
            return;
        }
        ma_device_uninit(tempDevice.get());
        ma_resource_manager_uninit(tempResourceManager.get());
        ma_engine_uninit(tempEngine.get());
    }
}
auto
AudioEngine::getBackendNames() const -> QStringList
{
    return backendNames;
}
auto
AudioEngine::getDeviceNames() const -> QStringList
{
    return deviceNames;
}

} // namespace sounds