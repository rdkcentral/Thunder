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

#ifdef __cplusplus
extern "C"
{
#endif

#include <EGL/egl.h>

#ifdef __cplusplus
}
#endif

#ifdef VC6
#include "ModeSet.h"

#include <unordered_map>

#else

#ifdef __cplusplus
extern "C"
{
#endif

#include <EGL/eglext.h>
#include <bcm_host.h>

#ifdef __cplusplus
}
#endif

#endif

#include <algorithm>

#include "../../core/core.h"
#include "../../interfaces/IComposition.h"
#include "../../virtualinput/virtualinput.h"
#include "../Client.h"

int g_pipefd[2];

enum inputtype {
    KEYBOARD,
    MOUSE,
    TOUCHSCREEN
};

struct Message {
    inputtype type;
    union {
        struct {
            keyactiontype type;
            uint32_t code;
        } keyData;
        struct {
            mouseactiontype type;
            uint16_t button;
            int16_t horizontal;
            int16_t vertical;
        } mouseData;
        struct {
            touchactiontype type;
            uint16_t index;
            uint16_t x;
            uint16_t y;
        } touchData;
    };
};

static const char* connectorNameVirtualInput = "/tmp/keyhandler";

static void VirtualKeyboardCallback(keyactiontype type, unsigned int code)
{
    if (type != KEY_COMPLETED) {
        Message message;
        message.type = KEYBOARD;
        message.keyData.type = type;
        message.keyData.code = code;
        write(g_pipefd[1], &message, sizeof(message));
    }
}

static void VirtualMouseCallback(mouseactiontype type, unsigned short button, signed short horizontal, signed short vertical)
{
    Message message;
    message.type = MOUSE;
    message.mouseData.type = type;
    message.mouseData.button = button;
    message.mouseData.horizontal = horizontal;
    message.mouseData.vertical = vertical;
    write(g_pipefd[1], &message, sizeof(message));
}

static void VirtualTouchScreenCallback(touchactiontype type, unsigned short index, unsigned short x, unsigned short y)
{
    Message message;
    message.type = TOUCHSCREEN;
    message.touchData.type = type;
    message.touchData.index = index;
    message.touchData.x = x;
    message.touchData.y = y;
    write(g_pipefd[1], &message, sizeof(message));
}

namespace {

#ifdef VC6

using namespace WPEFramework;

#ifdef __cplusplus
extern "C"
{
#endif

EGLSurface eglCreateWindowSurface(EGLDisplay, EGLConfig, EGLNativeWindowType, const EGLint*);
EGLBoolean eglDestroySurface(EGLDisplay, EGLSurface);
EGLBoolean eglChooseConfig(EGLDisplay, const EGLint*, EGLConfig*, EGLint, EGLint*);
EGLBoolean eglSwapBuffers(EGLDisplay, EGLSurface);

#ifdef __cplusplus
}
#endif

class Platform {
private:
    Platform() : _platform()
    {
    }

public:
    Platform(const Platform&) = delete;
    Platform& operator=(const Platform&) = delete;

    static Platform& Instance() {
        static Platform singleton;
        return singleton;
    }
    ~Platform()
    {
    }

public:
    EGLNativeDisplayType Display() const 
    {
        EGLNativeDisplayType result (static_cast<EGLNativeDisplayType>(EGL_DEFAULT_DISPLAY));

        const struct gbm_device* pointer = _platform.UnderlyingHandle();

        if(pointer != nullptr) {
            result = reinterpret_cast<EGLNativeDisplayType>(const_cast<struct gbm_device*>(pointer));
        }
        else {
            TRACE_L1(_T("The native display (id) might be invalid / unsupported. Using the EGL default display instead!"));
        }

        return (result);
    }
    uint32_t Width() const
    {
        return (_platform.Width());
    }
    uint32_t Height() const
    {
        return (_platform.Height());
    }
    EGLSurface CreateSurface (const EGLNativeDisplayType& display, const uint32_t width, const uint32_t height) 
    {
        // A Native surface that acts as a native window
        EGLSurface result = reinterpret_cast<EGLSurface>(_platform.CreateRenderTarget(width, height));

        if (result != 0) {
            TRACE_L1(_T("The native window (handle) might be invalid / unsupported. Expect undefined behavior!"));
        }

        return (result);
    }
    void DestroySurface(const EGLSurface& surface) 
    {
        _platform.DestroyRenderTarget(reinterpret_cast<struct gbm_surface*>(surface));
    }
    void Opacity(const EGLSurface&, const uint8_t) 
    {
        TRACE_L1(_T("Currently not supported"));
    }
    void Geometry (const EGLSurface&, const Exchange::IComposition::Rectangle&) 
    {
        TRACE_L1(_T("Currently not supported"));
    }
    void ZOrder(const EGLSurface&, const int8_t)
    {
        TRACE_L1(_T("Currently not supported"));
    }
    bool ScanOutSurface(const EGLSurface& surface)
    {
        bool ret = false;
        // See the comment in eglSwapBuffers
        _platform.FlipRenderTarget(reinterpret_cast<const struct gbm_surface*>(surface));

        return ret;
    }

private:
    friend EGLBoolean eglSwapBuffers(EGLDisplay, EGLSurface);
    friend EGLSurface eglCreateWindowSurface(EGLDisplay, EGLConfig, EGLNativeWindowType, const EGLint*);
    friend EGLBoolean eglDestroySurface(EGLDisplay, EGLSurface);

    ModeSet _platform;
    std::unordered_map<EGLSurface, EGLNativeWindowType> _map;
};

static const std::string& LibraryName()
{
    static std::string lib_name;

    TRACE_L1(_T("Testing proper/expected/required symbol resolution for EGL API."));

    if(lib_name.empty() == true) {

        // Clear any existing error
        // The returned string may be statically allocated!
        dlerror();

        const std::string& (*_LibraryName)() = nullptr;

        _LibraryName = reinterpret_cast<const std::string& (*)()>(reinterpret_cast<uintptr_t>(dlsym(RTLD_DEFAULT, "LibraryName")));

        // The returned string may be statically allocated!
        /* const */ char* err = dlerror();

        if(err == nullptr) {

            if(_LibraryName != nullptr) {

                Dl_info info;

                if(dladdr(reinterpret_cast<void*>(_LibraryName), &info) != 0) {
                    if(info.dli_fname != nullptr) {
                        lib_name = info.dli_fname;
                    }

                    TRACE_L1(_T("Current library name equals %s"), lib_name.c_str());
                }
            }
        }
    }

    return lib_name;
}

#ifdef __cplusplus
extern "C"
{
#endif

__attribute__((visibility("default"))) EGLBoolean eglChooseConfig(EGLDisplay dpy, const EGLint* attrib_list, EGLConfig* configs, EGLint config_size, EGLint* num_config)
{
    static EGLBoolean (*_eglChooseConfig)(EGLDisplay, const EGLint*, EGLConfig*, EGLint, EGLint*) = nullptr;
    static bool resolved = false;

    if((resolved != true) && (_eglChooseConfig == nullptr)) {

        bool available = false;

        // Clear any existing error
        // The returned string may be statically allocated!
        dlerror();

        _eglChooseConfig = reinterpret_cast<EGLBoolean (*)(EGLDisplay, const EGLint*, EGLConfig*, EGLint, EGLint*)>(reinterpret_cast<uintptr_t>(dlsym(RTLD_NEXT, "eglChooseConfig")));

        /* const */ char* err = dlerror();

        if(err == nullptr) {

            if(_eglChooseConfig != nullptr) {

                Dl_info info;

                if(dladdr(reinterpret_cast<void*>(_eglChooseConfig), &info) !=0) {

                    if(info.dli_fname != nullptr) {
                        TRACE_L1(_T("eglChooseConfig found in %s"), info.dli_fname);

                        const std::string& lib_name = LibraryName();

                        available = (lib_name.compare(info.dli_fname) != 0);
                    }
                }
            }
        }

        if(available != true) {
            TRACE_L1(_T("Unexpected symbol resolution. Render support might have unexpected behavior."));
        }
        else {
            resolved = true;
        }
    }

    EGLBoolean ret = EGL_FALSE;

    if((resolved != false) && (_eglChooseConfig != nullptr)) {
        // Call the true eglChooseConfig with the added attribute

        std::vector<EGLConfig> unfiltered_configs;

        unfiltered_configs.assign(config_size, configs);

        ret = EGL_FALSE;

        if(unfiltered_configs.empty() != true) {
            // No check on parameters!
            ret = _eglChooseConfig(dpy, attrib_list, unfiltered_configs.data(), config_size, num_config);

            if((ret != EGL_FALSE) && (num_config > 0)) {
                unfiltered_configs.resize(*num_config);

                *num_config = 0;

                for(auto it = unfiltered_configs.begin(), end = unfiltered_configs.end(); it != end; it++) {

                    EGLint value;

                    ret = eglGetConfigAttrib(dpy, *it, EGL_NATIVE_VISUAL_ID, &value);

                    if(ret != EGL_FALSE) {
                        // Assert if this type is not in the set
                        static_assert((ModeSet::SupportedBufferType() == DRM_FORMAT_ARGB8888) || (ModeSet::SupportedBufferType() == DRM_FORMAT_XRGB8888));

                        // Both formats should be considered equivalent / interchangeable
                        if((value == DRM_FORMAT_ARGB8888) || (value == DRM_FORMAT_XRGB8888)) {
                            configs[*num_config] = *it;
                            (*num_config)++;
                        }
                    }

                }
            }
        }
    }

    return ret;
}

// Interposition function
 __attribute__((visibility("default"))) EGLBoolean eglSwapBuffers(EGLDisplay dpy, EGLSurface surface)
{
    static EGLBoolean (*_eglSwapBuffers)(EGLDisplay, EGLSurface) = nullptr;
    static bool resolved = false;

    if((resolved != true) && (_eglSwapBuffers == nullptr)) {

        bool available = false;

        // Clear any existing error
        // The returned string may be statically allocated!
        dlerror();

        _eglSwapBuffers = reinterpret_cast<EGLBoolean (*)(EGLDisplay, EGLSurface)>(reinterpret_cast<uintptr_t>(dlsym(RTLD_NEXT, "eglSwapBuffers")));

        /* const */ char* err = dlerror();

        if(err == nullptr) {

            if(_eglSwapBuffers != nullptr) {

                Dl_info info;

                if(0 != dladdr(reinterpret_cast<void*>(_eglSwapBuffers), &info)) {

                    if(info.dli_fname != nullptr) {
                        TRACE_L1(_T("eglSwapBuffers found in %s"), info.dli_fname);

                        const std::string& lib_name = LibraryName();

                        available = (lib_name.compare(info.dli_fname) != 0);
                    }
                }
            }
        }

        if(available != true) {
            TRACE_L1(_T("Unexpected symbol resolution. Render support might have unexpected behavior."));
        }
        else {
            resolved = true;
        }
    }

    EGLBoolean ret = EGL_FALSE;

    if((resolved != false) && (_eglSwapBuffers != nullptr)) {
        // Call the true eglSwapBuffers and do the update for libdrm

        // No check on parameters!
        ret = _eglSwapBuffers(dpy, surface);

        if(ret != EGL_FALSE) {
//           The author of the (original) Platform interface made the assumption that EGLSurface and the EGLNativeWindowType can be interchanged (via casts) and that it does not cause any (safety) issues, despite the implicit (and hidden) platform implementation details.
//           EGLSurface is used throughout the member declarations. Hence the newly added ScanOutSurface() does not break with this 'tradition'.
//
//              Most of the details can be found in eglplatform.h
//
//              For GBM
//                  typedef struct gbm_device* EGLNativeDisplay
//                  typedef void* EGLNativeWindowType
//
//              But they might well be
//
//                  tyepdef int EGLNativeDisplay
//                  typedef int EGLNativeWindowType
//
//              Whereas egl.h shows
//                  typedef void* EGLSurface
//
//           For the VC6 platform the aforementioned basically reduces to the question: do *(struct gbm_surface*) and EGLSurface represent the same data given that different API is used to create related objects?
//
            // A hint if the map is really required
            static_assert(!std::is_same<EGLSurface, EGLNativeWindowType>::value);
            std::unordered_map<EGLSurface, EGLNativeWindowType>::iterator it = Platform::Instance()._map.find(surface);

            if(it != Platform::Instance()._map.end()) {
                TRACE_L1(_T("Successfully found association."));

                // True EGLSurface
//                ret = static_cast<EGLBoolean>(Platform::Instance().ScanOutSurface(it->first));

                // EGLNativeWindowType casted to EGLSurface
            static_assert((static_cast<EGLBoolean>(true) == EGL_TRUE) && (static_cast<EGLBoolean>(false) == EGL_FALSE));
                ret = static_cast<EGLBoolean>(Platform::Instance().ScanOutSurface(reinterpret_cast<struct gbm_surface*>(it->second)));
            }
            else {
                TRACE_L1(_T("Unable to find association. Rendering might have unexpected behavior."));
            }
        }
    }

    return ret;
}

// Interposition function
// Tracking the single association between a native window and EGLSurface
__attribute__((visibility("default"))) EGLSurface eglCreateWindowSurface(EGLDisplay dpy, EGLConfig config, EGLNativeWindowType win, const EGLint* attrib_list)
{
    static EGLSurface (*_eglCreateWindowSurface)(EGLDisplay, EGLConfig, EGLNativeWindowType, const EGLint*) = nullptr;
    static bool resolved = false;

    if((resolved != true) && (_eglCreateWindowSurface == nullptr)) {

        bool available = false;

        // Clear any existing error
        // The returned string may be statically allocated!
        dlerror();

        _eglCreateWindowSurface = reinterpret_cast<EGLSurface (*)(EGLDisplay, EGLConfig, EGLNativeWindowType, const EGLint*)>(reinterpret_cast<uintptr_t>(dlsym(RTLD_NEXT, "eglCreateWindowSurface")));

        /* const */ char* err = dlerror();

        if(err == nullptr) {

            if(_eglCreateWindowSurface != nullptr) {

                Dl_info info;

                if(dladdr(reinterpret_cast<void*>(_eglCreateWindowSurface), &info) !=0){

                    if(info.dli_fname != nullptr) {
                        TRACE_L1(_T("eglCreateWindowSurface found in %s"), info.dli_fname);

                        const std::string& lib_name = LibraryName();

                        available = (lib_name.compare(info.dli_fname) != 0);
                    }
                }
            }
        }

        if(available != true) {
            TRACE_L1(_T("Unexpected symbol resolution. Render support might have unexpected behavior."));
        }
        else {
            resolved = true;
        }
    }

    EGLSurface ret = EGL_NO_SURFACE;

    if((resolved != false) && (_eglCreateWindowSurface != nullptr)) {
        // Call the true eglCreateWindowSurface

        // No check on parameters!
        ret = _eglCreateWindowSurface(dpy, config, win, attrib_list);
        if(ret != EGL_NO_SURFACE) {
            // create a map between EGLSurface and win, aka native window (struct gbm_surface*)
            std::pair<std::unordered_map<EGLSurface, EGLNativeWindowType>::iterator, bool> result = Platform::Instance()._map.insert(std::pair<EGLSurface, EGLNativeWindowType>(ret, win));

            if(result.second != true) {
                TRACE_L1(_T("Unable to associate EGLSurface and EGLNativeWindowType objects. Rendering may have unexpected side effects."));
            }
            else {
                TRACE_L1(_T("Successfully added association."));
            }
        }
    }

    return ret;
}

// Interposition function
// Remove the tracked single association between a native window and EGLSurface
__attribute__((visibility("default"))) EGLBoolean eglDestroySurface(EGLDisplay dpy, EGLSurface surface)
{
    static EGLBoolean (*_eglDestroySurface)(EGLDisplay, EGLSurface) = nullptr;
    static bool resolved = false;

    if((resolved != true) && (_eglDestroySurface == nullptr)) {

        bool available = false;

        // Clear any existing error
        // The returned string may be statically allocated!
        dlerror();

        _eglDestroySurface = reinterpret_cast<EGLBoolean (*)(EGLDisplay, EGLSurface)>(reinterpret_cast<uintptr_t>(dlsym(RTLD_NEXT, "eglDestroySurface")));

        /* const */ char* err = dlerror();

        if(err == nullptr) {

            if(_eglDestroySurface != nullptr) {

                Dl_info info;

                if(0 != dladdr(reinterpret_cast<void*>(_eglDestroySurface), &info)) {

                    if(info.dli_fname != nullptr) {
                        TRACE_L1(_T("eglDestroySurface found in %s"), info.dli_fname);

                        const std::string& lib_name = LibraryName();

                        available = (lib_name.compare(info.dli_fname) != 0);
                    }
                }
            }
        }

        if(available != true) {
            TRACE_L1(_T("Unexpected symbol resolution. Render support might have unexpected behavior."));
        }
        else {
            resolved = true;
        }
    }

    EGLBoolean ret = EGL_FALSE;

    if((resolved != false) && (_eglDestroySurface != nullptr)) {
        // Call the true eglDestroySurface

        // No check on parameters!
        ret = _eglDestroySurface(dpy, surface);

        if(ret != EGL_FALSE) {
            // create a map between EGLSurface and win, aka native window (struct gbm_surface*)
            size_t result = Platform::Instance()._map.erase(surface);

            if(result == 0) {
                // Unable to insert
                TRACE_L1(_T("Unable to remove association between EGLSurface and EGLNativeWindowType objects. Rendering may have unexpected side effects."));
                // Call eglDestroySurface and return EGL_FALSE?
            }
            else {
                TRACE_L1(_T("Successfully removed association."));
            }
        }
    }

    return ret;
}

#ifdef __cplusplus
}
#endif

#else

class Platform {
private:
    struct Surface {
        EGL_DISPMANX_WINDOW_T surface;
        VC_RECT_T rectangle;
        int8_t layer;
        uint8_t opacity;
    };

    Platform()
    {
        bcm_host_init();
    }

public:
    Platform(const Platform&) = delete;
    Platform& operator=(const Platform&) = delete;

    static Platform& Instance() {
        static Platform singleton;
        return singleton;
    }
    ~Platform()
    {
        bcm_host_deinit();
    }

public:
    EGLNativeDisplayType Display() const 
    {
        EGLNativeDisplayType result (static_cast<EGLNativeDisplayType>(EGL_DEFAULT_DISPLAY));

        return (result);
    }
    uint32_t Width() const
    {
        uint32_t width, height;
        graphics_get_display_size(0, &width, &height);
        return (width);
    }
    uint32_t Height() const
    {
        uint32_t width, height;
        graphics_get_display_size(0, &width, &height);
        return (height);
    }
    EGLSurface CreateSurface (const EGLNativeWindowType& display, const uint32_t width, const uint32_t height) 
    {
        EGLSurface result;

        uint32_t displayWidth  = Width();
        uint32_t displayHeight = Height();
        Surface* surface = new Surface;

        VC_RECT_T srcRect;

        vc_dispmanx_rect_set(&(surface->rectangle), 0, 0, displayWidth, displayHeight);
        vc_dispmanx_rect_set(&srcRect, 0, 0, displayWidth << 16, displayHeight << 16);
        surface->layer = 0;
        surface->opacity = 255;

        VC_DISPMANX_ALPHA_T alpha = {
            static_cast<DISPMANX_FLAGS_ALPHA_T>(DISPMANX_FLAGS_ALPHA_FROM_SOURCE | DISPMANX_FLAGS_ALPHA_MIX),
            surface->opacity,
            255
        };

        DISPMANX_DISPLAY_HANDLE_T dispmanDisplay = vc_dispmanx_display_open(0);
        DISPMANX_UPDATE_HANDLE_T  dispmanUpdate  = vc_dispmanx_update_start(0);
        DISPMANX_ELEMENT_HANDLE_T dispmanElement = vc_dispmanx_element_add(
            dispmanUpdate,
            dispmanDisplay,
            surface->layer, /* Z order layer, new one is always on top */
            &(surface->rectangle),
            0 /*src*/,
            &srcRect,
            DISPMANX_PROTECTION_NONE,
            &alpha /*alpha*/,
            0 /*clamp*/,
            DISPMANX_NO_ROTATE);
        vc_dispmanx_update_submit_sync(dispmanUpdate);

        surface->surface.element = dispmanElement;
        surface->surface.width   = displayWidth;
        surface->surface.height  = displayHeight;
        result                   = static_cast<EGLSurface>(surface);

        return (result);
    }

    void DestroySurface(const EGLSurface& surface) 
    {
        Surface* object = reinterpret_cast<Surface*>(surface);
        // DISPMANX_DISPLAY_HANDLE_T dispmanDisplay = vc_dispmanx_display_open(0);
        DISPMANX_UPDATE_HANDLE_T  dispmanUpdate  = vc_dispmanx_update_start(0);
        vc_dispmanx_element_remove(dispmanUpdate, object->surface.element);
        vc_dispmanx_update_submit_sync(dispmanUpdate);
        // vc_dispmanx_display_close(dispmanDisplay);
        delete object;
    }
    
    void Opacity(const EGLSurface& surface, const uint16_t opacity) 
    {
        VC_RECT_T srcRect;
        Surface* object = reinterpret_cast<Surface*>(surface);

        vc_dispmanx_rect_set(&srcRect, 0, 0, (Width() << 16), (Height() << 16));
        object->opacity = opacity;

        DISPMANX_UPDATE_HANDLE_T  dispmanUpdate = vc_dispmanx_update_start(0);
        vc_dispmanx_element_change_attributes(dispmanUpdate,
            object->surface.element,
            (1 << 1),
            object->layer,
            object->opacity,
            &object->rectangle,
            &srcRect,
            0,
            DISPMANX_NO_ROTATE);
        vc_dispmanx_update_submit_sync(dispmanUpdate);
    }

    void Geometry (const EGLSurface& surface, const WPEFramework::Exchange::IComposition::Rectangle& rectangle) 
    {
        VC_RECT_T srcRect;
        Surface* object = reinterpret_cast<Surface*>(surface);

        vc_dispmanx_rect_set(&srcRect, 0, 0, (Width() << 16), (Height() << 16));
        vc_dispmanx_rect_set(&(object->rectangle), rectangle.x, rectangle.y, rectangle.width, rectangle.height);

        DISPMANX_UPDATE_HANDLE_T  dispmanUpdate = vc_dispmanx_update_start(0);
        vc_dispmanx_element_change_attributes(dispmanUpdate,
            object->surface.element,
            (1 << 2),
            object->layer,
            object->opacity,
            &object->rectangle,
            &srcRect,
            0,
            DISPMANX_NO_ROTATE);
        vc_dispmanx_update_submit_sync(dispmanUpdate);
    }

    void ZOrder(const EGLSurface& surface, const int8_t layer)
    {
        // RPI is unique: layer #0 actually means "deepest", so we need to convert.
        const int8_t actualLayer = 127 - layer;
        Surface* object = reinterpret_cast<Surface*>(surface);
        DISPMANX_UPDATE_HANDLE_T  dispmanUpdate = vc_dispmanx_update_start(0);
        object->layer = layer;
        vc_dispmanx_element_change_layer(dispmanUpdate, object->surface.element, actualLayer);
        vc_dispmanx_update_submit_sync(dispmanUpdate);
    }
};

#endif

}

namespace WPEFramework {
namespace RPI {

static Core::NodeId Connector()
{
    string connector;
    if ((Core::SystemInfo::GetEnvironment(_T("COMPOSITOR"), connector) == false) || (connector.empty() == true)) {
        connector = _T("/tmp/compositor");
    }
    return (Core::NodeId(connector.c_str()));
}

class Display : public Compositor::IDisplay {
private:
    Display() = delete;
    Display(const Display&) = delete;
    Display& operator=(const Display&) = delete;

    class EXTERNAL CompositorClient {
    private:
        // -------------------------------------------------------------------
        // This object should not be copied or assigned. Prevent the copy
        // constructor and assignment constructor from being used. Compiler
        // generated assignment and copy methods will be blocked by the
        // following statments.
        // Define them but do not implement them, compile error/link error.
        // -------------------------------------------------------------------
        CompositorClient(const CompositorClient& a_Copy) = delete;
        CompositorClient& operator=(const CompositorClient& a_RHS) = delete;

    public:
        CompositorClient(const TCHAR formatter[], ...)
        {
            va_list ap;
            va_start(ap, formatter);
            Trace::Format(_text, formatter, ap);
            va_end(ap);
        }
        CompositorClient(const string& text)
            : _text(Core::ToString(text))
        {
        }
        ~CompositorClient() = default;

    public:
        inline const char* Data() const
        {
            return (_text.c_str());
        }
        inline uint16_t Length() const
        {
            return (static_cast<uint16_t>(_text.length()));
        }

    private:
        string _text;
    };

    class SurfaceImplementation : public Exchange::IComposition::IClient, public Compositor::IDisplay::ISurface {
    public:
        SurfaceImplementation() = delete;
        SurfaceImplementation(const SurfaceImplementation&) = delete;
        SurfaceImplementation& operator=(const SurfaceImplementation&) = delete;

        SurfaceImplementation(
            Display* compositor, const std::string& name,
            const uint32_t width, const uint32_t height);
        virtual ~SurfaceImplementation();

        using Exchange::IComposition::IClient::AddRef;

        string Name() const override
        {
            return _name;
        }
        void Kill() override
        {
            //todo: implement
            TRACE_L1(_T("Kill called for Client %s. Not supported."), Name().c_str());
        }
        void Opacity(const uint32_t value) override;
        uint32_t Geometry(const Exchange::IComposition::Rectangle& rectangle) override;
        Exchange::IComposition::Rectangle Geometry() const override;
        uint32_t ZOrder(const uint16_t zorder) override;

        inline EGLNativeWindowType Native() const
        {
            return (reinterpret_cast<EGLNativeWindowType>(_nativeSurface));
        }
        inline int32_t Width() const
        {
            return _width;
        }
        inline int32_t Height() const
        {
            return _height;
        }
        inline void Keyboard(Compositor::IDisplay::IKeyboard* keyboard) override
        {
            assert((_keyboard == nullptr) ^ (keyboard == nullptr));
            _keyboard = keyboard;
        }
        inline void Wheel(Compositor::IDisplay::IWheel* wheel) override
        {
            assert((_wheel == nullptr) ^ (wheel == nullptr));
            _wheel = wheel;
        }
        inline void Pointer(Compositor::IDisplay::IPointer* pointer) override
        {
            assert((_pointer == nullptr) ^ (pointer == nullptr));
            _pointer = pointer;
        }
        inline void TouchPanel(Compositor::IDisplay::ITouchPanel* touchpanel) override
        {
            assert((_touchpanel == nullptr) ^ (touchpanel == nullptr));
            _touchpanel = touchpanel;
        }
        inline void SendKey(
            const uint32_t key,
            const IKeyboard::state action, const uint32_t time)
        {
            if (_keyboard != nullptr) {
                _keyboard->Direct(key, action);
            }
        }
        inline void SendWheelMotion(const int16_t x, const int16_t y, const uint32_t time)
        {
            if (_wheel != nullptr) {
                _wheel->Direct(x, y);
            }
        }
        inline void SendPointerButton(const uint8_t button, const IPointer::state state, const uint32_t time)
        {
            if (_pointer != nullptr) {
                _pointer->Direct(button, state);
            }
        }
        inline void SendPointerPosition(const int16_t x, const int16_t y, const uint32_t time)
        {
            if (_pointer != nullptr) {
                _pointer->Direct(x, y);
            }
        }
        inline void SendTouch(const uint8_t index, const ITouchPanel::state state, const uint16_t x, const uint16_t y, const uint32_t time)
        {
            if (_touchpanel != nullptr) {
                _touchpanel->Direct(index, state, x, y);
            }
        }

    private:
        BEGIN_INTERFACE_MAP(Entry)
        INTERFACE_ENTRY(Exchange::IComposition::IClient)
        END_INTERFACE_MAP

    private:
        Display& _display;
        const std::string _name;
        uint32_t _width;
        uint32_t _height;
        uint32_t _opacity;
        uint32_t _layer;

        EGLSurface _nativeSurface;

        IKeyboard* _keyboard;
        IWheel* _wheel;
        IPointer* _pointer;
        ITouchPanel* _touchpanel;

        Exchange::IComposition::Rectangle _destination;
    };

public:
    Display(const std::string& displayName);
    virtual ~Display();

    virtual void AddRef() const
    {
        if (Core::InterlockedIncrement(_refCount) == 1) {
            const_cast<Display*>(this)->Initialize();
        }
        return;
    }

    virtual uint32_t Release() const
    {
        if (Core::InterlockedDecrement(_refCount) == 0) {
            const_cast<Display*>(this)->Deinitialize();

            return (Core::ERROR_CONNECTION_CLOSED);
        }
        return (Core::ERROR_NONE);
    }
    virtual EGLNativeDisplayType Native() const override
    {
        return (Platform::Instance().Display());
    }
    virtual const std::string& Name() const final
    {
        return (_displayName);
    }
    virtual int Process(const uint32_t data) override;
    virtual int FileDescriptor() const override;
    virtual ISurface* Create(
        const std::string& name,
        const uint32_t width, const uint32_t height) override;

    inline uint32_t DisplaySizeWidth() const
    {
        return Platform::Instance().Width();
    }

    inline uint32_t DisplaySizeHeight() const
    {
        return Platform::Instance().Height();
    }

private:
    inline void Register(SurfaceImplementation* surface);
    inline void Unregister(SurfaceImplementation* surface);
    inline void OfferClientInterface(Exchange::IComposition::IClient* client);
    inline void RevokeClientInterface(Exchange::IComposition::IClient* client);

    inline void Initialize()
    {
        _adminLock.Lock();
        _isRunning = true;

        if (Core::WorkerPool::IsAvailable() == true) {
            // If we are in the same process space as where a WorkerPool is registered (Main Process or
            // hosting ptocess) use, it!
            Core::ProxyType<RPC::InvokeServer> engine = Core::ProxyType<RPC::InvokeServer>::Create(&Core::WorkerPool::Instance());
            ASSERT(static_cast<Core::IReferenceCounted*>(engine) != nullptr);

            _compositerServerRPCConnection = Core::ProxyType<RPC::CommunicatorClient>::Create(Connector(), Core::ProxyType<Core::IIPCServer>(engine));
            ASSERT(_compositerServerRPCConnection != nullptr);

            engine->Announcements(_compositerServerRPCConnection->Announcement());
        } else {
            // Seems we are not in a process space initiated from the Main framework process or its hosting process.
            // Nothing more to do than to create a workerpool for RPC our selves !
            Core::ProxyType<RPC::InvokeServerType<2,0,8>> engine = Core::ProxyType<RPC::InvokeServerType<2,0,8>>::Create();
            ASSERT(engine != nullptr);

            _compositerServerRPCConnection = Core::ProxyType<RPC::CommunicatorClient>::Create(Connector(), Core::ProxyType<Core::IIPCServer>(engine));
            ASSERT(_compositerServerRPCConnection != nullptr);

            engine->Announcements(_compositerServerRPCConnection->Announcement());
        }

        uint32_t result = _compositerServerRPCConnection->Open(RPC::CommunicationTimeOut);

        if (result != Core::ERROR_NONE) {
            TRACE_L1(_T("Could not open connection to Compositor with node %s. Error: %s"), _compositerServerRPCConnection->Source().RemoteId(), Core::NumberType<uint32_t>(result).Text());
            _compositerServerRPCConnection.Release();
        }

        _virtualinput = virtualinput_open(_displayName.c_str(), connectorNameVirtualInput, VirtualKeyboardCallback, VirtualMouseCallback, VirtualTouchScreenCallback);

        if (_virtualinput == nullptr) {
            TRACE_L1(_T("Initialization of virtual input failed for Display %s!"), Name());
        }

        if (pipe(g_pipefd) == -1) {
            g_pipefd[0] = -1;
            g_pipefd[1] = -1;
        }
        _adminLock.Unlock();
    }

    inline void Deinitialize()
    {
        _adminLock.Lock();
        _isRunning = false;

        close(g_pipefd[0]);
        Message message;
        memset(&message, 0, sizeof(message));
        write(g_pipefd[1], &message, sizeof(message));
        close(g_pipefd[1]);

        if (_virtualinput != nullptr) {
            virtualinput_close(_virtualinput);
        }

        std::list<SurfaceImplementation*>::iterator index(_surfaces.begin());
        while (index != _surfaces.end()) {
            string name = (*index)->Name();

            if (static_cast<Core::IUnknown*>(*index)->Release() != Core::ERROR_DESTRUCTION_SUCCEEDED) { //note, need cast to prevent ambigious call
                TRACE_L1(_T("Compositor Surface [%s] is not properly destructed"), name.c_str());
            }

            index = _surfaces.erase(index);
        }
        if (_compositerServerRPCConnection.IsValid() == true) {
            _compositerServerRPCConnection.Release();
        }

        _adminLock.Unlock();
    }

    bool _isRunning;
    std::string _displayName;
    mutable Core::CriticalSection _adminLock;
    void* _virtualinput;
    std::list<SurfaceImplementation*> _surfaces;
    Core::ProxyType<RPC::CommunicatorClient> _compositerServerRPCConnection;
    uint16_t _pointer_x;
    uint16_t _pointer_y;
    uint16_t _touch_x;
    uint16_t _touch_y;
    uint16_t _touch_state;

    mutable uint32_t _refCount;
};

Display::SurfaceImplementation::SurfaceImplementation(
    Display* display,
    const std::string& name,
    const uint32_t width, const uint32_t height)
    : Exchange::IComposition::IClient()
    , _display(*display)
    , _name(name)
    , _width(width)
    , _height(height)
    , _opacity(255)
    , _layer(0)
    , _keyboard(nullptr)
    , _wheel(nullptr)
    , _pointer(nullptr)
    , _touchpanel(nullptr)
    , _destination({ 0, 0, width, height})
{
    // To support scanout the underlying buffer should be large enough to support the selected mode
    // A buffer of smaller dimensions than the display will fail. A larger one is possible but will
    // probably fail in the current setup. Currently, it is best to give both equal dimensions

    if ((width != _display.DisplaySizeWidth()) || (height != _display.DisplaySizeHeight())) {
        TRACE_L1(_T("Requested surface dimensions [%d, %d] might not be honered. Rendering might fail!"), width, height);

        // Truncating
        if (_width  != _display.DisplaySizeWidth())  { _width  = _display.DisplaySizeWidth();  }
        if (_height != _display.DisplaySizeHeight()) { _height = _display.DisplaySizeHeight(); }
    }

    _nativeSurface = Platform::Instance().CreateSurface(_display.Native(), _width, _height);

    _display.Register(this);
}

Display::SurfaceImplementation::~SurfaceImplementation()
{
    TRACE_L1(_T("Destructing client named: %s"), _name.c_str());

    _display.Unregister(this);

    Platform::Instance().DestroySurface(_nativeSurface);
}

void Display::SurfaceImplementation::Opacity(
    const uint32_t value)
{

    _opacity = (value > Exchange::IComposition::maxOpacity) ? Exchange::IComposition::maxOpacity : value;

    Platform::Instance().Opacity(_nativeSurface, _opacity);
}


uint32_t Display::SurfaceImplementation::Geometry(const Exchange::IComposition::Rectangle& rectangle)
{
    _destination = rectangle;

    Platform::Instance().Geometry(_nativeSurface, _destination);
    return (Core::ERROR_NONE);
}

Exchange::IComposition::Rectangle Display::SurfaceImplementation::Geometry() const 
{
    return (_destination);
}

uint32_t Display::SurfaceImplementation::ZOrder(const uint16_t zorder)
{
    int8_t layer = 0;

    if (zorder == static_cast<uint16_t>(~0)) {
        layer = -1;
    } else {
        layer = static_cast<uint8_t>(zorder);
        _layer = layer;
    }

    Platform::Instance().ZOrder(_nativeSurface, layer);

    return (Core::ERROR_NONE);
}

Display::Display(const string& name)
    : _isRunning(true)
    , _displayName(name)
    , _adminLock()
    , _virtualinput(nullptr)
    , _compositerServerRPCConnection()
    , _pointer_x(0)
    , _pointer_y(0)
    , _touch_x(-1)
    , _touch_y(-1)
    , _touch_state(0)
    , _refCount(0)
{
}

Display::~Display()
{
}

int Display::Process(const uint32_t data)
{
    Message message;
    if ((data != 0) && (g_pipefd[0] != -1) && (read(g_pipefd[0], &message, sizeof(message)) > 0)) {
        _adminLock.Lock();

        time_t timestamp = time(nullptr);
        std::function<void(SurfaceImplementation*)> action = nullptr;
        if (message.type == KEYBOARD) {
            const IDisplay::IKeyboard::state state = ((message.keyData.type == KEY_RELEASED)? IDisplay::IKeyboard::released : ((message.keyData.type == KEY_REPEAT)? IDisplay::IKeyboard::repeated : IDisplay::IKeyboard::pressed));
            action = [=](SurfaceImplementation* s) { s->SendKey(message.keyData.code, state, timestamp); };
        } else if (message.type == MOUSE) {
            // Clamp movement to display size
            // TODO: Handle surfaces that are not full screen
            _pointer_x = std::max(0, std::min(static_cast<int32_t>(_pointer_x) + message.mouseData.horizontal, static_cast<int32_t>(DisplaySizeWidth() - 1)));
            _pointer_y = std::max(0, std::min(static_cast<int32_t>(_pointer_y) + message.mouseData.vertical, static_cast<int32_t>(DisplaySizeHeight() - 1)));
            switch (message.mouseData.type)
            {
            case MOUSE_MOTION:
                action = [=](SurfaceImplementation* s) { s->SendPointerPosition(_pointer_x, _pointer_y, timestamp); };
                break;
            case MOUSE_SCROLL:
                action = [=](SurfaceImplementation* s) { s->SendWheelMotion(message.mouseData.horizontal, message.mouseData.vertical, timestamp); };
                break;
            case MOUSE_RELEASED:
            case MOUSE_PRESSED:
                action = [=](SurfaceImplementation* s) { s->SendPointerButton(message.mouseData.button, message.mouseData.type == MOUSE_RELEASED? IDisplay::IPointer::released : IDisplay::IPointer::pressed , timestamp); };
                break;
            }
        } else if (message.type == TOUCHSCREEN) {
            // Get touch position in pixels
            // TODO: Handle surfaces that are not full screen
            const uint16_t x = (DisplaySizeWidth() * (message.touchData.x)) >> 16;
            const uint16_t y = (DisplaySizeHeight() * (message.touchData.y)) >> 16;
            const IDisplay::ITouchPanel::state state = ((message.touchData.type == TOUCH_RELEASED)? ITouchPanel::released : ((message.touchData.type == TOUCH_PRESSED)? ITouchPanel::pressed : ITouchPanel::motion));
            // Reduce IPC traffic. The physical touch coordinates might be different, but when scaled to screen position, they may be same as previous.
            if ((x != _touch_x) || (y != _touch_y) || (state != _touch_state)) {
                action = [=](SurfaceImplementation* s) { s->SendTouch(message.touchData.index, state, x, y, timestamp); };
                _touch_state = state;
                _touch_x = x;
                _touch_y = y;
            }
        }

        if ((action != nullptr) && (_isRunning == true)) {
            std::for_each(begin(_surfaces), end(_surfaces), action);
        }

        _adminLock.Unlock();
    }
    return (0);
}

int Display::FileDescriptor() const
{
    return (g_pipefd[0]);
}

Compositor::IDisplay::ISurface* Display::Create(
    const std::string& name, const uint32_t width, const uint32_t height)
{
    SurfaceImplementation* retval = (Core::Service<SurfaceImplementation>::Create<SurfaceImplementation>(this, name, width, height));

    OfferClientInterface(retval);

    return retval;
}

inline void Display::Register(Display::SurfaceImplementation* surface)
{
    ASSERT(surface != nullptr);

    _adminLock.Lock();

    std::list<SurfaceImplementation*>::iterator index(
        std::find(_surfaces.begin(), _surfaces.end(), surface));
    if (index == _surfaces.end()) {
        _surfaces.push_back(surface);
    }

    _adminLock.Unlock();
}

inline void Display::Unregister(Display::SurfaceImplementation* surface)
{
    ASSERT(surface != nullptr);

    _adminLock.Lock();

    auto index(std::find(_surfaces.begin(), _surfaces.end(), surface));
    ASSERT(index != _surfaces.end());
    if (index != _surfaces.end()) {
        _surfaces.erase(index);
    }
    _adminLock.Unlock();

    RevokeClientInterface(surface);
}

void Display::OfferClientInterface(Exchange::IComposition::IClient* client)
{
    ASSERT(client != nullptr);

    if (_compositerServerRPCConnection.IsValid()) {
        _adminLock.Lock();
        uint32_t result = _compositerServerRPCConnection->Offer(client);
        _adminLock.Unlock();

        if (result != Core::ERROR_NONE) {
            TRACE_L1(_T("Could not offer IClient interface with callsign %s to Compositor. Error: %s"), client->Name(), Core::NumberType<uint32_t>(result).Text());
        }
    } else {
#if defined(COMPOSITORSERVERPLUGIN)
        SYSLOG(Trace::Fatal, (_T("The CompositorServer plugin is included in the build, but not able to reach!")));
        ASSERT(false && "The CompositorServer plugin is included in the build, but not able to reach!");
#endif
    }
}

void Display::RevokeClientInterface(Exchange::IComposition::IClient* client)
{
    ASSERT(client != nullptr);

    if (_compositerServerRPCConnection.IsValid()) {
        _adminLock.Lock();
        uint32_t result = _compositerServerRPCConnection->Revoke(client);
        _adminLock.Unlock();

        if (result != Core::ERROR_NONE) {
            TRACE_L1(_T("Could not revoke IClient interface with callsign %s to Compositor. Error: %s"), client->Name(), Core::NumberType<uint32_t>(result).Text());
        }
    }else {
#if defined(COMPOSITORSERVERPLUGIN)
        SYSLOG(Trace::Fatal, (_T("The CompositorServer plugin is included in the build, but not able to reach!")));
        ASSERT(false && "The CompositorServer plugin is included in the build, but not able to reach!");
#endif
    }
}

} // RPI

Compositor::IDisplay* Compositor::IDisplay::Instance(const string& displayName)
{
    static RPI::Display& myDisplay = Core::SingletonType<RPI::Display>::Instance(displayName);
    myDisplay.AddRef();

    return (&myDisplay);
}
} // WPEFramework
