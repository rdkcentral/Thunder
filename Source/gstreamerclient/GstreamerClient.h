#pragma once

extern "C" {

typedef struct _GstPad GstPad;

typedef enum SinkType_t {
    THUNDER_GSTREAMER_CLIENT_AUDIO,
    THUNDER_GSTREAMER_CLIENT_VIDEO,
    THUNDER_GSTREAMER_CLIENT_TEXT
} SinkType;

/**/
int gtsreamer_client_link_sink (SinkType type, GstElement *pipeline, GstPad *srcPad);

/**/
int gtsreamer_client_unlink_sink (SinkType type, GstElement *pipeline);

};
