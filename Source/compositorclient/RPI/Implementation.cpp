#include "Module.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <bcm_host.h>

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

class BCMHostInit {
public:
    BCMHostInit(const BCMHostInit&) = delete;
    BCMHostInit& operator=(const BCMHostInit&) = delete;

    BCMHostInit()
    {
        bcm_host_init();
    }

    ~BCMHostInit()
    {
        bcm_host_deinit();
    }
};
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

        void Opacity(const uint32_t value) override;
        void ChangedGeometry(const Exchange::IComposition::Rectangle& rectangle) override;
        void ChangedZOrder(const uint8_t zorder) override;

        virtual string Name() const override
        {
            return _name;
        }
        virtual void Kill() override
        {
            //todo: implement
            TRACE(CompositorClient, (_T("Kill called for Client %s. Not supported."), Name().c_str()));
        }
        inline EGLNativeWindowType Native() const
        {
            return (static_cast<EGLNativeWindowType>(_nativeSurface));
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
        const uint32_t _width;
        const uint32_t _height;
        uint32_t _opacity;
        uint32_t _layer;

        EGLSurface _nativeSurface;
        EGL_DISPMANX_WINDOW_T _nativeWindow;
        DISPMANX_DISPLAY_HANDLE_T _dispmanDisplay;
        DISPMANX_UPDATE_HANDLE_T _dispmanUpdate;
        DISPMANX_ELEMENT_HANDLE_T _dispmanElement;

        VC_RECT_T _dstRect;
        VC_RECT_T _srcRect;

        IKeyboard* _keyboard;
        IWheel* _wheel;
        IPointer* _pointer;
        ITouchPanel* _touchpanel;
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
        return (static_cast<EGLNativeDisplayType>(EGL_DEFAULT_DISPLAY));
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
        return _displaysize.first;
    }

    inline uint32_t DisplaySizeHeight() const
    {
        return _displaysize.second;
    }

private:
    inline void Register(SurfaceImplementation* surface);
    inline void Unregister(SurfaceImplementation* surface);
    inline void OfferClientInterface(Exchange::IComposition::IClient* client);
    inline void RevokeClientInterface(Exchange::IComposition::IClient* client);

    using DisplaySize = std::pair<uint32_t, uint32_t>;

    inline static DisplaySize RetrieveDisplaySize()
    {
        DisplaySize displaysize;
        graphics_get_display_size(0, &displaysize.first, &displaysize.second);
        return displaysize;
    }

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
            Core::ProxyType<RPC::InvokeServerType<2, 1>> engine = Core::ProxyType<RPC::InvokeServerType<2, 1>>::Create(Core::Thread::DefaultStackSize());
            ASSERT(engine != nullptr);

            _compositerServerRPCConnection = Core::ProxyType<RPC::CommunicatorClient>::Create(Connector(), Core::ProxyType<Core::IIPCServer>(engine));
            ASSERT(_compositerServerRPCConnection != nullptr);

            engine->Announcements(_compositerServerRPCConnection->Announcement());
        }

        uint32_t result = _compositerServerRPCConnection->Open(RPC::CommunicationTimeOut);

        if (result != Core::ERROR_NONE) {
            TRACE(CompositorClient, (_T("Could not open connection to Compositor with node %s. Error: %s"), _compositerServerRPCConnection->Source().RemoteId(), Core::NumberType<uint32_t>(result).Text()));
            _compositerServerRPCConnection.Release();
        }

        _virtualinput = virtualinput_open(_displayName.c_str(), connectorNameVirtualInput, VirtualKeyboardCallback, VirtualMouseCallback, VirtualTouchScreenCallback);

        if (_virtualinput == nullptr) {
            TRACE(CompositorClient, (_T("Initialization of virtual input failed for Display %s!"), Name()));
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
                TRACE(CompositorClient, (_T("Compositor Surface [%s] is not properly destructed"), name.c_str()));
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
    const DisplaySize _displaysize;
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
{

    TRACE(CompositorClient, (_T("Created client named: %s"), _name.c_str()));

    VC_DISPMANX_ALPHA_T alpha = {
        static_cast<DISPMANX_FLAGS_ALPHA_T>(DISPMANX_FLAGS_ALPHA_FROM_SOURCE | DISPMANX_FLAGS_ALPHA_MIX),
        255,
        0
    };
    vc_dispmanx_rect_set(&_dstRect, 0, 0, _display.DisplaySizeWidth(), _display.DisplaySizeHeight());
    vc_dispmanx_rect_set(&_srcRect,
        0, 0, (_display.DisplaySizeWidth() << 16), (_display.DisplaySizeHeight() << 16));

    _dispmanDisplay = vc_dispmanx_display_open(0);
    _dispmanUpdate = vc_dispmanx_update_start(0);
    _dispmanElement = vc_dispmanx_element_add(
        _dispmanUpdate,
        _dispmanDisplay,
        _layer,
        &_dstRect,
        0 /*src*/,
        &_srcRect,
        DISPMANX_PROTECTION_NONE,
        &alpha /*alpha*/,
        0 /*clamp*/,
        DISPMANX_NO_ROTATE);
    vc_dispmanx_update_submit_sync(_dispmanUpdate);

    _nativeWindow.element = _dispmanElement;
    _nativeWindow.width = _display.DisplaySizeWidth();
    _nativeWindow.height = _display.DisplaySizeHeight();
    _nativeSurface = static_cast<EGLSurface>(&_nativeWindow);

    _display.Register(this);
}

Display::SurfaceImplementation::~SurfaceImplementation()
{

    TRACE(CompositorClient, (_T("Destructing client named: %s"), _name.c_str()));

    _display.Unregister(this);
    _dispmanUpdate = vc_dispmanx_update_start(0);
    vc_dispmanx_element_remove(_dispmanUpdate, _dispmanElement);
    vc_dispmanx_update_submit_sync(_dispmanUpdate);
    vc_dispmanx_display_close(_dispmanDisplay);
}

void Display::SurfaceImplementation::Opacity(
    const uint32_t value)
{

    _opacity = (value > Exchange::IComposition::maxOpacity) ? Exchange::IComposition::maxOpacity : value;

    _dispmanUpdate = vc_dispmanx_update_start(0);
    vc_dispmanx_element_change_attributes(_dispmanUpdate,
        _dispmanElement,
        (1 << 1),
        _layer,
        _opacity,
        &_dstRect,
        &_srcRect,
        0,
        DISPMANX_NO_ROTATE);
    vc_dispmanx_update_submit_sync(_dispmanUpdate);
}

void Display::SurfaceImplementation::ChangedGeometry(const Exchange::IComposition::Rectangle& rectangle)
{
    vc_dispmanx_rect_set(&_dstRect, rectangle.x, rectangle.y, rectangle.width, rectangle.height);
    vc_dispmanx_rect_set(&_srcRect,
        0, 0, (_display.DisplaySizeWidth() << 16), (_display.DisplaySizeHeight() << 16));

    _dispmanUpdate = vc_dispmanx_update_start(0);
    vc_dispmanx_element_change_attributes(_dispmanUpdate,
        _dispmanElement,
        (1 << 2),
        _layer,
        _opacity,
        &_dstRect,
        &_srcRect,
        0,
        DISPMANX_NO_ROTATE);
    vc_dispmanx_update_submit_sync(_dispmanUpdate);
}
void Display::SurfaceImplementation::ChangedZOrder(const uint8_t zorder)
{
    _dispmanUpdate = vc_dispmanx_update_start(0);
    int8_t layer = 0;

    if (zorder == static_cast<uint8_t>(~0)) {
        layer = -1;
    } else {
        layer = zorder;
        _layer = layer;
    }
    vc_dispmanx_element_change_layer(_dispmanUpdate, _dispmanElement, layer);
    vc_dispmanx_update_submit_sync(_dispmanUpdate);
}

Display::Display(const string& name)
    : _isRunning(true)
    , _displayName(name)
    , _adminLock()
    , _virtualinput(nullptr)
    , _displaysize(RetrieveDisplaySize())
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
            const IDisplay::IKeyboard::state state = ((message.keyData.type == KEY_RELEASED)? IDisplay::IKeyboard::released : IDisplay::IKeyboard::pressed);
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

    std::list<SurfaceImplementation*>::iterator index(
        std::find(_surfaces.begin(), _surfaces.end(), surface));
    ASSERT(index != _surfaces.end());
    _adminLock.Unlock();

    RevokeClientInterface(surface);
}

void Display::OfferClientInterface(Exchange::IComposition::IClient* client)
{
    ASSERT(client != nullptr);

    _adminLock.Lock();
    uint32_t result = _compositerServerRPCConnection->Offer(client);
    _adminLock.Unlock();

    if (result != Core::ERROR_NONE) {
        TRACE(CompositorClient, (_T("Could not offer IClient interface with callsign %s to Compositor. Error: %s"), client->Name(), Core::NumberType<uint32_t>(result).Text()));
    }
}

void Display::RevokeClientInterface(Exchange::IComposition::IClient* client)
{
    ASSERT(client != nullptr);

    _adminLock.Lock();
    uint32_t result = _compositerServerRPCConnection->Revoke(client);
    _adminLock.Unlock();

    if (result != Core::ERROR_NONE) {
        TRACE(CompositorClient, (_T("Could not revoke IClient interface with callsign %s to Compositor. Error: %s"), client->Name(), Core::NumberType<uint32_t>(result).Text()));
    }
}

} // RPI

Compositor::IDisplay* Compositor::IDisplay::Instance(const string& displayName)
{
    static BCMHostInit bcmhostinit; // must be done before Display constructor
    static RPI::Display& myDisplay = Core::SingletonType<RPI::Display>::Instance(displayName);
    myDisplay.AddRef();

    return (&myDisplay);
}
} // WPEFramework
