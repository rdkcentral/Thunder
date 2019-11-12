#include "Module.h"

#include "../gstreamerclient.h"

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>

static GstElement* findElement(GstElement *element, const char* targetName);

struct GstPlayerSink {
private:
    GstPlayerSink(const GstPlayerSink&) = delete;
    GstPlayerSink& operator= (const GstPlayerSink&) = delete;

public:
    GstPlayerSink()
            : _audioDecodeBin(nullptr)
            , _videoDecodeBin(nullptr)
            , _audioSink(nullptr)
            , _videoSink(nullptr)
            , _audioCallbacks(nullptr)
            , _videoCallbacks(nullptr) {
    }

public:


    bool ConfigureAudioSink(GstElement *pipeline, GstPad *srcPad, GstreamerClientCallbacks *callbacks, bool forcePcm) {

        TRACE_L1("Configure audio sink");

        if (_audioDecodeBin) {
            TRACE_L1("Audio Sink is already configured");
            return false;
        }
        
        if (!forcePcm) {
           // Setup audio decodebin
           _audioDecodeBin = gst_element_factory_make ("decodebin", "audio_decode");
           if (!_audioDecodeBin)
               return false;

           g_signal_connect(_audioDecodeBin, "pad-added", G_CALLBACK(OnAudioPad), this);
           g_signal_connect(_audioDecodeBin, "element-added", G_CALLBACK(OnAudioElementAdded), this);
           g_signal_connect(_audioDecodeBin, "element-removed", G_CALLBACK(OnAudioElementRemoved), this);
           g_object_set(_audioDecodeBin, "caps", gst_caps_from_string("audio/x-raw; audio/x-brcm-native"), nullptr);

           // Create an audio sink
           _audioSink = gst_element_factory_make ("brcmaudiosink", "audio-sink");

           if (!_audioSink)
               return false;

           gst_bin_add_many (GST_BIN (pipeline), _audioDecodeBin, _audioSink, nullptr);

           GstPad *pSinkPad = gst_element_get_static_pad(_audioDecodeBin, "sink");
           gst_pad_link (srcPad, pSinkPad);
           gst_object_unref(pSinkPad);

           gst_element_sync_state_with_parent(_audioDecodeBin);
           gst_element_sync_state_with_parent(_audioSink);
           _audioCallbacks = callbacks;
        } else {
           _audioSink = gst_element_factory_make ("brcmpcmsink", "audio-sink");

           if (!_audioSink)
               return false;

           gst_bin_add_many (GST_BIN (pipeline), _audioSink, nullptr);

           GstPad *pSinkPad = gst_element_get_static_pad(_audioSink, "sink");
           gst_pad_link (srcPad, pSinkPad);
           gst_object_unref(pSinkPad);

           _audioCallbacks = callbacks;
        }

        return true;
    }

    bool ConfigureVideoSink(GstElement *pipeline, GstPad *srcPad, GstreamerClientCallbacks *callbacks) {

        TRACE_L1("Configure video sink");

        if (_videoDecodeBin) {
            TRACE_L1("Video Sink is already configured");
            return false;
        }

        // Setup video decodebin
        _videoDecodeBin = gst_element_factory_make ("decodebin", "video_decode");
        if(!_videoDecodeBin)
            return false;

        g_signal_connect(_videoDecodeBin, "pad-added", G_CALLBACK(OnVideoPad), this);
        g_signal_connect(_videoDecodeBin, "element-added", G_CALLBACK(OnVideoElementAdded), this);
        g_signal_connect(_videoDecodeBin, "element-removed", G_CALLBACK(OnVideoElementRemoved), this);
        g_object_set(_videoDecodeBin, "caps", gst_caps_from_string("video/x-raw; video/x-brcm-native"), nullptr);

        // Create video sink
        _videoSink       = gst_element_factory_make ("brcmvideosink", "video-sink");
        if (!_videoSink)
            return false;

        _videoCallbacks = callbacks;
        gst_bin_add_many (GST_BIN (pipeline), _videoDecodeBin, _videoSink, nullptr);

        GstPad *pSinkPad = gst_element_get_static_pad(_videoDecodeBin, "sink");
        gst_pad_link (srcPad, pSinkPad);
        gst_object_unref (pSinkPad);

        gst_element_sync_state_with_parent(_videoDecodeBin);
        gst_element_sync_state_with_parent(_videoSink);

        return true;
    }

    void DestructSink() {

        if (_videoDecodeBin) {
            g_signal_handlers_disconnect_by_func(_videoDecodeBin, reinterpret_cast<gpointer>(OnVideoPad), this);
            g_signal_handlers_disconnect_by_func(_videoDecodeBin, reinterpret_cast<gpointer>(OnVideoElementAdded), this);
            g_signal_handlers_disconnect_by_func(_videoDecodeBin, reinterpret_cast<gpointer>(OnVideoElementRemoved), this);
        }

        if (_audioDecodeBin) {
            g_signal_handlers_disconnect_by_func(_audioDecodeBin, reinterpret_cast<gpointer>(OnAudioPad), this);
            g_signal_handlers_disconnect_by_func(_audioDecodeBin, reinterpret_cast<gpointer>(OnAudioElementAdded), this);
            g_signal_handlers_disconnect_by_func(_audioDecodeBin, reinterpret_cast<gpointer>(OnAudioElementRemoved), this);
        }
    }

    uint64_t FramesRendered() {

        guint64 decodedFrames = 0;
        if (_videoSink) {
            g_object_get(_videoSink, "frames-rendered", &decodedFrames, nullptr);
        }
        TRACE_L1("frames decoded: %llu",  decodedFrames);

        return decodedFrames;
    }

    uint64_t FramesDropped() {
        guint64 droppedFrames = 0;

        if (_videoSink) {
            g_object_get(_videoSink, "frames-dropped", &droppedFrames, nullptr);
        }
        TRACE_L1("frames dropped: %llu",  droppedFrames);

        return droppedFrames;
    }

    bool GetResolution(GstElement *pipeline, uint32_t& width, uint32_t& height)
    {
       gint sourceHeight = 0;
       gint sourceWidth = 0;

       GstElement* videoDec = findElement(pipeline, "brcmvideodecoder");
       if (!videoDec) {
          return false;
       }

       g_object_get(videoDec, "video_height", &sourceHeight, NULL);
       g_object_get(videoDec, "video_width", &sourceWidth, NULL);

       width = static_cast<uint32_t>(sourceWidth);
       height = static_cast<uint32_t>(sourceHeight);
       return true;
    }

    GstClockTime GetCurrentPosition(GstElement *pipeline)
    {
       GstClockTime currentPts = GST_CLOCK_TIME_NONE;
       GstElement* videoDec = findElement(pipeline, "brcmvideodecoder");
       if (videoDec) {
          g_object_get(videoDec, "video_pts", &currentPts, NULL);
          currentPts = (currentPts * GST_MSECOND) / 45;
       }
       return currentPts;
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

    static void OnVideoElementAdded (GstBin *decodebin2, GstElement* element, gpointer user_data) {

        GstPlayerSink *self = (GstPlayerSink*)(user_data);

        if (!self->_videoCallbacks)
            return;

        if (g_strrstr(GST_ELEMENT_NAME(element), "brcmvideodecoder"))
        {
            g_signal_connect(element, "buffer-underflow-callback",
                             G_CALLBACK(self->_videoCallbacks->buffer_underflow_callback), self->_videoCallbacks->user_data);
        }
    }

    static void OnVideoElementRemoved (GstBin *decodebin2, GstElement* element, gpointer user_data) {

        GstPlayerSink *self = (GstPlayerSink*)(user_data);

        if (!self->_videoCallbacks)
            return;

        if (g_strrstr(GST_ELEMENT_NAME(element), "brcmvideodecoder"))
        {
            g_signal_handlers_disconnect_by_func (element,
                                                  reinterpret_cast<gpointer>(self->_videoCallbacks->buffer_underflow_callback), self->_videoCallbacks->user_data);
        }
    }

    static void OnAudioElementAdded (GstBin *decodebin2, GstElement* element, gpointer user_data) {

        GstPlayerSink *self = (GstPlayerSink*)(user_data);

        if (!self->_audioCallbacks)
            return;

        if (g_strrstr(GST_ELEMENT_NAME(element), "brcmaudiodecoder"))
        {
            g_signal_connect(element, "buffer-underflow-callback",
                             G_CALLBACK(self->_audioCallbacks->buffer_underflow_callback), self->_videoCallbacks->user_data);
        }
    }

    static void OnAudioElementRemoved (GstBin *decodebin2, GstElement* element, gpointer user_data) {

        GstPlayerSink *self = (GstPlayerSink*)(user_data);

        if (!self->_audioCallbacks)
            return;

        if (g_strrstr(GST_ELEMENT_NAME(element), "brcmaudiodecoder"))
        {
            g_signal_handlers_disconnect_by_func(element,
                                                 reinterpret_cast<gpointer>(self->_audioCallbacks->buffer_underflow_callback), self->_videoCallbacks->user_data);
        }
    }

private:
    GstElement *_audioDecodeBin;
    GstElement *_videoDecodeBin;
    GstElement *_audioSink;
    GstElement *_videoSink;
    GstreamerClientCallbacks *_audioCallbacks;
    GstreamerClientCallbacks *_videoCallbacks;
    // TODO: store pipeline, video decoder, audio decoder
};

struct GstPlayer {
private:
    GstPlayer(const GstPlayer&) = delete;
    GstPlayer& operator= (const GstPlayer&) = delete;
    GstPlayer()
        : _sinks() {
    }

public:
    typedef std::map<GstElement*, GstPlayerSink*> SinkMap;

    static GstPlayer* Instance() {
        static GstPlayer *instance = new GstPlayer();

        return (instance);
    }

    void Add (GstElement* pipeline, GstPlayerSink* sink) {
        SinkMap::iterator index(_sinks.find(pipeline));

        if (index == _sinks.end()) {
            _sinks.insert(std::pair<GstElement*, GstPlayerSink*>(pipeline, sink));
        } else {
            printf("Same pipeline created!\n");
        }
    }

    void Remove(GstElement* pipeline) {

        SinkMap::iterator index(_sinks.find(pipeline));

        if (index != _sinks.end()) {
            _sinks.erase(index);
        } else {
            printf("Could not find a pipeline to remove");
        }
    }

    GstPlayerSink* Find(GstElement* pipeline) {

        GstPlayerSink* result = nullptr;
        SinkMap::iterator index(_sinks.find(pipeline));

        if (index != _sinks.end()) {
            result = index->second;
        }
        return result;
    }

private:
    SinkMap _sinks;
};


static GstElement* findElement(GstElement *element, const char* targetName)
{
    GstElement *re = NULL;
    if (GST_IS_BIN(element)) {
        GstIterator* it = gst_bin_iterate_elements(GST_BIN(element));
        GValue item = G_VALUE_INIT;
        bool done = false;
        while(!done) {
            switch (gst_iterator_next(it, &item)) {
                case GST_ITERATOR_OK:
                {
                    GstElement *next = GST_ELEMENT(g_value_get_object(&item));
                    done = (re = findElement(next, targetName)) != NULL;
                    g_value_reset (&item);
                    break;
                }
                case GST_ITERATOR_RESYNC:
                    gst_iterator_resync (it);
                    break;
                case GST_ITERATOR_ERROR:
                case GST_ITERATOR_DONE:
                    done = true;
                    break;
            }
        }
        g_value_unset (&item);
        gst_iterator_free(it);
    } else {
        if (strstr(gst_element_get_name(element), targetName)) {
            re = element;
        }
    }
    return re;
}

extern "C" {

int gstreamer_client_sink_link (SinkType type, GstElement *pipeline, GstPad *srcPad, GstreamerClientCallbacks* callbacks, bool forcePcm)
{
    struct GstPlayer* instance = GstPlayer::Instance();
    int result = 0;

    GstPlayerSink *sink =  instance->Find(pipeline);
    if (!sink) {
        // create a new sink
        sink = new GstPlayerSink();
        instance->Add(pipeline, sink);
    }

    switch (type) {
        case THUNDER_GSTREAMER_CLIENT_AUDIO:
            if (!sink->ConfigureAudioSink(pipeline, srcPad, callbacks, forcePcm)) {
                gstreamer_client_sink_unlink(type, pipeline);
                result = -1;
            }
            break;
        case THUNDER_GSTREAMER_CLIENT_VIDEO:
            if (!sink->ConfigureVideoSink(pipeline, srcPad, callbacks)) {
                gstreamer_client_sink_unlink(type, pipeline);
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

int gstreamer_client_sink_unlink (SinkType type, GstElement *pipeline)
{
    struct GstPlayer* instance = GstPlayer::Instance();

    GstPlayerSink *sink =  instance->Find(pipeline);
    if (!sink)
        return -1;

    // Cleanup linked signals, pads etc.
    sink->DestructSink();

    // Remove from pipeline list
    instance->Remove(pipeline);

    // pipeline destruction will free allocated elements
    return 0;
}

unsigned long gtsreamer_client_sink_frames_rendered (GstElement *pipeline)
{
    struct GstPlayer* instance = GstPlayer::Instance();
    GstPlayerSink *sink =  instance->Find(pipeline);

    if (!sink)
        return 0;

    return (sink->FramesRendered());
}

unsigned long gtsreamer_client_sink_frames_dropped (GstElement *pipeline)
{
    struct GstPlayer* instance = GstPlayer::Instance();
    GstPlayerSink *sink =  instance->Find(pipeline);

    if (!sink)
        return 0;

    return (sink->FramesDropped());
}

int gstreamer_client_post_eos (GstElement * element)
{
   // Nexus:
   gst_app_src_end_of_stream(GST_APP_SRC(element));
   // Rest:
   // gst_element_post_message(mSrc, gst_message_new_eos(GST_OBJECT(mSrc)));
   // See: https://github.com/Metrological/netflix/blob/f5646af2f6b5fd9690681366908a2e711ae1d021/partner/dpi/gstreamer/ESPlayerGst.cpp#L334
   return 0;
}

int gstreamer_client_can_report_stale_pts ()
{
    return 1;
    // All others will return 0
    // See: https://github.com/Metrological/netflix/blob/f5646af2f6b5fd9690681366908a2e711ae1d021/partner/dpi/gstreamer/ESPlayerGst.cpp#L747
}

int gstreamer_client_set_volume(GstElement *pipeline, double volume)
{
   const float scaleFactor = 100.0; // For all others is 1.0 (so no scaling)
   GstElement * audioSink = findElement(pipeline, "audio-sink");

   g_object_set(G_OBJECT(audioSink), "volume", 1.0 * scaleFactor, NULL);

   return 0;
}

int gstreamer_client_get_resolution(GstElement *pipeline, uint32_t * width, uint32_t * height)
{
   GstPlayer* instance = GstPlayer::Instance();
   GstPlayerSink *sink =  instance->Find(pipeline);
   bool success = sink->GetResolution(pipeline, *width, *height);
   return (success ? 1 : 0);
}

GstClockTime gstreamer_client_get_current_position(GstElement *pipeline)
{
   GstPlayer* instance = GstPlayer::Instance();
   GstPlayerSink *sink =  instance->Find(pipeline);
   return sink->GetCurrentPosition(pipeline);

   // For other platforms, see:
   // https://github.com/Metrological/netflix/blob/1fb2cb81c75efd7c89f25f84aae52919c6d1fece/partner/dpi/gstreamer/PlaybackGroupNative.cpp#L1003-L1010
}


};
