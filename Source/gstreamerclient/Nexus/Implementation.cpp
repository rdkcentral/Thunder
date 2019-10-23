#include "Module.h"

#include "../GstreamerClient.h"

#include <gst/gst.h>

struct GstPlayerSink {
private:
    GstPlayerSink(const GstPlayerSink&) = delete;
    GstPlayerSink& operator= (const GstPlayerSink&) = delete;

    GstPlayerSink()
            : _audioDecodeBin(nullptr)
            , _videoDecodeBin(nullptr)
            , _audioSink(nullptr)
            , _videoSink(nullptr) {
    }

public:
    static GstPlayerSink* Instance() {
        static GstPlayerSink *instance = new GstPlayerSink();

        return (instance);
    }

    bool ConfigureAudioSink(GstElement *pipeline, GstElement *appSrc) {

        TRACE_L1("Configure audio sink");
        // Setup audio decodebin
        _audioDecodeBin = gst_element_factory_make ("decodebin", "audio_decode");
        if (!_audioDecodeBin)
            return false;

        g_signal_connect(_audioDecodeBin, "pad-added", G_CALLBACK(OnAudioPad), this);
        g_object_set(_audioDecodeBin, "caps", gst_caps_from_string("audio/x-raw; audio/x-brcm-native"), nullptr);

        // Create an audio sink
        _audioSink = gst_element_factory_make ("brcmaudiosink", "audio-sink");

        if (!_audioSink)
            return false;

        gst_bin_add_many (GST_BIN (pipeline), _audioDecodeBin, _audioSink, nullptr);
        gst_element_link(appSrc, _audioDecodeBin);

        return true;
    }

    bool ConfigureVideoSink(GstElement *pipeline, GstElement *appSrc) {

        TRACE_L1("Configure video sink");
        // Setup video decodebin
        _videoDecodeBin = gst_element_factory_make ("decodebin", "video_decode");
        if(!_videoDecodeBin)
            return false;

        g_signal_connect(_videoDecodeBin, "pad-added", G_CALLBACK(OnVideoPad), this);
        g_object_set(_videoDecodeBin, "caps", gst_caps_from_string("video/x-raw; video/x-brcm-native"), nullptr);

        // Create video sink
        _videoSink       = gst_element_factory_make ("brcmvideosink", "video-sink");
        if (!_videoSink)
            return false;

        gst_bin_add_many (GST_BIN (pipeline), _videoDecodeBin, _videoSink, nullptr);
        gst_element_link(appSrc, _videoDecodeBin);

        return true;
    }
private:
    static void OnVideoPad (GstElement *decodebin2, GstPad *pad, gpointer user_data) {

        GstPlayerSink *self = (GstPlayerSink*)(user_data);

        GstCaps *caps;
        GstStructure *structure;
        const gchar *name;

        caps = gst_pad_query_caps(pad,nullptr);
        structure = gst_caps_get_structure(caps,0);
        name = gst_structure_get_name(structure);

        if (g_strrstr(name, "video/x-"))
        {

            GValue window_set = {0, };
            static char str[40];
            std::snprintf(str, 40, "%d,%d,%d,%d", 0,0, 1280, 720);

            g_value_init(&window_set, G_TYPE_STRING);
            g_value_set_static_string(&window_set, str);
            g_object_set(self->_videoSink, "window_set", str, nullptr);
            g_object_set(self->_videoSink, "zorder", 0, nullptr);
            g_object_set(self->_videoSink, "zoom-mode", 1, nullptr); // By default box mode

            if (gst_element_link(self->_videoDecodeBin, self->_videoSink) == FALSE) {
                TRACE_L1("Could not make link video sink to bin");
            }
        }
        gst_caps_unref(caps);
    }

    static void OnAudioPad (GstElement *decodebin2, GstPad *pad, gpointer user_data) {
        GstPlayerSink *self = (GstPlayerSink*)(user_data);

        GstCaps *caps;
        GstStructure *structure;
        const gchar *name;

        caps = gst_pad_query_caps(pad,nullptr);
        structure = gst_caps_get_structure(caps,0);
        name = gst_structure_get_name(structure);

        if (g_strrstr(name, "audio/x-"))
        {
            if (gst_element_link(self->_audioDecodeBin, self->_audioSink) == FALSE) {
                TRACE_L1("Could not make link video sink to bin");
            }
        }
        gst_caps_unref(caps);
    }

private:
    GstElement *_audioDecodeBin;
    GstElement *_videoDecodeBin;
    GstElement *_audioSink;
    GstElement *_videoSink;
};

extern "C" {

int gtsreamer_client_link_sink (SinkType type, GstElement *pipeline, GstElement *appSrc)
{
    struct GstPlayerSink* instance = GstPlayerSink::Instance();
    int result = 0;

    switch (type) {
        case THUNDER_GSTREAMER_CLIENT_AUDIO:
            if (!instance->ConfigureAudioSink(pipeline, appSrc)) {
                result = -1;
            }
            break;
        case THUNDER_GSTREAMER_CLIENT_VIDEO:
            if (!instance->ConfigureVideoSink(pipeline, appSrc)) {
                result = -1;
            }
            break;
        case THUNDER_GSTREAMER_CLIENT_TEXT:
        default:
            result = -1;
            break;
    }
    return result;
}

int gtsreamer_client_unlink_sink (SinkType type, GstElement *pipeline)
{
    // pipeline destruction will free allocated elements
    return 0;
}

};