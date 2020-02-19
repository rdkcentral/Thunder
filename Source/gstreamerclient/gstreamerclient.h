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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// TODO: include gst headers?
typedef unsigned long long guint64;
typedef struct _GstPad GstPad;
typedef struct _GstElement GstElement;
typedef guint64 GstClockTime;

typedef enum SinkType_t {
    THUNDER_GSTREAMER_CLIENT_AUDIO,
    THUNDER_GSTREAMER_CLIENT_VIDEO,
    THUNDER_GSTREAMER_CLIENT_TEXT
} SinkType;

/**
 * Registered callbacks.
 */
typedef struct {
    /**/
    // TODO: check these arguments, seems like gstreamer doesn't use uint32_t
    void (*buffer_underflow_callback)(GstElement *element, uint32_t value1, uint32_t value2, void* user_data);
    void *user_data;

} GstreamerClientCallbacks;

/**/
int gstreamer_client_sink_link (SinkType type, GstElement *pipeline, GstElement * srcElement, GstPad *srcPad, GstreamerClientCallbacks* callbacks);

/**/
int gstreamer_client_sink_unlink (SinkType type, GstElement *pipeline);

/**/
unsigned long gtsreamer_client_sink_frames_rendered (GstElement *pipeline);

/**/
unsigned long gtsreamer_client_sink_frames_dropped (GstElement *pipeline);

int gstreamer_client_post_eos (GstElement * element);

int gstreamer_client_can_report_stale_pts ();

int gstreamer_client_set_volume(GstElement *pipeline, double volume);

int gstreamer_client_get_resolution(GstElement *pipeline, uint32_t * width, uint32_t * height);

GstClockTime gstreamer_client_get_current_position(GstElement *pipeline);

int gstreamer_client_move_video_rectangle(GstElement *pipeline, uint32_t x, uint32_t y, uint32_t width, uint32_t height);

#ifdef __cplusplus
}
#endif
