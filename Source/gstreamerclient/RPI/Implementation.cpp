
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
#include "compositorclient/Client.h"
#include "interfaces/IComposition.h"

#include <gst/gst.h>
#include <gst/video/videooverlay.h>

struct GstPlayerSink {
private:
    GstPlayerSink(const GstPlayerSink&) = delete;
    GstPlayerSink& operator= (const GstPlayerSink&) = delete;

    GstPlayerSink()
            : _audioDecodeBin(nullptr)
            , _videoDecodeBin(nullptr)
            , _audioSink(nullptr)
            , _videoSink(nullptr)
            , _surface(nullptr) {
    }

public:
    static GstPlayerSink* Instance() {
        static GstPlayerSink *instance = new GstPlayerSink();

        return (instance);
    }

    bool ConfigureAudioSink(GstElement *pipeline, GstElement *appSrc) {

        // Setup audio decodebin
        _audioDecodeBin = gst_element_factory_make ("decodebin", "audio_decode");
        if (!_audioDecodeBin)
            return false;

        g_signal_connect(_audioDecodeBin, "pad-added", G_CALLBACK(OnAudioPad), this);
        g_object_set(_audioDecodeBin, "caps", gst_caps_from_string("audio/x-raw;"), nullptr);

        // Create an audio sink
        _audioSink = gst_element_factory_make ("autoaudiosink", "audio-sink");
        if (!_audioSink)
            return false;

        gst_bin_add_many (GST_BIN (pipeline), _audioDecodeBin, _audioSink, nullptr);

        if (gst_element_link(appSrc, _audioDecodeBin) == false) {
            TRACE_L1("Failed to link video source with decoder");
            return false;
        }

        return true;
    }

    bool ConfigureVideoSink(GstElement *pipeline, GstElement *appSrc) {

        // Initialize Surface
        if (!_surface)
            _surface = WPEFramework::Compositor::IDisplay::Instance("AmazonPlayer")->Create("AmazonVideo", 0, 0);

        // Setup video decodebin
        _videoDecodeBin = gst_element_factory_make ("decodebin", "video_decode");
        if(!_videoDecodeBin)
            return false;

        g_signal_connect(_videoDecodeBin, "pad-added", G_CALLBACK(OnVideoPad), this);
        g_object_set(_videoDecodeBin, "caps", gst_caps_from_string("video/x-raw;"), nullptr);

        // Create video sink
        _videoSink       = gst_element_factory_make ("videoscale", "video_scale");
        if (!_videoSink) {
            TRACE_L1("Failed to create video-scaling bin!");
            return false;
        }

        _glImage = gst_element_factory_make ("glimagesink", "glimagesink0");
        if (_glImage == nullptr) {
            TRACE_L1("Failed to create glimagesink!");
            return false;
        }

        gst_bin_add_many (GST_BIN (pipeline), _videoDecodeBin, _videoSink, _glImage, nullptr);

        if (gst_element_link(appSrc, _videoDecodeBin) == false) {
            TRACE_L1("Failed to link video source with decoder");
            return false;
        }

        if (gst_element_link_filtered(_videoSink, _glImage, gst_caps_from_string("video/x-raw,width=1280,height=720")) == false) {
            TRACE_L1("Failed to link video scaling with image sink");
            return false;
        }

        GstBus *bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
        gst_bus_set_sync_handler (bus, (GstBusSyncHandler) createWindow, this, NULL);
        gst_object_unref(bus);

        return true;
    }

    void Deinitialise() {

        if (_surface) {
            // Delete surface
            _surface->Release();
            _surface = nullptr;
        }
    }
private:

    static GstBusSyncReply createWindow (GstBus * bus, GstMessage * message, gpointer user_data) {
        // ignore anything but 'prepare-window-handle' element messages
        if (!gst_is_video_overlay_prepare_window_handle_message(message))
            return GST_BUS_PASS;

        GstPlayerSink *self = (GstPlayerSink*)(user_data);

        GstVideoOverlay *overlay = GST_VIDEO_OVERLAY (GST_MESSAGE_SRC (message));
        gst_video_overlay_set_window_handle (overlay, (guintptr)self->_surface->Native());

        return GST_BUS_DROP;

    }
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

            //self->_surface->setLayer(-1);

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
                printf("Could not make link audio sink to bin\n");
            }
        }
        gst_caps_unref(caps);
    }

private:
    GstElement *_audioDecodeBin;
    GstElement *_videoDecodeBin;
    GstElement *_audioSink;
    GstElement *_videoSink;
    GstElement *_glImage;

    WPEFramework::Compositor::IDisplay::ISurface* _surface;
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
    GstPlayerSink::Instance()->Deinitialise();

    return 0;
}

};
