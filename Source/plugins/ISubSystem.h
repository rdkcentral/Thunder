/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 Metrological
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

#ifndef __ISYSTEMINFO_H
#define __ISYSTEMINFO_H

#include <com/ICOM.h>

namespace WPEFramework {
namespace PluginHost {

    struct EXTERNAL IMetadata : virtual public Core::IUnknown {
        enum {
            ID = RPC::ID_SYSTEM_METADATA
        };
        
        // Software information
        virtual string BuildTreeHash() const = 0;
        virtual uint8_t Major() const = 0;
        virtual uint8_t Minor() const = 0;
        virtual uint8_t Patch() const = 0;
    };

    // This interface gives direct access to a switchboard
    struct EXTERNAL ISubSystem : virtual public Core::IUnknown {
        enum {
            ID = RPC::ID_SUBSYSTEM
        };

        enum subsystem : uint32_t {
            PLATFORM = 0, // platform is available.
            SECURITY, // A security system can validate external requests (JSONRPC/WebRequest)
            NETWORK, // Network connectivity has been established.
            IDENTIFIER, // System identification has been accomplished.
            GRAPHICS, // Graphics screen EGL is available.
            INTERNET, // Network connectivity to the outside world has been established.
            LOCATION, // Location of the device has been set.
            TIME, // Time has been synchronized.
            PROVISIONING, // Provisioning information is available.
            DECRYPTION, // Decryption functionality is available.
            WEBSOURCE, // Content exposed via a local web server is available.
            STREAMING, // Content can be streamed.
            BLUETOOTH, // The bluetooth subsystem is up and running.
            END_LIST,

            // Also define a "negative" value.
            NEGATIVE_START = 0x80000000,
            NOT_PLATFORM = NEGATIVE_START, // platform is NOT available.
            NOT_SECURITY, // A security system can validate external requests (JSONRPC/WebRequest)
            NOT_NETWORK, // Network connectivity has NOT been established.
            NOT_IDENTIFIER, // System identification has NOT been accomplished.
            NOT_GRAPHICS, // Graphics screen EGL is NOT available.
            NOT_INTERNET, // Network connectivity to the outside world has been established.
            NOT_LOCATION, // Location of the device has NOT been set.
            NOT_TIME, // Time has been NOT synchronized.
            NOT_PROVISIONING, // Provisioning information is NOT available.
            NOT_DECRYPTION, // Decryption functionality is NOT available.
            NOT_WEBSOURCE, // Content exposed via a local web server is NOT available.
            NOT_STREAMING, // Content can NOT be streamed.
            NOT_BLUETOOTH // The Bluetooth communication system is NOT available.
        };

        struct EXTERNAL INotification : virtual public Core::IUnknown {

            enum {
                ID = RPC::ID_SUBSYSTEM_NOTIFICATION
            };

            // Some change happened with respect to any of the subsystems
            virtual void Updated() = 0;
        };

        struct EXTERNAL ISecurity : virtual public Core::IUnknown {

            enum { ID = RPC::ID_SUBSYSTEM_SECURITY };

            enum { SUBSYSTEM = SECURITY };

            // Security information
            virtual string Callsign() const = 0;    
        };

        struct EXTERNAL IInternet : virtual public Core::IUnknown {

            enum { ID = RPC::ID_SUBSYSTEM_INTERNET };

            enum { SUBSYSTEM = INTERNET };

            enum network_type : uint8_t {
                UNKNOWN,
                IPV4,
                IPV6
            };

            // Network information
            virtual string PublicIPAddress() const = 0;
            virtual network_type NetworkType() const = 0;

            static const TCHAR* ToString(const network_type value);
        };

        // Location information
        struct EXTERNAL ILocation : virtual public Core::IUnknown {

            enum { ID = RPC::ID_SUBSYSTEM_LOCATION };

            enum { SUBSYSTEM = LOCATION };

            // Location information
            virtual string TimeZone() const = 0;
            virtual string Country() const = 0;
            virtual string Region() const = 0;
            virtual string City() const = 0;
            virtual int32_t Latitude() const { return (51832547); }
            virtual int32_t Longitude() const { return (5674899); }
        };

        // Device specific identification.
        struct EXTERNAL IIdentifier : virtual public Core::IUnknown {

            enum { ID = RPC::ID_SUBSYSTEM_IDENTIFIER };

            enum { SUBSYSTEM = IDENTIFIER };

            // Device specific identification.
            virtual uint8_t Identifier(const uint8_t length, uint8_t buffer[] /*@maxlength:length @out*/) const = 0;
            virtual string Architecture() const = 0;
            virtual string Chipset() const = 0;
            virtual string FirmwareVersion() const = 0;
        };

        // Time synchronisation reporting
        struct EXTERNAL ITime : virtual public Core::IUnknown {

            enum { ID = RPC::ID_SUBSYSTEM_TIME };

            enum { SUBSYSTEM = TIME };

            // Time synchronisation reporting.
            virtual uint64_t TimeSync() const = 0;
        };

        // IProvisioning reporting
        struct EXTERNAL IProvisioning : virtual public Core::IUnknown, public RPC::IStringIterator {

            enum { ID = RPC::ID_SUBSYSTEM_PROVISIONING };

            enum { SUBSYSTEM = PROVISIONING };

            virtual string Storage() const = 0;

            virtual bool Next(string& info /* @out */) = 0;
            virtual bool Previous(string& info /* @out */) = 0;
            virtual void Reset(const uint32_t position) = 0;
            virtual bool IsValid() const = 0;
            virtual uint32_t Count() const = 0;
            virtual string Current() const = 0;
        };

        // Decryption reporting
        struct EXTERNAL IDecryption : virtual public Core::IUnknown, public RPC::IStringIterator {

            enum { ID = RPC::ID_SUBSYSTEM_DECRYPTION };

            enum { SUBSYSTEM = DECRYPTION };

            virtual bool Next(string& info /* @out */) = 0;
            virtual bool Previous(string& info /* @out */) = 0;
            virtual void Reset(const uint32_t position) = 0;
            virtual bool IsValid() const = 0;
            virtual uint32_t Count() const = 0;
            virtual string Current() const = 0;
        };

        virtual ~ISubSystem() = default;

        virtual void Register(ISubSystem::INotification* notification) = 0;
        virtual void Unregister(ISubSystem::INotification* notification) = 0;

        // Events setter and getters.
        virtual void Set(const subsystem type, Core::IUnknown* information) = 0;
        virtual const Core::IUnknown* Get(const subsystem type) const = 0;
        virtual bool IsActive(const subsystem type) const = 0;

        // Report some version info on the Hosting Application
        virtual string BuildTreeHash() const = 0;
        virtual string Version() const = 0;

        template <typename REQUESTEDINTERFACE>
        const REQUESTEDINTERFACE* Get() const
        {
            const Core::IUnknown* baseInterface(Get(static_cast<const subsystem>(REQUESTEDINTERFACE::SUBSYSTEM)));

            if (baseInterface != nullptr) {
                const REQUESTEDINTERFACE* result = baseInterface->QueryInterface<const REQUESTEDINTERFACE>();

                baseInterface->Release();

                return (result);
            }

            return (nullptr);
        }
    };

} // namespace PluginHost
}

#endif // __ISYSTEMINFO_H
