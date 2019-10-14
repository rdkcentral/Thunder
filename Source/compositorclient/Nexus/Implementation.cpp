#include <algorithm>
#include <cassert>
#include <list>
#include <string>
#include <string.h>

#define EGL_EGLEXT_PROTOTYPES 1

#include "../Client.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/eglext.h>

#include <nexus_config.h>
#include <nexus_core_utils.h>
#include <nexus_display.h>
#include <nexus_platform.h>
#include <default_nexus.h>

#define BACKEND_BCM_NEXUS_NXCLIENT 1
#ifdef BACKEND_BCM_NEXUS_NXCLIENT
#include <nxclient.h>
#endif

#include <virtualinput/virtualinput.h>

// pipe to relay the keys to the display...
int g_pipefd[2];

struct Message {
    uint32_t code;
    keyactiontype type;
};

static const char* connectorName = "/tmp/keyhandler";

static void VirtualKeyboardCallback(keyactiontype type, unsigned int code)
{
    if (type != KEY_COMPLETED) {
        Message message;
        message.code = code;
        message.type = type;
        write(g_pipefd[1], &message, sizeof(message));
    }
}

namespace WPEFramework {
namespace Nexus {

    class Display : public Compositor::IDisplay {
    private:
        Display() = delete;
        Display(const Display&) = delete;
        Display& operator=(const Display&) = delete;

        class SurfaceImplementation : public Compositor::IDisplay::ISurface {
        private:
            SurfaceImplementation() = delete;
            SurfaceImplementation(const SurfaceImplementation&) = delete;
            SurfaceImplementation& operator=(const SurfaceImplementation&) = delete;

        public:
            SurfaceImplementation(Display& display, const std::string& name, const uint32_t width, const uint32_t height)
                : _parent(display)
                , _refcount(1)
                , _name(name)
                , _width(width)
                , _height(height)
                , _nativeWindow(nullptr)
                , _keyboard(nullptr)
            {

                NXPL_NativeWindowInfoEXT windowInfo;
                NXPL_GetDefaultNativeWindowInfoEXT(&windowInfo);
                windowInfo.x = 0;
                windowInfo.y = 0;
                windowInfo.width = _width;
                windowInfo.height = _height;
                windowInfo.stretch = true;
#ifdef BACKEND_BCM_NEXUS_NXCLIENT
                windowInfo.zOrder = 0;
#endif
                windowInfo.clientID = display.NexusClientId();
                _nativeWindow = NXPL_CreateNativeWindowEXT(&windowInfo);

               _parent.Register(this);
            }

            virtual ~SurfaceImplementation()
            {
                NEXUS_SurfaceClient_Release(reinterpret_cast<NEXUS_SurfaceClient*>(_nativeWindow));
                _parent.Unregister(this);
            }

        public:
            virtual void AddRef() const override
            {
                _refcount++;
            }
            virtual uint32_t Release() const override
            {
                if (--_refcount == 0) {
                    delete const_cast<SurfaceImplementation*>(this);
                }
                return (0);
            }
            virtual EGLNativeWindowType Native() const override
            {
                return (static_cast<EGLNativeWindowType>(_nativeWindow));
            }
            virtual std::string Name() const override
            {
                return _name;
            }
            virtual int32_t Height() const override
            {
                return (_height);
            }
            virtual int32_t Width() const override
            {
                return (_width);
            }
            virtual void Keyboard(Compositor::IDisplay::IKeyboard* keyboard) override
            {
                assert((_keyboard == nullptr) ^ (keyboard == nullptr));
                _keyboard = keyboard;
            }
            inline void SendKey(const uint32_t key, const IKeyboard::state action, const uint32_t time)
            {

                if (_keyboard != nullptr) {
                    _keyboard->Direct(key, action);
                }
            }

        private:
            Display& _parent;
            mutable uint32_t _refcount;
            std::string _name;
            int32_t _width;
            int32_t _height;
            EGLSurface _nativeWindow;
            IKeyboard* _keyboard;
        };

    private:
        Display(const std::string& name)
            : _displayName(name)
            , _nxplHandle(nullptr)
            , _virtualkeyboard(nullptr)
            , _nexusClientId(0)
        {

            NEXUS_DisplayHandle displayHandle(nullptr);

            std::string idOverride = IDisplay::Configuration();
            if (idOverride.empty() == false) {
                _nexusClientId = atoi(idOverride.c_str());
            }

#ifdef V3D_DRM_DISABLE
           ::setenv("V3D_DRM_DISABLE", "1", 1);
#endif

#ifdef BACKEND_BCM_NEXUS_NXCLIENT
           NxClient_JoinSettings joinSettings;
           NxClient_GetDefaultJoinSettings(&joinSettings);

           strcpy(joinSettings.name, name.c_str());

           NEXUS_Error rc = NxClient_Join(&joinSettings);
           BDBG_ASSERT(!rc);
#else
           NEXUS_Error rc = NEXUS_Platform_Join();
           BDBG_ASSERT(!rc);
#endif

           NXPL_RegisterNexusDisplayPlatform(&_nxplHandle, displayHandle);

           if (pipe(g_pipefd) == -1) {
               g_pipefd[0] = -1;
               g_pipefd[1] = -1;
           }

           _virtualkeyboard = virtualinput_open(name.c_str(), connectorName, VirtualKeyboardCallback, nullptr, nullptr);

           if (_virtualkeyboard == nullptr) {
               fprintf(stderr, "[LibinputServer] Initialization of virtual keyboard failed!!!\n");
           }

           printf("Constructed the Display: %p - %s", this, _displayName.c_str());
        }

        uint32_t NexusClientId() const { return _nexusClientId; }

    public:
        static Compositor::IDisplay* Instance(const std::string& displayName)
        {
            static Display myDisplay(displayName);

            return (&myDisplay);
        }

        virtual ~Display()
        {
            NXPL_UnregisterNexusDisplayPlatform(_nxplHandle);
#ifdef BACKEND_BCM_NEXUS_NXCLIENT
            NxClient_Uninit();
#endif
            if (_virtualkeyboard != nullptr) {
                virtualinput_close(_virtualkeyboard);
            }

            printf("Destructed the Display: %p - %s", this, _displayName.c_str());
        }

    public:
        // Lifetime management
        virtual void AddRef() const
        {
        }
        virtual uint32_t Release() const
        {
            // Display can not be destructed, so who cares :-)
            return (0);
        }

        // Methods
        virtual EGLNativeDisplayType Native() const override
        {
            return (static_cast<EGLNativeDisplayType>(EGL_DEFAULT_DISPLAY));
        }
        virtual const std::string& Name() const override
        {
            return (_displayName);
        }
        virtual int Process(const uint32_t data) override
        {
            Message message;
            if ((data != 0) && (g_pipefd[0] != -1) && (read(g_pipefd[0], &message, sizeof(message)) > 0)) {

                std::list<SurfaceImplementation*>::iterator index(_surfaces.begin());

                while (index != _surfaces.end()) {
                    // RELEASED  = 0,
                    // PRESSED   = 1,
                    // REPEAT    = 2,

                    (*index)->SendKey(message.code, (message.type == 0 ? IDisplay::IKeyboard::released : IDisplay::IKeyboard::pressed), time(nullptr));
                    index++;
                }
            }

            return (0);
        }
        virtual ISurface* Create(const std::string& name, const uint32_t width, const uint32_t height) override
        {
            return (new SurfaceImplementation(*this, name, width, height));
        }
        virtual int FileDescriptor() const override
        {
            return (g_pipefd[0]);
        }

    private:
        inline void Register(SurfaceImplementation* surface)
        {
            std::list<SurfaceImplementation*>::iterator index(std::find(_surfaces.begin(), _surfaces.end(), surface));

            if (index == _surfaces.end()) {
                _surfaces.push_back(surface);
            }
        }
        inline void Unregister(SurfaceImplementation* surface)
        {
            std::list<SurfaceImplementation*>::iterator index(std::find(_surfaces.begin(), _surfaces.end(), surface));

            if (index != _surfaces.end()) {
                _surfaces.erase(index);
            }
        }

    private:
        const std::string _displayName;
        NXPL_PlatformHandle _nxplHandle;
        void* _virtualkeyboard;
        std::list<SurfaceImplementation*> _surfaces;
        uint32_t _nexusClientId;
    };

} // Nexus

Compositor::IDisplay* Compositor::IDisplay::Instance(const std::string& displayName)
{
    return (Nexus::Display::Instance(displayName));
}
} // WPEFramework
