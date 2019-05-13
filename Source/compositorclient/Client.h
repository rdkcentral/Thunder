#ifndef COMPOSITOR_CPP_ABSTRACTION_H
#define COMPOSITOR_CPP_ABSTRACTION_H

#include <EGL/egl.h>
#include <EGL/eglext.h>

#if __cplusplus <= 199711L
#define nullptr NULL
#endif

namespace WPEFramework {
namespace Compositor {

    struct IDisplay {
        struct IKeyboard {
            virtual ~IKeyboard() {}

            enum state {
                released = 0,
                pressed
            };

            // Lifetime management
            virtual void AddRef() const = 0;
            virtual uint32_t Release() const = 0;

            // Methods
            virtual void KeyMap(const char information[], const uint16_t size) = 0;
            virtual void Key(const uint32_t key, const state action, const uint32_t time) = 0;
            virtual void Modifiers(uint32_t depressedMods, uint32_t latchedMods, uint32_t lockedMods, uint32_t group) = 0;
            virtual void Repeat(int32_t rate, int32_t delay) = 0;
            virtual void Direct(const uint32_t key, const state action) = 0;
        };
        struct ISurface {
            virtual ~ISurface(){};

            // Lifetime management
            virtual void AddRef() const = 0;
            virtual uint32_t Release() const = 0;

            // Methods
            virtual EGLNativeWindowType Native() const = 0;
            virtual std::string Name() const = 0;
            virtual void Keyboard(IKeyboard* keyboard) = 0;
            virtual int32_t Width() const = 0;
            virtual int32_t Height() const = 0;
        };

        static IDisplay* Instance(const std::string& displayName);

        virtual ~IDisplay() {}

        // Lifetime management
        virtual void AddRef() const = 0;
        virtual uint32_t Release() const = 0;

        // Methods
        virtual EGLNativeDisplayType Native() const = 0;
        virtual const std::string& Name() const = 0;
        virtual ISurface* Create(const std::string& name, const uint32_t width, const uint32_t height) = 0; //initial position on screen is fullscreen,x and y therefore implicit and 0
        virtual int Process(const uint32_t data) = 0;
        virtual int FileDescriptor() const = 0;
    };
} // Compositor
} // WPEFramework

#endif // COMPOSITOR_CPP_ABSTRACTION_H
