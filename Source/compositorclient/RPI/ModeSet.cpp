#include "ModeSet.h"

#include <vector>
#include <list>
#include <string>
#include <cassert>

#ifdef __cplusplus
extern "C"
{
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <drm/drm_fourcc.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>

#ifdef __cplusplus
}
#endif

#include <limits>

static constexpr uint8_t DrmMaxDevices()
{
    // Just an arbitrary choice
    return 16;
}

static void GetNodes(uint32_t type, std::vector<std::string>& list)
{
    drmDevicePtr devices[DrmMaxDevices()];

    static_assert(sizeof(DrmMaxDevices()) <= sizeof(int));
    static_assert(std::numeric_limits<decltype(DrmMaxDevices())>::max() <= std::numeric_limits<int>::max());

    int device_count = drmGetDevices2(0 /* flags */, &devices[0], static_cast<int>(DrmMaxDevices()));

    if (device_count > 0)
    {
        for (int i = 0; i < device_count; i++)
        {
            switch (type)
            {
                case DRM_NODE_PRIMARY   :   // card<num>, always created, KMS, privileged
                case DRM_NODE_CONTROL   :   // ControlD<num>, currently unused
                case DRM_NODE_RENDER    :   // Solely for render clients, unprivileged
                                            {
                                                if ((1 << type) == (devices[i]->available_nodes & (1 << type)))
                                                {
                                                    list.push_back( std::string(devices[i]->nodes[type]) );
                                                }
                                                break;
                                            }
                case DRM_NODE_MAX       :
                default                 :   // Unknown (new) node type
                                        ;
            }
        }

        drmFreeDevices(&devices[0], device_count);
    }
}

static int FileDescriptor()
{
    static int fd = -1;

    if(fd < 0) {
        std::vector<std::string> nodes;
        GetNodes(DRM_NODE_PRIMARY, nodes);

        std::vector<std::string>::iterator index(nodes.begin());

        while ((index != nodes.end()) && (fd == -1)) {
            // Select the first from the list
            if (index->empty() == false)
            {
                // The node might be priviliged and the call will fail.
                // Do not close fd with exec functions! No O_CLOEXEC!
                fd = open(index->c_str(), O_RDWR); 
            }
            index++;
        }
    }

    return (fd);
}

static uint32_t GetConnectors(int fd, uint32_t type)
{
    uint32_t bitmask = 0;

    drmModeResPtr resources = drmModeGetResources(fd);

    if(nullptr != resources)
    {
        for (int i = 0; i < resources->count_connectors; i++)
        {
            drmModeConnectorPtr connector = drmModeGetConnector(fd, resources->connectors[i]);

            if(nullptr != connector)
            {
                if ((type == connector->connector_type) && (DRM_MODE_CONNECTED == connector->connection))
                {
                    bitmask = bitmask | (1 << i);
                }

                drmModeFreeConnector(connector);
            }
        }

        drmModeFreeResources(resources);
    }

    return bitmask;
}

static uint32_t GetCRTCS(int fd, bool valid)
{
    uint32_t bitmask = 0;

    drmModeResPtr resources = drmModeGetResources(fd);

    if(nullptr != resources)
    {
        for(int i = 0; i < resources->count_crtcs; i++)
        {
            drmModeCrtcPtr crtc = drmModeGetCrtc(fd, resources->crtcs[i]);

            if(nullptr != crtc)
            {
                bool currentSet = (crtc->mode_valid == 1);

                if(valid == currentSet)
                {
                    bitmask = bitmask | (1 << i);
                }
                drmModeFreeCrtc(crtc);
            }
        }

        drmModeFreeResources(resources);
    }

    return bitmask;
}

static bool FindProperDisplay(int fd, uint32_t& crtc, uint32_t& encoder, uint32_t& connector, uint32_t& fb)
{
    bool found = false;

    assert(fd != -1);

    // Only connected connectors are considered
    uint32_t connectorMask = GetConnectors(fd, DRM_MODE_CONNECTOR_HDMIA);

    // All CRTCs are considered for the given mode (valid / not valid)
    uint32_t crtcs = GetCRTCS(fd, true);

    drmModeResPtr resources = drmModeGetResources(fd);

    if(nullptr != resources)
    {
        int32_t connectorIndex = 0;

        while ((found == false) && (connectorIndex < resources->count_connectors))
        {
            if ((connectorMask & (1 << connectorIndex)) != 0)
            {
                drmModeConnectorPtr connectors = drmModeGetConnector(fd, resources->connectors[connectorIndex]);

                if(nullptr != connectors)
                {
                    int32_t encoderIndex = 0;

                    while ((found == false) && (encoderIndex < connectors->count_encoders))
                    {
                        drmModeEncoderPtr encoders = drmModeGetEncoder(fd, connectors->encoders[encoderIndex]);

                        if (nullptr != encoders)
                        {
                            uint32_t  matches = (encoders->possible_crtcs & crtcs);
                            uint32_t* pcrtc   = resources->crtcs;

                            while ((found == false) && (matches > 0))
                            {
                                if ((matches & 1) != 0)
                                {
                                    drmModeCrtcPtr modePtr = drmModeGetCrtc(fd, *pcrtc);

                                    if(nullptr != modePtr)
                                    {
                                        // A viable set found
                                        crtc = *pcrtc;
                                        encoder = encoders->encoder_id;
                                        connector = connectors->connector_id;
                                        fb = modePtr->buffer_id;

                                        drmModeFreeCrtc(modePtr);
                                        found = true;
                                    }
                                }
                                matches >>= 1;
                                pcrtc++;
                            }
                            drmModeFreeEncoder(encoders);
                        }
                        encoderIndex++;
                    }
                    drmModeFreeConnector(connectors);
                }
            }
            connectorIndex++;
        }

        drmModeFreeResources(resources);
    }

    return (found);
}

static bool CreateBuffer(int fd, const uint32_t connector, gbm_device*& device, uint32_t& modeIndex, uint32_t& id, struct gbm_bo*& buffer)
{
    assert(fd != -1);

    bool created = false;
    buffer = nullptr;
    modeIndex = 0;
    id = 0;
    device = gbm_create_device(fd);

    if (nullptr != device)
    {
        drmModeConnectorPtr pconnector = drmModeGetConnector(fd, connector);

        if (nullptr != pconnector)
        {
            bool found = false;
            uint32_t index = 0;
            uint64_t area = 0;

            while ( (found == false) && (index < static_cast<uint32_t>(pconnector->count_modes)) )
            {
                uint32_t type = pconnector->modes[index].type;

                // At least one preferred mode should be set by the driver, but dodgy EDID parsing might not provide it
                if (DRM_MODE_TYPE_PREFERRED == (DRM_MODE_TYPE_PREFERRED & type))
                {
                    modeIndex = index;

                    // Found a suitable mode; break the loop
                    found = true;
                }
                else if (DRM_MODE_TYPE_DRIVER == (DRM_MODE_TYPE_DRIVER & type))
                {
                    // Calculate screen area
                    uint64_t size = pconnector->modes[index].hdisplay * pconnector->modes[index].vdisplay;
                    if (area < size) {
                        area = size;

                        // Use another selection criterium
                        // Select highest clock and vertical refresh rate

                        if ( (pconnector->modes[index].clock > pconnector->modes[modeIndex].clock) ||
                             ( (pconnector->modes[index].clock == pconnector->modes[modeIndex].clock) && 
                               (pconnector->modes[index].vrefresh > pconnector->modes[modeIndex].vrefresh) ) )
                        {
                            modeIndex = index;
                        }
                    }
                }
                index++;
            }

            // A large enough initial buffer for scan out
            struct gbm_bo* bo = gbm_bo_create(
                                  device, 
                                  pconnector->modes[modeIndex].hdisplay,
                                  pconnector->modes[modeIndex].vdisplay,
                                  ModeSet::SupportedBufferType(),
                                  GBM_BO_USE_SCANOUT /* presented on a screen */ | GBM_BO_USE_RENDERING /* used for rendering */);

            drmModeFreeConnector(pconnector);

            if(nullptr != bo)
            {
                // Associate a frame buffer with this bo
                int32_t fb_fd = gbm_device_get_fd(device);

                int32_t ret = drmModeAddFB(
                                fb_fd, 
                                gbm_bo_get_width(bo), 
                                gbm_bo_get_height(bo), 
                                ModeSet::ColorDepth(), ModeSet::BPP(),
                                gbm_bo_get_stride(bo), 
                                gbm_bo_get_handle(bo).u32, &id);

                if(0 == ret)
                {
                    buffer = bo;

                    created = true;
                }
            }
        }
    }

    return created;
}

ModeSet::ModeSet()
    : _crtc(0)
    , _encoder(0)
    , _connector(0)
    , _mode(0)
    , _buffer(nullptr)
{
    if (drmAvailable() > 0) {
        if (Create() == false) {
            // We are NOT initialized properly, destruct !!!
            Destruct();
        }
    }
}

bool ModeSet::Create()
{
    bool enabled = false;

    int fd = FileDescriptor();

    if(fd >= 0) {

        if ( (FindProperDisplay(fd, _crtc, _encoder, _connector, _fb) == false) ||
/* TODO: Changes the original fb which might not be what is intended */
             (CreateBuffer(fd, _connector, _device, _mode, _fb, _buffer) == false) ||
             (drmSetMaster(fd) != 0) ) {
        }
        else {
            drmModeConnectorPtr pconnector = drmModeGetConnector(fd, _connector);

            if(pconnector != nullptr) {
                /* At least one mode has to be set */
                enabled = (0 == drmModeSetCrtc(fd, _crtc, _fb, 0, 0, &_connector, 1, &(pconnector->modes[_mode])));

                drmModeFreeConnector(pconnector);
            }
        }
    }

    return enabled;
}

ModeSet::~ModeSet()
{
    Destruct();

    int fd = FileDescriptor();

    if(fd >= 0) {
        close(fd);
    }
}

void ModeSet::Destruct()
{
    // Destroy the initial buffer if one exists
    if (nullptr != _buffer)
    {
        gbm_bo_destroy(_buffer);
        _buffer = nullptr;
    }

    if (nullptr != _device)
    {
        gbm_device_destroy(_device);
        _device = nullptr;
    }

    _crtc = 0;
    _encoder = 0;
    _connector = 0;
}

uint32_t ModeSet::Width() const
{
    uint32_t width = 0;

    if (nullptr != _buffer)
    {
        width = gbm_bo_get_width(_buffer);
    }

    return width;
}

uint32_t ModeSet::Height() const
{
    uint32_t height = 0;

    if (nullptr != _buffer)
    {
        height = gbm_bo_get_height(_buffer);
    }

    return height;
}

// These created resources are automatically destroyed if gbm_device is destroyed
struct gbm_surface* ModeSet::CreateRenderTarget(const uint32_t width, const uint32_t height)
{
    struct gbm_surface* result = nullptr;

    if(nullptr != _device)
    {
        result = gbm_surface_create(_device, width, height, SupportedBufferType(), GBM_BO_USE_SCANOUT /* presented on a screen */ | GBM_BO_USE_RENDERING /* used for rendering */);
    }

    return result;
}

void ModeSet::DestroyRenderTarget(struct gbm_surface* surface)
{
    if (nullptr != surface)
    {
        gbm_surface_release_buffer(surface, _buffer);

        gbm_surface_destroy(surface);
    }
}

extern "C"
{
void page_flip_handler(int fd, unsigned int frame, unsigned int sec, unsigned int usec, void* data)
{
    bool *waiting_for_flip = reinterpret_cast<bool*>(data);
    *waiting_for_flip = false;
}
}

bool ModeSet::FlipRenderTarget(const gbm_surface* surface)
{
    bool flipped = false;

    // Global scope to understand exit strategy of the waiting loop
    bool waiting_for_flip = true;

    int fd = FileDescriptor();

    if((fd >= 0) && (surface != nullptr)) {

        // Do not forget to release buffers, otherwise memory pressure will increase

        // Buffer object representing our front buffer
        struct gbm_bo* bo = gbm_surface_lock_front_buffer(const_cast<struct gbm_surface*>(surface));

        if(bo != nullptr) {
            // Use the created bo buffer as the internal target for rendering

            uint32_t stride = gbm_bo_get_stride(bo);
            uint32_t height = gbm_bo_get_height(bo);
            uint32_t width = gbm_bo_get_width(bo);
            uint32_t handle = gbm_bo_get_handle(bo).u32;

            int32_t ret = -1; // anything != 0
            uint32_t id = 0;

            drmModeFBPtr fb_ptr = drmModeGetFB(fd, _fb);
            if(nullptr != fb_ptr) {
                // Try to extract bpp and stride from previous frame buffer so they match for the next frame

                // Create an associated frame buffer (for each b0 buffer)
                ret = drmModeAddFB(fd, width /* Should equal fb_ptr->width */, height /* Should equal fb_ptr->height */, fb_ptr->depth, fb_ptr->bpp, stride /* Should equal fb_ptr->pitch */, handle, &id);

                drmModeFreeFB(fb_ptr);
            }

            if(ret == 0) {
                // A new FB and a new bo exist; time to make something visible

                ret = drmModePageFlip(fd, _crtc, id, DRM_MODE_PAGE_FLIP_EVENT, &waiting_for_flip);

                if(ret != 0) {
                    // Error

                    // Many causes, but the most obvious is a busy resource
                    // There is nothing to be done about it; notify the user and just continue
                }
                else {
                    // Use the magic constant here because the struct is versioned!
                    drmEventContext context = { .version = 2, . vblank_handler = nullptr, .page_flip_handler = page_flip_handler };

                    fd_set fds;

                    // Wait up to max 1 second
                    struct timespec timeout = { .tv_sec = 1, .tv_nsec = 0 };

                    while(waiting_for_flip != false) {
                        FD_ZERO(&fds);
                        FD_SET(fd, &fds);

                        // Race free
                        ret = pselect(fd + 1, &fds, nullptr, nullptr, &timeout, nullptr);

                        if(ret < 0) {
                            // Error; break the loop
                            break;
                        }
                        else {
                            if(ret == 0) {
                                // Timeout; retry
// TODO: add another condition to break the loop after several retries
                            }
                            else { // ret > 0
                                if(FD_ISSET(fd, &fds) != 0) {
                                    // Node is readable
                                    if(drmHandleEvent(fd, &context) != 0) {
                                        // Error; break the loop

                                        break;
                                    }

                                    // Flip probably occured already otherwise it loops again
                                }
                            }
                        }
                    }
                }

                if(nullptr != _buffer) {
                    /* void */ gbm_surface_release_buffer(const_cast<struct gbm_surface*>(surface), _buffer);
                }

                // One FB should be released
                /* int */ drmModeRmFB(fd, /* old id */ _fb);

                // Unconditionally; there is nothing to be done if the system fails to remove the (frame) buffer
                _fb = id;
                _buffer = bo;
            }
            else {
                // Release only bo and do not update FB
                /* void */ gbm_surface_release_buffer(const_cast<struct gbm_surface*>(surface), bo);
            }
        }

        // The surface must have at least one buffer left free for rendering otherwise something has not been properly released / removed
        int count = gbm_surface_has_free_buffers(const_cast<struct gbm_surface*>(surface));

        // The value of waiting_for_flip provides a hint on the exit strategy applied
        flipped =  (waiting_for_flip != true) && (count > 0);
    }

    return flipped;
}
