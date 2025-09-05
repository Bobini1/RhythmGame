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
    pipeline = gst_pipeline_new("sound-player");
    volume = gst_element_factory_make("volume", "volume");

    if (!pipeline || !player || !volume) {
        g_printerr("Can't create.\n");
        throw std::runtime_error("Failed to create pipeline.");
    }
    // connect
    gst_bin_add_many(GST_BIN(pipeline), buffer->getBuffer(), volume, player);
    if (!gst_element_link(buffer->getBuffer(), volume) ||
        !gst_element_link(volume, player)) {
        g_printerr("Can't connect.\n");
        gst_object_unref(pipeline);
        pipeline = nullptr;
        throw std::runtime_error("Failed to link elements.");
    }
}

Sound::~Sound()
{
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
}

void Sound::play()
{
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