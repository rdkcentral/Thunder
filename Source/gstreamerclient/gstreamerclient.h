#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _GstPad GstPad;
typedef struct _GstElement GstElement;

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
    void (*buffer_underflow_callback)(GstElement *element, uint32_t value1, uint32_t value2, void* user_data);

} GstreamerClientCallbacks;

/**/
int gstreamer_client_sink_link (SinkType type, GstElement *pipeline, GstPad *srcPad, GstreamerClientCallbacks* callbacks);

/**/
int gstreamer_client_sink_unlink (SinkType type, GstElement *pipeline);

/**/
unsigned long gtsreamer_client_sink_frames_rendered (GstElement *pipeline);

/**/
unsigned long gtsreamer_client_sink_frames_dropped (GstElement *pipeline);

int gstreamer_client_post_eos (GstElement * element);

int gstreamer_client_can_report_stale_pts ();

int gstreamer_client_set_volume(GstElement *pipeline, double volume);

#ifdef __cplusplus
}
#endif

