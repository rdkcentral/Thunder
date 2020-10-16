#pragma once

#include <stdint.h>

class ModeSet
{
    public:
        ModeSet(const ModeSet&) = delete;
        ModeSet& operator= (const ModeSet&) = delete;

        ModeSet();
        ~ModeSet();

    public:
        const struct gbm_device* UnderlyingHandle() const
        {
            return _device;
        }
        uint32_t Width() const;
        uint32_t Height() const;
        struct gbm_surface* CreateRenderTarget(const uint32_t width, const uint32_t height);
        void DestroyRenderTarget(struct gbm_surface* surface);

    private:
        int  Open();
        void Destruct();

    private :
        uint32_t _crtc;
        uint32_t _encoder;
        uint32_t _connector;

        uint32_t _fb;
        uint32_t _mode;

        struct gbm_device* _device;
        struct gbm_bo* _buffer;
};
