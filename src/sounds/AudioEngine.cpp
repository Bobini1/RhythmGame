//
// Created by PC on 04/09/2025.
//

#include "AudioEngine.h"
#include "Sound.h"
#include <spdlog/spdlog.h>
#include <stdexcept>


//


namespace sounds {
namespace {
void dataCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
    ma_data_source_read_pcm_frames(
      pDevice->pUserData, pOutput, frameCount, nullptr);

    (void)pInput;
}

} // namespace

AudioEngine::AudioEngine()
{
    ma_result result;
    ma_device_config deviceConfig;
    ma_resource_manager_config resourceManagerConfig;
    ma_device_info* pPlaybackDeviceInfos;
    ma_uint32 playbackDeviceCount;
    ma_device_info* pCaptureDeviceInfos;
    ma_uint32 captureDeviceCount;

    ma_context context;
    if (ma_context_init(NULL, 0, NULL, &context) != MA_SUCCESS) {
        throw std::runtime_error("Failed to initialize audio context.");
    }

    result = ma_context_get_devices(&context, &pPlaybackDeviceInfos, &playbackDeviceCount, &pCaptureDeviceInfos, &captureDeviceCount);
    if (result != MA_SUCCESS) {
        throw std::runtime_error("Failed to retrieve device information.");
    }

    printf("Playback Devices\n");
    for (ma_uint32 iDevice = 0; iDevice < playbackDeviceCount; ++iDevice) {
        spdlog::info("    {}: {}\n", iDevice, pPlaybackDeviceInfos[iDevice].name);
    }

    /* We'll initialize the device first. */
    deviceConfig = ma_device_config_init(ma_device_type_playback);
    //deviceConfig.dataCallback = dataCallback;
    deviceConfig.playback.format   = ma_format_f32; /* <-- Request 32-bit floating point output. */
    deviceConfig.playback.channels = 2;            /* <-- Request stereo output. */
    deviceConfig.sampleRate        = sampleRate;   /* <-- Request 44.1kHz output. */
    deviceConfig.periodSizeInFrames = 512;        /* <-- Request a period size of 512 frames. */
    deviceConfig.noFixedSizedCallback = MA_TRUE; /* <-- This tells miniaudio to not require the callback to be called with a fixed number of frames. */
    deviceConfig.performanceProfile = ma_performance_profile_low_latency; /* <-- Low latency. */

    result = ma_device_init(NULL, &deviceConfig, &device);
    if (result != MA_SUCCESS) {
        throw std::runtime_error("Failed to initialize audio device.");
    }

    resourceManagerConfig = ma_resource_manager_config_init();
    resourceManagerConfig.decodedFormat     = device.playback.format;
    resourceManagerConfig.decodedChannels   = device.playback.channels;
    resourceManagerConfig.decodedSampleRate = device.sampleRate;
    resourceManagerConfig.jobThreadCount = std::thread::hardware_concurrency();

    result = ma_resource_manager_init(&resourceManagerConfig, &resourceManager);
    if (result != MA_SUCCESS) {
        ma_device_uninit(&device);
        throw std::runtime_error("Failed to initialize resource manager.");
    }

    // create engine
    auto engineConfig = ma_engine_config_init();
    engineConfig.pResourceManager = &resourceManager;
    result = ma_engine_init(&engineConfig, &engine);
    if (result != MA_SUCCESS) {
        ma_device_uninit(&device);
        ma_resource_manager_uninit(&resourceManager);
        throw std::runtime_error("Failed to initialize audio engine.");
    }
    (void)ma_device_start(&device);

}

AudioEngine::~AudioEngine()
{
    ma_device_uninit(&device);
    ma_resource_manager_uninit(&resourceManager);
}

}