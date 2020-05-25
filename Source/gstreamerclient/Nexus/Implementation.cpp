/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "Module.h"

#include "gstreamerclient.h"

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <core/core.h>

using namespace WPEFramework;
using SafeCriticalSection = Core::SafeSyncType<Core::CriticalSection>;

static Core::CriticalSection g_adminLock;

struct GstPlayerSink {
private:
    GstPlayerSink(const GstPlayerSink&) = delete;
    GstPlayerSink& operator= (const GstPlayerSink&) = delete;

public:
    GstPlayerSink(GstElement * pipeline)
            : _pipeline(pipeline)
            , _audioDecodeBin(nullptr)
            , _videoDecodeBin(nullptr)
            , _audioSink(nullptr)
            , _videoSink(nullptr)
            , _videoDec(nullptr)
            , _audioCallbacks(nullptr)
            , _videoCallbacks(nullptr) {
    }

public:


    bool ConfigureAudioSink(GstElement * srcElement, GstPad *srcPad, GstreamerClientCallbacks *callbacks) {

        TRACE_L1("Configure audio sink");

        if (_audioDecodeBin) {
            TRACE_L1("Audio Sink is already configured");
            return false;
        }

        // Special case: BCM audio decoder doesn't support PCM. If PCM is requested, we need
        // to create a PCM sink instead and connect the source to it.
        GValue capsValue = G_VALUE_INIT;
        g_value_init (&capsValue, GST_TYPE_CAPS);
        g_object_get_property(G_OBJECT(srcElement), "caps", &capsValue);
        const GstCaps * caps = gst_value_get_caps(&capsValue);
        gchar * capsStr = gst_caps_to_string(caps);
        g_value_unset(&capsValue);

        // Check if it is "audio/x-raw" (PCM).
        bool isPcm = false;
        if (capsStr) {
           isPcm = strstr(capsStr, "audio/x-raw");
        }
        g_free(capsStr);

        if (!isPcm) {
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

           gst_bin_add_many (GST_BIN (_pipeline), _audioDecodeBin, _audioSink, nullptr);

           if (srcPad != nullptr) {
              // Link via pad
              GstPad *pSinkPad = gst_element_get_static_pad(_audioDecodeBin, "sink");
              gst_pad_link (srcPad, pSinkPad);
              gst_object_unref(pSinkPad);
           } else {
              // Link elements
              gst_element_link(srcElement, _audioDecodeBin);
           }

           gst_element_sync_state_with_parent(_audioDecodeBin);
        } else {
           _audioSink = gst_element_factory_make ("brcmpcmsink", "audio-sink");

           if (!_audioSink)
               return false;

           gst_bin_add_many (GST_BIN (_pipeline), _audioSink, nullptr);

           if (srcPad != nullptr) {
              GstPad *pSinkPad = gst_element_get_static_pad(_audioSink, "sink");
              gst_pad_link (srcPad, pSinkPad);
              gst_object_unref(pSinkPad);
           } else {
              gst_element_link(srcElement, _audioDecodeBin);
           }


        }
        _audioCallbacks = callbacks;
        gst_element_sync_state_with_parent(_audioSink);

        return true;
    }

    bool ConfigureVideoSink(GstElement * srcElement, GstPad *srcPad, GstreamerClientCallbacks *callbacks) {

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
        gst_bin_add_many (GST_BIN (_pipeline), _videoDecodeBin, _videoSink, nullptr);

        if (srcPad != nullptr) {
           // Link via pad
           GstPad *pSinkPad = gst_element_get_static_pad(_videoDecodeBin, "sink");
           gst_pad_link (srcPad, pSinkPad);
           gst_object_unref(pSinkPad);
        } else {
           // Link elements
           gst_element_link(srcElement, _videoDecodeBin);
        }

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

    bool GetResolution(uint32_t& width, uint32_t& height)
    {
       gint sourceHeight = 0;
       gint sourceWidth = 0;

       if (!_videoDec) {
          TRACE_L1("No video decoder when querying resolution");
          return false;
       }

       g_object_get(_videoDec, "video_height", &sourceHeight, NULL);
       g_object_get(_videoDec, "video_width", &sourceWidth, NULL);

       width = static_cast<uint32_t>(sourceWidth);
       height = static_cast<uint32_t>(sourceHeight);
       return true;
    }

    GstClockTime GetCurrentPosition()
    {
       GstClockTime currentPts = GST_CLOCK_TIME_NONE;
       if (!_videoDec) {
          TRACE_L1("No video decoder when querying position");
          return false;
       }

       g_object_get(_videoDec, "video_pts", &currentPts, NULL);
       currentPts = (currentPts * GST_MSECOND) / 45;

       return currentPts;
    }
    
    bool MoveVideoRectangle(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
    {
       if (_videoSink == nullptr) {
          TRACE_L1("No video sink to move rectangle for");
          return false;
       }

       char rectString[64];
       sprintf(rectString,"%d,%d,%d,%d", x, y, width, height);
       g_object_set(_videoSink, "rectangle", rectString, nullptr);
      
       return true;
    }

    bool SetVolume(double volume)
    {
      const float scaleFactor = 100.0; // For all others is 1.0 (so no scaling)

      g_object_set(G_OBJECT(_audioSink), "volume", volume * scaleFactor, NULL);

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

        if (g_strrstr(name, "video/x-")) {

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

        if (g_strrstr(name, "audio/x-")) {
            if (gst_element_link(self->_audioDecodeBin, self->_audioSink) == FALSE) {
                TRACE_L1("Could not make link video sink to bin");
            }
        }
        gst_caps_unref(caps);
    }

    static void OnVideoElementAdded (GstBin *decodebin2, GstElement* element, gpointer user_data) {

        GstPlayerSink *self = (GstPlayerSink*)(user_data);

        if (g_strrstr(GST_ELEMENT_NAME(element), "brcmvideodecoder")) {
            self->_videoDec = element;

            if (self->_videoCallbacks) {

                g_signal_connect(element, "buffer-underflow-callback",
                                 G_CALLBACK(self->_videoCallbacks->buffer_underflow_callback),
                                 self->_videoCallbacks->user_data);
            }
        }
    }

    static void OnVideoElementRemoved (GstBin *decodebin2, GstElement* element, gpointer user_data) {

        GstPlayerSink *self = (GstPlayerSink*)(user_data);

        if (g_strrstr(GST_ELEMENT_NAME(element), "brcmvideodecoder")) {

            self->_videoDec = nullptr;
            if (self->_videoCallbacks) {
                g_signal_handlers_disconnect_by_func(element,
                                                     reinterpret_cast<gpointer>(self->_videoCallbacks->buffer_underflow_callback),
                                                     self->_videoCallbacks->user_data);
            }
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
    GstElement *_pipeline;
    GstElement *_audioDecodeBin;
    GstElement *_videoDecodeBin;
    GstElement *_audioSink;
    GstElement *_videoSink;
    GstElement *_videoDec;
    GstreamerClientCallbacks *_audioCallbacks;
    GstreamerClientCallbacks *_videoCallbacks;
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
            TRACE_L1("Same pipeline created!");
        }
    }

    void Remove(GstElement* pipeline) {

        SinkMap::iterator index(_sinks.find(pipeline));

        if (index != _sinks.end()) {
            _sinks.erase(index);
        } else {
            TRACE_L1("Could not find a pipeline to remove");
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

extern "C" {

int gstreamer_client_sink_link (SinkType type, GstElement *pipeline, GstElement * srcElement, GstPad *srcPad, GstreamerClientCallbacks* callbacks)
{
    SafeCriticalSection lock(g_adminLock);
    struct GstPlayer* instance = GstPlayer::Instance();
    int result = 0;

    GstPlayerSink *sink =  instance->Find(pipeline);
    if (!sink) {
        // create a new sink
        sink = new GstPlayerSink(pipeline);
        instance->Add(pipeline, sink);
    }

    switch (type) {
        case THUNDER_GSTREAMER_CLIENT_AUDIO:
            if (!sink->ConfigureAudioSink(srcElement, srcPad, callbacks)) {
                gstreamer_client_sink_unlink(type, pipeline);
                result = -1;
            }
            break;
        case THUNDER_GSTREAMER_CLIENT_VIDEO:
            if (!sink->ConfigureVideoSink(srcElement, srcPad, callbacks)) {
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
    SafeCriticalSection lock(g_adminLock);
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
    SafeCriticalSection lock(g_adminLock);
    struct GstPlayer* instance = GstPlayer::Instance();
    GstPlayerSink *sink =  instance->Find(pipeline);

    if (!sink)
        return 0;

    return (sink->FramesRendered());
}

unsigned long gtsreamer_client_sink_frames_dropped (GstElement *pipeline)
{
    SafeCriticalSection lock(g_adminLock);
    struct GstPlayer* instance = GstPlayer::Instance();
    GstPlayerSink *sink =  instance->Find(pipeline);

    if (!sink)
        return 0;

    return (sink->FramesDropped());
}

int gstreamer_client_post_eos (GstElement * element)
{
    SafeCriticalSection lock(g_adminLock);
   // Nexus:
   gst_app_src_end_of_stream(GST_APP_SRC(element));
   // Rest:
   // gst_element_post_message(mSrc, gst_message_new_eos(GST_OBJECT(mSrc)));
   // See: https://github.com/Metrological/netflix/blob/f5646af2f6b5fd9690681366908a2e711ae1d021/partner/dpi/gstreamer/ESPlayerGst.cpp#L334
   return 0;
}

int gstreamer_client_can_report_stale_pts ()
{
    SafeCriticalSection lock(g_adminLock);
    return 1;
    // All others will return 0
    // See: https://github.com/Metrological/netflix/blob/f5646af2f6b5fd9690681366908a2e711ae1d021/partner/dpi/gstreamer/ESPlayerGst.cpp#L747
}

int gstreamer_client_set_volume(GstElement *pipeline, double volume)
{
   SafeCriticalSection lock(g_adminLock);
   GstPlayer* instance = GstPlayer::Instance();
   GstPlayerSink *sink =  instance->Find(pipeline);
   if (sink == nullptr) {
      TRACE_L1("Trying to set volume for unregistered pipeline");
      return 0;
   }
   bool success = sink->SetVolume(volume);
   return (success ? 1 : 0);
}

int gstreamer_client_get_resolution(GstElement *pipeline, uint32_t * width, uint32_t * height)
{
   SafeCriticalSection lock(g_adminLock);
   GstPlayer* instance = GstPlayer::Instance();
   GstPlayerSink *sink =  instance->Find(pipeline);
   if (sink == nullptr) {
      TRACE_L1("Trying to get resolution for unregistered pipeline");
      return 0;
   }
   bool success = sink->GetResolution(*width, *height);
   return (success ? 1 : 0);
}

GstClockTime gstreamer_client_get_current_position(GstElement *pipeline)
{
   SafeCriticalSection lock(g_adminLock);
   GstPlayer* instance = GstPlayer::Instance();
   GstPlayerSink *sink =  instance->Find(pipeline);
   if (sink == nullptr) {
      TRACE_L1("Trying to get current position for unregistered pipeline");
      return 0;
   }
   return sink->GetCurrentPosition();

   // For other platforms, see:
   // https://github.com/Metrological/netflix/blob/1fb2cb81c75efd7c89f25f84aae52919c6d1fece/partner/dpi/gstreamer/PlaybackGroupNative.cpp#L1003-L1010
}

int gstreamer_client_move_video_rectangle(GstElement *pipeline, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
  SafeCriticalSection lock(g_adminLock);
  GstPlayer* instance = GstPlayer::Instance();
  GstPlayerSink *sink =  instance->Find(pipeline);
  if (sink == nullptr) {
      TRACE_L1("Trying to move video rectangle for unregistered pipeline");
      return 0;
  }
  
  bool success = sink->MoveVideoRectangle(x, y, width, height);
  return (success ? 1 : 0);
}

};
