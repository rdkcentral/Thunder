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

#ifndef COMPOSITOR_CPP_ABSTRACTION_H
#define COMPOSITOR_CPP_ABSTRACTION_H

#include <assert.h>
#include <string.h>
#include <cstddef>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <string>

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
                pressed,
                repeated,
                completed
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

        struct IPointer {
            virtual ~IPointer() {}

            enum state {
                released = 0,
                pressed
            };

            // Lifetime management
            virtual void AddRef() const = 0;
            virtual uint32_t Release() const = 0;

            // Methods
            virtual void Direct(const uint8_t button, const state action) = 0;
            virtual void Direct(const uint16_t x, const uint16_t y) = 0;
        };

        struct IWheel {
            virtual ~IWheel() {}

            // Lifetime management
            virtual void AddRef() const = 0;
            virtual uint32_t Release() const = 0;

            // Methods
            virtual void Direct(const int16_t horizontal, const int16_t vertical) = 0;
        };

        struct ITouchPanel {
            virtual ~ITouchPanel() {}

            enum state {
                released = 0,
                pressed,
                motion
            };

            // Lifetime management
            virtual void AddRef() const = 0;
            virtual uint32_t Release() const = 0;

            // Methods
            virtual void Direct(const uint8_t index, const ITouchPanel::state state, const uint16_t x, const uint16_t y) = 0;
        };

        struct ISurface {
            virtual ~ISurface(){};

            // Lifetime management
            virtual void AddRef() const = 0;
            virtual uint32_t Release() const = 0;

            // Methods
            virtual EGLNativeWindowType Native() const = 0;
            virtual std::string Name() const = 0;
            virtual void Keyboard(IKeyboard* keyboard) { }
            virtual void Pointer(IPointer* pointer) { }
            virtual void Wheel(IWheel* wheel) { }
            virtual void TouchPanel(ITouchPanel* touchpanel) { }
            virtual int32_t Width() const = 0;
            virtual int32_t Height() const = 0;
        };

        static IDisplay* Instance(const std::string& displayName);

        // If CLIENT_IDENTIFIER variable is set its value is tokenized using comma as the separator
        // and the first token is used to override the passed in name. The second - if present - is
        // used as an underlaying compositor specific information, Configuration information.

        // This method is intended to be used by the layers "using" the compositor client. using the 
        // following method, a SuggestedName (typically the callsign) can be extracted from the 
        // environment and can be used in the "static IDisplay* Instance(<displayName>)" method.
        static std::string SuggestedName()
        {
            std::string result;
            const char* overrides = getenv("CLIENT_IDENTIFIER");
            if (overrides != nullptr) {
                const char* delimiter = strchr(overrides, ',');

                if (delimiter != nullptr) {
                    std::ptrdiff_t delimiterPos = delimiter - overrides;
                    result.insert (0, overrides, delimiterPos);
                }
                else {
                    result.insert (0, overrides);
                }
            }
            return (result);
        }

        // This method is intended to be used by the layers implementing the compositor client. 
        // using the following method, additional configuration information e.g. to which wayland 
        // server to connect to, or what ID to be used to connect to the nexus server, can be passed 
        // to the implementation layers.
        static std::string Configuration()
        {
            std::string result;
            const char* overrides = getenv("CLIENT_IDENTIFIER");
            if (overrides != nullptr) {
                const char* delimiter = strchr(overrides, ',');

                if (delimiter != nullptr) {
                    std::ptrdiff_t delimiterPos = delimiter - overrides;
                    result.insert (0, overrides + delimiterPos + 1);
                }
            }
            return (result);
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
