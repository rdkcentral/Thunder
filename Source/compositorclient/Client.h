#ifndef COMPOSITOR_CPP_ABSTRACTION_H
#define COMPOSITOR_CPP_ABSTRACTION_H

#include <assert.h>
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

        // If CLIENT_IDENTIFIER variable is set its value is tokenized using comma as the separator
        // and the first token is used to override the passed in name. The second - if present - is
        // used as an underlaying compositor specific identifier.
        // See GetOverrides()
        static IDisplay* Instance(const std::string& displayName);
        static bool GetOverrides(std::string* name, std::string* identifier)
        {
            assert(name != nullptr || identifier != nullptr);

            const char* overrides = getenv("CLIENT_IDENTIFIER");
            if (overrides != nullptr && overrides[0] != '\0') {
                const char* delimiter = strchr(overrides, ',');
                if (delimiter != nullptr) {
                    ptrdiff_t delimiterPos = delimiter - overrides;
                    if (name != nullptr) {
                        name->clear();
                        name->insert(0, overrides, delimiterPos);
                    }

                    if (identifier != nullptr) {
                        identifier->clear();
                        if (delimiterPos + 1 < strlen(overrides))
                            identifier->insert(0, overrides + delimiterPos + 1);
                    }
                } else {
                    // No delimiter found so it's only a name.
                    if (name != nullptr) {
                        name->clear();
                        name->insert(0, overrides);
                    }

                    if (identifier != nullptr) {
                        identifier->clear();
                    }
                }

                return true;
            }

            return false;
        }

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
