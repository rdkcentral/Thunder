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

#ifdef __cplusplus
}
#endif

