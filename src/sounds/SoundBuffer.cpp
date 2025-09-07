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
    proxysink =
      gst_element_factory_make("proxysink", "proxy-sink");

    if (!pipeline || !source || !decode) {
        spdlog::error("Failed to create gstreamer pipeline");
    }

    auto filenameStr = filename.string();
    for (auto& c : filenameStr) {
        if (c == '\\') {
            c = '/';
        }
    }
    g_object_set(G_OBJECT(source), "location", filenameStr.c_str(), NULL);

    gst_bin_add_many(GST_BIN(pipeline), source, decode, proxysink, NULL);
    gst_element_link_many(source, decode, proxysink, NULL);

    /*GstBus* bus = gst_element_get_bus(pipeline);
    gst_bus_timed_pop_filtered(
      bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ASYNC_DONE);
    gst_object_unref(bus);*/
}
sounds::SoundBuffer::~SoundBuffer()
{
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
}

void
sounds::SoundBuffer::enable() const
{


    GstStateChangeReturn ret =
      gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        // print error explanation
        spdlog::error("Failed to set pipeline to GST_STATE_PLAYING state");
    }
}
auto
sounds::SoundBuffer::getBuffer() const -> GstElement*
{
    return proxysink;
}
