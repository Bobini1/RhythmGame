//
// Created by bobini on 15.04.23.
//

#include "SoundBuffer.h"

#include <spdlog/spdlog.h>
#include <sndfile.hh>
#include <gst/gst.h>

sounds::SoundBuffer::SoundBuffer(const std::filesystem::path& filename)
  : pipeline(gst_pipeline_new("sound-pipeline"))
{
    GstElement* source = gst_element_factory_make("filesrc", "file-source");
    GstElement* decode = gst_element_factory_make("decodebin", "decode");

    if (!pipeline || !source || !decode) {
        throw std::runtime_error("Failed to create gstreamer pipeline");
    }

    g_object_set(G_OBJECT(source), "location", filename.c_str(), NULL);

    gst_bin_add_many(GST_BIN(pipeline), source, decode, NULL);
    gst_element_link(source, decode);

    GstStateChangeReturn ret =
      gst_element_set_state(pipeline, GST_STATE_PAUSED);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Can't load pipeline.\n");
        throw std::runtime_error("Failed to load sound stream.");
    }

    GstBus* bus = gst_element_get_bus(pipeline);
    gst_bus_timed_pop_filtered(
      bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ASYNC_DONE);
    gst_object_unref(bus);
}
sounds::SoundBuffer::~SoundBuffer()
{
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
}
auto
sounds::SoundBuffer::getBuffer() const -> GstElement*
{
    return pipeline;
}
