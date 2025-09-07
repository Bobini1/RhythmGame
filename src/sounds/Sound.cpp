//
// Created by bobini on 14.01.23.
//

#include <stdexcept>
#include <memory>
#include "Sound.h"

#include "sounds/SoundBuffer.h"

#include <gst/gstpipeline.h>
#include <gst/gstutils.h>
namespace sounds {
Sound::Sound(GstElement* player,
             std::shared_ptr<const SoundBuffer> buffer)
  : pipeline(gst_pipeline_new("sound-player"))
  , buffer(std::move(buffer))
{
    volume = gst_element_factory_make("volume", "volume");
    auto* proxysrc =
      gst_element_factory_make("proxysrc", "proxy-source");
    auto proxysink =
      gst_element_factory_make("proxysink", "proxy-sink");


    if (!pipeline || !player || !volume) {
        g_printerr("Can't create.\n");
        spdlog::error("Failed to create pipeline.");
        return;
    }
    // connect
    gst_bin_add_many(GST_BIN(pipeline), proxysrc, volume, proxysink, NULL);
    if (!gst_element_link_many(proxysrc, volume, proxysink, NULL)) {
        g_printerr("Can't connect.\n");
        gst_object_unref(pipeline);
        pipeline = nullptr;
        spdlog::error("Failed to link elements.");
        return;
    }
    g_object_set (player, "proxysink", proxysink, NULL);
    g_object_set (proxysrc, "proxysink", this->buffer->getBuffer(), NULL);
}

Sound::~Sound()
{
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
}

void Sound::play()
{
    this->buffer->enable();
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
}

void Sound::stop()
{
    gst_element_set_state(pipeline, GST_STATE_NULL);
}

auto Sound::isPlaying() const -> bool
{
    GstState state;
    gst_element_get_state(pipeline, &state, nullptr, 0);
    return state == GST_STATE_PLAYING;
}

void Sound::setVolume(float v)
{
    g_object_set(G_OBJECT(volume), "volume", v, NULL);
}

auto Sound::getVolume() const -> float
{
    float v = 0.0f;
    g_object_get(G_OBJECT(volume), "volume", &v, NULL);
    return v;
}

auto Sound::getBuffer() const
  -> const std::shared_ptr<const SoundBuffer>&
{
    return buffer;
}

} // namespace sounds