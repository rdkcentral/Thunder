/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2021 Metrological
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

#pragma once

#include "Module.h"
#include "SDPSocket.h"

#include <set>


namespace Thunder {

namespace Bluetooth {

namespace SDP {

    static constexpr uint16_t CHARSET_US_ASCII = 3;
    static constexpr uint16_t CHARSET_UTF8 = 106;

    class EXTERNAL ClassID {
    public:
        enum id : uint16_t {
            Undefined   = 0x0000,

            // Protocols
            SDP         = 0x0001,
            UDP         = 0x0002,
            RFCOMM      = 0x0003,
            TCP         = 0x0004,
            TCS_BIN     = 0x0005,
            TCS_AT      = 0x0006,
            ATT         = 0x0007,
            OBEX        = 0x0008,
            IP          = 0x0009,
            FTP         = 0x000a,
            HTTP        = 0x000c,
            WSP         = 0x000e,
            BNEP        = 0x000f,
            UPNP        = 0x0010,
            HIDP        = 0x0011,
            HCRP_CTRL   = 0x0012,
            HCRP_DATA   = 0x0014,
            HCRP_NOTE   = 0x0016,
            AVCTP       = 0x0017,
            AVDTP       = 0x0019,
            CMTP        = 0x001b,
            UDI         = 0x001d,
            MCAP_CTRL   = 0x001e,
            MCAP_DATA   = 0x001f,
            L2CAP       = 0x0100,

            // SDP itself
            ServiceDiscoveryServer                  = 0x1000, // Service
            BrowseGroupDescriptor                   = 0x1001,
            PublicBrowseRoot                        = 0x1002,

            // Services and Profiles
            SerialPort                              = 0x1101, // Service + Profile
            LANAccessUsingPPP                       = 0x1102, // Service + Profile
            DialupNetworking                        = 0x1103, // Service + Profile
            IrMCSync                                = 0x1104, // Service + Profile
            OBEXObjectPush                          = 0x1105, // Service + Profile
            OBEXFileTransfer                        = 0x1106, // Service + Profile
            IrMCSyncCommand                         = 0x1107, // Service
            HeadsetHSP                              = 0x1108, // Service + Profile
            CordlessTelephony                       = 0x1109, // Service + Profile
            AudioSource                             = 0x110A, // Service
            AudioSink                               = 0x110B, // Service
            AVRemoteControlTarget                   = 0x110C, // Service
            AdvancedAudioDistribution               = 0x110D, //           Profile
            AVRemoteControl                         = 0x110E, // Service + Profile
            AVRemoteControlController               = 0x110F, // Service
            Intercom                                = 0x1110, // Service + Profile
            Fax                                     = 0x1111, // Service + Profile
            HeadsetAudioGateway                     = 0x1112, // Service
            WAP                                     = 0x1113, // Service
            WAPClient                               = 0x1114, // Service
            PANU                                    = 0x1115, // Service + Profile
            NAP                                     = 0x1116, // Service + Profile
            GN                                      = 0x1117, // Service + Profile
            DirectPrinting                          = 0x1118, // Service
            ReferencePrinting                       = 0x1119, // Service
            BasicImagingProfile                     = 0x111A, //           Profile
            ImagingResponder                        = 0x111B, // Service
            ImagingAutomaticArchive                 = 0x111C, // Service
            ImagingReferencedObjects                = 0x111D, // Service
            Handsfree                               = 0x111E, // Service + Profile
            HandsfreeAudioGateway                   = 0x111F, // Service
            DirectPrintingReferenceObjects          = 0x1120, // Service
            ReflectedUI                             = 0x1121, // Service
            BasicPrinting                           = 0x1122, //           Profile
            PrintingStatus                          = 0x1123, // Service
            HumanInterfaceDeviceService             = 0x1124, // Service + Profile
            HardcopyCableReplacement                = 0x1125, //           Profile
            HCRPrint                                = 0x1126, // Service
            HCRScan                                 = 0x1127, // Service
            CommonISDNAccess                        = 0x1128, // Service + Profile
            SIMAccess                               = 0x112D, // Service + Profile
            PhonebookAccessPCE                      = 0x112E, // Service
            PhonebookAccessPSE                      = 0x112F, // Service
            PhonebookAccess                         = 0x1130, //           Profile
            HeadsetHS                               = 0x1131, // Service
            MessageAccessServer                     = 0x1132, // Service
            MessageNotificationServer               = 0x1133, // Service
            MessageAccess                           = 0x1134, //           Profile
            GNSS                                    = 0x1135, //           Profile
            GNSSServer                              = 0x1136, // Service
            ThreeDDisplay                           = 0x1137, // Service
            ThreeDGlasses                           = 0x1138, // Service
            ThreeDSynchronisation                   = 0x1339, //           Profile
            MPS                                     = 0x113A, //           Profile
            MPSSC                                   = 0x113B, // Service
            CTNAccessService                        = 0x113C, // Service
            CTNNotificationService                  = 0x113D, // Service
            CTN                                     = 0x113E, //           Profile
            PnPInformation                          = 0x1200, // Service
            GenericNetworking                       = 0x1201, // Service
            GenericFileTransfer                     = 0x1202, // Service
            GenericAudio                            = 0x1203, // Service
            GenericTelephony                        = 0x1204, // Service
            UPNPService                             = 0x1205, // Service
            UPNPIPService                           = 0x1206, // Service
            ESDPUPNPIPPAN                           = 0x1300, // Service
            ESDPUPNPIPLAP                           = 0x1301, // Service
            ESDPUPNPL2CAP                           = 0x1302, // Service
            VideoSource                             = 0x1303, // Service
            VideoSink                               = 0x1304, // Service
            VideoDistribution                       = 0x1305, //          Profile
            HDP                                     = 0x1400, //          Profile
            HDPSource                               = 0x1401, // Service
            HDPSink                                 = 0x1402  // Service
        };

    public:
        ClassID()
            : _id()
        {
        }
        ClassID(const ClassID& other)
            : _id(other._id)
        {
        }
        ClassID(const id& classId)
            : _id(UUID(classId))
        {
        }
        ClassID(const UUID& uuid)
            : _id(uuid)
        {
        }
        ~ClassID() = default;

    public:
        ClassID& operator=(const ClassID& rhs)
        {
            _id = rhs._id;
            return (*this);
        }
        bool operator==(const ClassID& rhs) const
        {
            return (_id == rhs._id);
        }
        bool operator!=(const ClassID& rhs) const
        {
            return !(*this == rhs);
        }
        bool operator<(const ClassID& rhs) const
        {
            return (_id < rhs._id);
        }

    public:
        const UUID& Type() const
        {
            return (_id);
        }
        const string Name() const
        {
            string name;
            if (_id.HasShort() == true) {
                id input = static_cast<id>(_id.Short());
                Core::EnumerateType<id> value(input);
                name = (value.IsSet() == true? string(value.Data()) : _id.ToString(false));
            }
            if (name.empty() == true) {
                name = _id.ToString();
            }
            return (name);
        }

    private:
        UUID _id;
    }; // class ClassID

    class EXTERNAL Service {
    public:
        class EXTERNAL AttributeDescriptor {
        public:
            // universal attributes
            enum id : uint16_t {
                // required
                ServiceRecordHandle             = 0x0000,
                ServiceClassIDList              = 0x0001,
                // optional
                ServiceRecordState              = 0x0002,
                ServiceID                       = 0x0003,
                ProtocolDescriptorList          = 0x0004,
                BrowseGroupList                 = 0x0005,
                LanguageBaseAttributeIDList     = 0x0006,
                ServiceInfoTimeToLive           = 0x0007,
                ServiceAvailability             = 0x0008,
                ProfileDescriptorList           = 0x0009,
                DocumentationURL                = 0x000a,
                ClientExecutableURL             = 0x000b,
                IconURL                         = 0x000c,
            };

            static constexpr uint16_t ServiceNameOffset        = 0;
            static constexpr uint16_t ServiceDescriptionOffset = 1;
            static constexpr uint16_t ProviderNameOffset       = 2;

            // specific to Advanced Audio Distribution
            enum class a2dp : uint16_t {
                SupportedFeatures               = 0x0311
            };

            // specfic to AVRemoteControl profile
            enum class avcrp : uint16_t {
                SupportedFeatures               = 0x0311
            };

            // specific to Basic Imaging profile
            enum class bip : uint16_t  {
                GoepL2capPsm                    = 0x0200,
                SupportedCapabilities           = 0x0300,
                SupportedFeatures               = 0x0311,
                SupportedFunctions              = 0x0312,
                TotalImagingDataCapacity        = 0x0313
            };

            // specific to PnPInformation profile
            enum class did : uint16_t {
                SpecificationID                 = 0x0200,
                VendorID                        = 0x0201,
                ProductID                       = 0x0202,
                Version                         = 0x0203,
                PrimaryRecord                   = 0x0204,
                VendorIDSource                  = 0x0205
            };

        public:
            AttributeDescriptor() = delete;
            AttributeDescriptor(const AttributeDescriptor&) = delete;
            AttributeDescriptor& operator=(const AttributeDescriptor&) = delete;

            template<typename T>
            AttributeDescriptor(const uint16_t id, T&& value)
                : _id(id)
                , _value(std::forward<T>(value))
            {
                TRACE_L5("AttributeDescriptor: adding raw attribute 0x%04x", id);
            }

            ~AttributeDescriptor() = default;

        public:
            bool operator==(const AttributeDescriptor& rhs) const
            {
                return (_id == rhs._id);
            }
            bool operator!=(const AttributeDescriptor& rhs) const
            {
                return !(*this == rhs);
            }
            bool operator<(const AttributeDescriptor& rhs) const
            {
                return (_id < rhs._id);
            }

        public:
            uint32_t Id() const
            {
                return (_id);
            }
            const Buffer& Value() const
            {
                return (_value);
            }
            string Name() const
            {
                Core::EnumerateType<id> value(_id);
                string name = (value.IsSet() == true? string(value.Data()) : _T("<custom>"));
                return (name);
            }

        private:
            uint16_t _id;
            Buffer _value;
        }; // class AttributeDescriptor

        struct Data {
            template<typename T>
            class EXTERNAL Element {
            public:
                Element() = delete;
                Element(const Element&) = default;
                Element& operator=(const Element&) = default;

                Element(T&& data)
                    : _data(std::move(data))
                {
                }
                Element(const T& data)
                    : _data(data)
                {
                }
                Element(const Buffer& buffer)
                    : _data()
                {
                    const Payload payload(buffer);
                    if (payload.Available() > 0) {
                        payload.Pop(use_descriptor, _data);
                    }
                }
                ~Element() = default;

            public:
                const T& Value() const
                {
                    return (_data);
                }

            public:
                operator Buffer() const
                {
                    uint8_t scratchpad[256];
                    Payload payload(scratchpad, sizeof(scratchpad), 0);
                    payload.Push(use_descriptor, _data);
                    return (payload);
                }

            private:
                T _data;
            }; // class Element
        }; // struct Data

        struct Protocol {
        public:
            class EXTERNAL L2CAP : public Data::Element<uint16_t> {
            public:
                using Element::Element;
                L2CAP(const L2CAP&) = default;
                L2CAP& operator=(const L2CAP&) = default;
                ~L2CAP() = default;

            public:
                uint16_t PSM() const
                {
                    return (Value());
                }
            }; // class L2CAP

            class EXTERNAL AVDTP : public Data::Element<uint16_t> {
            public:
                using Element::Element;
                AVDTP(const AVDTP&) = default;
                AVDTP& operator=(const AVDTP&) = default;
                ~AVDTP() = default;

            public:
                uint16_t Version() const
                {
                    return (Value());
                }
            }; // class AVDTP
        }; // struct Protocol

        struct Attribute {
            class EXTERNAL ServiceRecordHandle {
            public:
                static constexpr auto type = AttributeDescriptor::ServiceRecordHandle;

            public:
                ServiceRecordHandle()
                    : _handle(0)
                {
                }
                ServiceRecordHandle(const uint32_t handle)
                    : _handle(handle)
                {
                }
                ServiceRecordHandle(const uint8_t buffer[], const uint16_t size)
                {
                    uint32_t handle{};
                    const Payload payload(buffer, size);
                    payload.Pop(use_descriptor, handle);
                    Handle(handle);
                }
                ServiceRecordHandle(const ServiceRecordHandle&) = delete;
                ServiceRecordHandle& operator=(const ServiceRecordHandle&) = delete;
                ~ServiceRecordHandle() = default;

            public:
                operator Buffer() const
                {
                    uint8_t scratchPad[8];
                    Payload payload(scratchPad, sizeof(scratchPad), 0);
                    payload.Push(use_descriptor, _handle);
                    return (payload);
                }

            public:
                void Handle(const uint32_t handle)
                {
                    TRACE_L5("ServiceRecordHandle: handle 0x%08x", handle);
                    _handle = handle;
                }
                uint32_t Handle() const
                {
                    return (_handle);
                }

            private:
                uint32_t _handle;
            }; // class ServiceRecordHandle

            class EXTERNAL ServiceClassIDList {
            public:
                static constexpr auto type = AttributeDescriptor::ServiceClassIDList;

            public:
                ServiceClassIDList()
                    : _classes()
                {
                }
                ServiceClassIDList(const uint8_t buffer[], const uint16_t size)
                {
                    const Payload payload(buffer, size);
                    payload.Pop(use_descriptor, [&](const Payload& sequence) {
                        while (sequence.Available() > 0) {
                            UUID uuid;
                            sequence.Pop(use_descriptor, uuid);
                            Add(uuid);
                        }
                    });
                }
                ServiceClassIDList(const ServiceRecordHandle&) = delete;
                ServiceClassIDList& operator=(const ServiceRecordHandle&) = delete;
                ~ServiceClassIDList() = default;

            public:
                operator Buffer() const
                {
                    uint8_t scratchPad[256];
                    Payload payload(scratchPad, sizeof(scratchPad), 0);
                    payload.Push(use_descriptor, [this](Payload& sequence) {
                        for (auto const& classId : Classes()) {
                            sequence.Push(use_descriptor, classId.Type());
                        }
                    });

                    return (payload);
                }

            public:
                void Add(const ClassID& classId)
                {
                    TRACE_L5("ServiceClassIDList: Added class %s '%s'", classId.Type().ToString().c_str(), classId.Name().c_str());
                    _classes.emplace(classId);
                }
                const std::set<ClassID>& Classes() const
                {
                    return (_classes);
                }
                bool HasID(const UUID& id) const
                {
                    return (_classes.find(id) != _classes.cend());
                }

            private:
                std::set<ClassID> _classes;
            }; // class ServiceClassIDList

            class EXTERNAL ProtocolDescriptorList {
            public:
                static constexpr auto type = AttributeDescriptor::ProtocolDescriptorList;

            public:
                ProtocolDescriptorList()
                    : _protocols()
                {
                }
                ProtocolDescriptorList(const uint8_t buffer[], const uint16_t size)
                {
                    const Payload payload(buffer, size);
                    payload.Pop(use_descriptor, [&](const Payload& sequence) {
                        while (sequence.Available() > 0) {
                            sequence.Pop(use_descriptor, [&](const Payload& record) {
                                UUID uuid;
                                Buffer params;
                                record.Pop(use_descriptor, uuid);
                                record.Pop(params, record.Available()); // No descriptor! Just take everything what's left in this record.
                                Add(uuid, params);
                            });
                        }
                    });
                }
                ProtocolDescriptorList(const ProtocolDescriptorList&) = delete;
                ProtocolDescriptorList& operator=(const ProtocolDescriptorList&) = delete;
                ~ProtocolDescriptorList() = default;

            public:
                operator Buffer() const
                {
                    uint8_t scratchPad[256];
                    Payload payload(scratchPad, sizeof(scratchPad), 0);
                    payload.Push(use_descriptor, [this](Payload& sequence) {
                        for (auto const& kv : Protocols()) {
                            sequence.Push(use_descriptor, [&kv](Payload& record) {
                                record.Push(use_descriptor, kv.first.Type());
                                record.Push(kv.second); // no descriptor here!
                            });
                        }
                    });

                    return (payload);
                }

            public:
                template<typename T>
                void Add(const ClassID& id, T&& data)
                {
                    TRACE_L5("ProtocolDescriptorList: added %s '%s'", id.Type().ToString().c_str(), id.Name().c_str());
                    _protocols.emplace(id, std::forward<T>(data));
                }
                const std::map<ClassID, Buffer>& Protocols() const
                {
                    return (_protocols);
                }
                const Buffer* Protocol(const UUID& id) const
                {
                    auto const& it = _protocols.find(id);
                    if (it != _protocols.cend()) {
                        return (&(*it).second);
                    } else {
                        return (nullptr);
                    }
                }

            private:
                std::map<ClassID, Buffer> _protocols;
            }; // class ProtocolDescriptorList

            class EXTERNAL BrowseGroupList : public ServiceClassIDList {
            public:
                static constexpr auto type = AttributeDescriptor::BrowseGroupList;

            public:
                using ServiceClassIDList::ServiceClassIDList;
                BrowseGroupList(const BrowseGroupList&) = delete;
                BrowseGroupList& operator=(const BrowseGroupList&) = delete;
                ~BrowseGroupList() = default;
            }; // class BrowseGroupList

            class LanguageBaseAttributeIDList {
            public:
                static constexpr auto type = AttributeDescriptor::LanguageBaseAttributeIDList;

            public:
                class Triplet {
                public:
                    Triplet() = delete;
                    Triplet(const Triplet&) = default;
                    Triplet& operator=(const Triplet&) = default;

                    Triplet(const uint16_t language, const uint16_t charset, const uint16_t base)
                        : _language(language)
                        , _charset(charset)
                        , _base(base)
                    {
                    }

                    ~Triplet() = default;

                public:
                    uint16_t Language() const
                    {
                        return (_language);
                    }
                    uint16_t Charset() const
                    {
                        return (_charset);
                    }
                    uint16_t Base() const
                    {
                        return (_base);
                    }

                private:
                    uint16_t _language;
                    uint16_t _charset;
                    uint16_t _base;
                };

            public:
                LanguageBaseAttributeIDList()
                    : _languageBases()
                    , _freeBase(0x100)
                {
                }
                LanguageBaseAttributeIDList(const uint8_t buffer[], const uint16_t size)
                {
                    const Payload payload(buffer, size);
                    payload.Pop(use_descriptor, [&](const Payload& sequence) {
                        while (sequence.Available() > 0) {
                            uint16_t lang = 0;
                            uint16_t charset = 0;
                            uint16_t base = 0;

                            sequence.Pop(use_descriptor, lang);
                            sequence.Pop(use_descriptor, charset);
                            sequence.Pop(use_descriptor, base);

                            Add(lang, charset, base);
                        }
                    });
                }
                LanguageBaseAttributeIDList(const LanguageBaseAttributeIDList&) = delete;
                LanguageBaseAttributeIDList& operator=(const LanguageBaseAttributeIDList&) = delete;
                ~LanguageBaseAttributeIDList() = default;

            public:
                operator Buffer() const
                {
                    uint8_t scratchPad[256];
                    Payload payload(scratchPad, sizeof(scratchPad), 0);
                    payload.Push(use_descriptor, [this](Payload& sequence) {
                        for (auto const& entry : LanguageBases()) {
                            sequence.Push(use_descriptor, entry.Language());
                            sequence.Push(use_descriptor, entry.Charset());
                            sequence.Push(use_descriptor, entry.Base());
                        }
                    });

                    return (payload);
                }

            public:
                void Add(const uint16_t language, const uint16_t charset, const uint16_t base)
                {
                    TRACE_L5("LanguageBaseAttributeIDList: added 0x%04x 0x%04x 0x%04x", language, charset, base);
                    _languageBases.emplace_back(language, charset, base);
                    _freeBase = base + 3;
                }
                void Add(const string& language, const uint16_t charset, const uint16_t base)
                {
                    ASSERT(language.size() == 2);
                    Add(ToCode(language), charset, base);
                }
                uint16_t Add(const string& language = _T("en"), const uint16_t charset = CHARSET_US_ASCII)
                {
                    const uint16_t base = _freeBase;
                    Add(language, charset, base);
                    return (base);
                }

            public:
                const std::list<Triplet>& LanguageBases() const
                {
                    return (_languageBases);
                }
                uint16_t LanguageBase(const uint16_t language, const uint16_t charset) const
                {
                    auto const& it = std::find_if(_languageBases.cbegin(), _languageBases.cend(),
                                        [&](const Triplet& entry) { return ((entry.Language() == language) && (entry.Charset() == charset)); });

                    if (it != _languageBases.cend()) {
                        return ((*it).Base());
                    } else {
                        return (0);
                    }
                }
                uint16_t LanguageBase(const uint16_t language) const
                {
                    auto const& it = std::find_if(_languageBases.cbegin(), _languageBases.cend(),
                                            [&](const Triplet& entry) { return (entry.Language() == language); });

                    if (it != _languageBases.cend()) {
                        return ((*it).Base());
                    } else {
                        return (0);
                    }
                }
                uint16_t LanguageBase(const string& language, const uint16_t charset) const
                {
                    return (LanguageBase(ToCode(language), charset));
                }
                uint16_t LanguageBase(const string& language) const
                {
                    return (LanguageBase(ToCode(language)));
                }

            private:
                static uint16_t ToCode(const string& language)
                {
                    return ((static_cast<uint16_t>(language[0]) << 8) | language[1]);
                }

            private:
                std::list<Triplet> _languageBases;
                uint16_t _freeBase;
            }; // class LanguageBaseAttributeIDList

            class EXTERNAL ProfileDescriptorList {
            public:
                static constexpr auto type = AttributeDescriptor::ProfileDescriptorList;

            public:
                class Data {
                public:
                    Data() = delete;
                    Data(const Data&) = delete;
                    Data& operator=(const Data&) = delete;

                    Data(const uint16_t version)
                        : _version(version)
                    {
                    }

                    ~Data() = default;

                public:
                    uint16_t Version() const
                    {
                        return (_version);
                    }

                private:
                    uint16_t _version;
                }; // class Data

            public:
                ProfileDescriptorList()
                    : _profiles()
                {
                }
                ProfileDescriptorList(const uint8_t buffer[], const uint16_t size)
                {
                    const Payload payload(buffer, size);
                    payload.Pop(use_descriptor, [&](const Payload& sequence) {
                        while (sequence.Available() > 0) {
                            sequence.Pop(use_descriptor, [&](const Payload& record) {
                                UUID uuid;
                                uint16_t version{};
                                record.Pop(use_descriptor, uuid);
                                record.Pop(use_descriptor, version);
                                Add(uuid, version);
                            });
                        }
                    });
                }
                ProfileDescriptorList(const ProfileDescriptorList&) = delete;
                ProfileDescriptorList& operator=(const ProfileDescriptorList&) = delete;
                ~ProfileDescriptorList() = default;

            public:
                operator Buffer() const
                {
                    uint8_t scratchPad[256];
                    Payload payload(scratchPad, sizeof(scratchPad), 0);
                    payload.Push(use_descriptor, [this](Payload& sequence) {
                        for (auto const& kv : Profiles()) {
                            sequence.Push(use_descriptor, [&kv](Payload& record) {
                                record.Push(use_descriptor, kv.first.Type());
                                record.Push(use_descriptor, kv.second.Version());
                            });
                        }
                    });

                    return (payload);
                }

            public:
                void Add(const ClassID& id, const uint16_t version)
                {
                    TRACE_L5("ProfileDescriptorList: added %s '%s'", id.Type().ToString().c_str(), id.Name().c_str());
                    _profiles.emplace(id, version);
                }
                const std::map<ClassID, Data>& Profiles() const
                {
                    return (_profiles);
                }
                const Data* Profile(const UUID& id) const
                {
                    auto const& it = _profiles.find(id);
                    if (it != _profiles.cend()) {
                        return (&(*it).second);
                    } else {
                        return (nullptr);
                    }
                }

            private:
                std::map<ClassID, Data> _profiles;
            }; // class ProfileDescriptorList
        }; // struct Attribute

    public:
        Service() = delete;
        Service(const Service&) = delete;
        Service& operator=(const Service&) = delete;

        Service(const uint32_t handle)
            : _enabled(false)
            , _serviceRecordHandle(nullptr)
            , _serviceClassIDList(nullptr)
            , _protocolDescriptorList(nullptr)
            , _browseGroupList(nullptr)
            , _languageBaseAttributeIDList(nullptr)
            , _profileDescriptorList(nullptr)
        {
            if (handle != 0) {
                ServiceRecordHandle()->Handle(handle);
            }
        }

        ~Service()
        {
            delete _serviceRecordHandle;
            delete _serviceClassIDList;
            delete _protocolDescriptorList;
            delete _browseGroupList;
            delete _languageBaseAttributeIDList;
            delete _profileDescriptorList;
        }
    public:
        void Deserialize(const uint16_t id, const Buffer& buffer)
        {
            Add(id, buffer, _serviceRecordHandle,
                            _serviceClassIDList,
                            _protocolDescriptorList,
                            _browseGroupList,
                            _languageBaseAttributeIDList,
                            _profileDescriptorList);
        }
        Buffer Serialize(const uint16_t id) const
        {
            return (Descriptor(id, _serviceRecordHandle,
                                   _serviceClassIDList,
                                   _protocolDescriptorList,
                                   _browseGroupList,
                                   _languageBaseAttributeIDList,
                                   _profileDescriptorList));
        }

    public:
        uint32_t Handle() const
        {
            return (ServiceRecordHandle() == nullptr? 0 : ServiceRecordHandle()->Handle());
        }
        bool HasClassID(const UUID& id) const
        {
            return (ServiceClassIDList() == nullptr? false : ServiceClassIDList()->HasID(id));
        }
        bool IsInBrowseGroup(const UUID& id) const
        {
            return (BrowseGroupList() == nullptr? false : BrowseGroupList()->HasID(id));
        }
        const Buffer* Protocol(const UUID& id) const
        {
            return (ProtocolDescriptorList() == nullptr? nullptr : ProtocolDescriptorList()->Protocol(id));
        }
        const Attribute::ProfileDescriptorList::Data* Profile(const UUID& id) const
        {
            return (ProfileDescriptorList() == nullptr? 0 : ProfileDescriptorList()->Profile(id));
        }
        template<typename TYPE, /* if id is enum */ typename std::enable_if<std::is_enum<TYPE>::value, int>::type = 0>
        const Buffer* Attribute(const TYPE id) const
        {
            // enum class is not implicitly convertible to its underlying type
            return (Attribute(static_cast<uint16_t>(id)));
        }
        const Buffer* Attribute(const uint16_t id) const
        {
            auto const& it = std::find_if(_attributes.cbegin(), _attributes.cend(), [&](const AttributeDescriptor& attr) { return (attr.Id() == id); });
            return (it == _attributes.cend()? nullptr : &(*it).Value());
        }
        bool Search(const UUID& id) const
        {
            return ((IsEnabled() == true) && ((HasClassID(id) == true) || (IsInBrowseGroup(id) == true) || (Protocol(id) != nullptr) || (Profile(id) != 0)));
        }
        string Name() const
        {
            string name{};

            if (LanguageBaseAttributeIDList() != nullptr) {
                uint16_t lb = LanguageBaseAttributeIDList()->LanguageBase("en", CHARSET_US_ASCII);
                if (lb == 0) {
                    lb = LanguageBaseAttributeIDList()->LanguageBase("en", CHARSET_UTF8);
                }

                if (lb != 0) {
                    const Buffer* buffer = Attribute(lb + AttributeDescriptor::ServiceNameOffset);
                    if (buffer != nullptr) {
                        name = Data::Element<std::string>(*buffer).Value();
                    }
                }
            }

            return (name);
        }
        bool Metadata(string& name, string& description, string& provider, const string& language = _T("en"), const uint16_t charset = CHARSET_US_ASCII) const
        {
            bool result = false;

            if (LanguageBaseAttributeIDList() != nullptr) {
                uint16_t lb = LanguageBaseAttributeIDList()->LanguageBase(language, charset);
                if (lb != 0) {
                    const Buffer* buffer = Attribute(lb + AttributeDescriptor::ServiceNameOffset);
                    if (buffer != nullptr) {
                        name = Data::Element<std::string>(*buffer).Value();
                    }

                    buffer = Attribute(lb + AttributeDescriptor::ServiceDescriptionOffset);
                    if (buffer != nullptr) {
                        description = Data::Element<std::string>(*buffer).Value();
                    }

                    buffer = Attribute(lb + AttributeDescriptor::ProviderNameOffset);
                    if (buffer != nullptr) {
                        provider = Data::Element<std::string>(*buffer).Value();
                    }

                    result = true;
                }
            }

            return (result);
        }

    public:
        void Enable(const bool enable)
        {
            _enabled = enable;
        }
        bool IsEnabled() const
        {
            return (_enabled);
        }
        Attribute::ServiceRecordHandle* ServiceRecordHandle()
        {
            if (_serviceRecordHandle == nullptr) {
                _serviceRecordHandle = new Attribute::ServiceRecordHandle();
                ASSERT(_serviceRecordHandle != nullptr);
            }
            return (_serviceRecordHandle);
        }
        const Attribute::ServiceRecordHandle* ServiceRecordHandle() const
        {
            return (_serviceRecordHandle);
        }
        Attribute::ServiceClassIDList* ServiceClassIDList()
        {
            if (_serviceClassIDList == nullptr) {
                _serviceClassIDList = new Attribute::ServiceClassIDList();
                ASSERT(_serviceClassIDList != nullptr);
            }
            return (_serviceClassIDList);
        }
        const Attribute::ServiceClassIDList* ServiceClassIDList() const
        {
            return (_serviceClassIDList);
        }
        Attribute::ProtocolDescriptorList* ProtocolDescriptorList()
        {
            if (_protocolDescriptorList == nullptr) {
                _protocolDescriptorList = new Attribute::ProtocolDescriptorList();
                ASSERT(_protocolDescriptorList != nullptr);
            }
            return (_protocolDescriptorList);
        }
        const Attribute::ProtocolDescriptorList* ProtocolDescriptorList() const
        {
            return (_protocolDescriptorList);
        }
        Attribute::BrowseGroupList* BrowseGroupList()
        {
            if (_browseGroupList == nullptr) {
                _browseGroupList = new Attribute::BrowseGroupList();
                ASSERT(_browseGroupList != nullptr);
            }
            return (_browseGroupList);
        }
        const Attribute::BrowseGroupList* BrowseGroupList() const
        {
            return (_browseGroupList);
        }
        Attribute::LanguageBaseAttributeIDList* LanguageBaseAttributeIDList()
        {
            if (_languageBaseAttributeIDList == nullptr) {
                _languageBaseAttributeIDList = new Attribute::LanguageBaseAttributeIDList();
                ASSERT(_languageBaseAttributeIDList != nullptr);
            }
            return (_languageBaseAttributeIDList);
        }
        const Attribute::LanguageBaseAttributeIDList* LanguageBaseAttributeIDList() const
        {
            return (_languageBaseAttributeIDList);
        }
        Attribute::ProfileDescriptorList* ProfileDescriptorList()
        {
            if (_profileDescriptorList == nullptr) {
                _profileDescriptorList = new Attribute::ProfileDescriptorList();
                ASSERT(_profileDescriptorList != nullptr);
            }
            return (_profileDescriptorList);
        }
        const Attribute::ProfileDescriptorList* ProfileDescriptorList() const
        {
            return (_profileDescriptorList);
        }

    public:
        std::set<AttributeDescriptor>& Attributes()
        {
            return (_attributes);
        }
        const std::set<AttributeDescriptor>& Attributes() const
        {
            return (_attributes);
        }

    public:
        void Add(const uint16_t id, const Buffer& buffer)
        {
            if (buffer.empty() == false) {
                _attributes.emplace(id, buffer);
            }
        }
        template<typename TYPE, /* if id is enum */ typename std::enable_if<std::is_enum<TYPE>::value, int>::type = 0>
        void Add(const TYPE id, const Buffer& buffer)
        {
            // enum class is not implicitly convertible to its underlying type
            Add(static_cast<uint16_t>(id), buffer);
        }
        template<typename ...Args>
        void Description(const std::string& name, const std::string& description = {}, const std::string& provider = {}, Args... args)
        {
            uint16_t id = LanguageBaseAttributeIDList()->Add(args...);

            if (name.empty() == false) {
                Add(id + AttributeDescriptor::ServiceNameOffset, Data::Element<std::string>(name));
            }

            if (description.empty() == false) {
                Add(id + AttributeDescriptor::ServiceDescriptionOffset, Data::Element<std::string>(description));
            }

            if (provider.empty() == false) {
                Add(id + AttributeDescriptor::ProviderNameOffset, Data::Element<std::string>(provider));
            }
        }

    private:
        template<typename T, typename ...Ts>
        void Add(const uint16_t id, const Buffer& buffer, T& attribute, Ts&... attributes)
        {
            if (buffer.size() != 0) {
                using Attribute = typename std::remove_pointer<T>::type;

                if (id == Attribute::type) {
                    if (attribute == nullptr) {
                        attribute = new Attribute(buffer.data(), buffer.size());
                        ASSERT(attribute != nullptr);

#ifdef __DEBUG__
                        // In debug, store all the attributes in raw form for inspection.
                        Add(id, buffer);
#endif
                    }
                } else {
                    Add(id, buffer, attributes...);
                    // Once the attribute list for deserialisation is recursively exhausted
                    // this will fall back to storing raw attributes.
                }
            }
        }

    private:
        template<typename T, typename ...Ts>
        Buffer Descriptor(const uint16_t id, const T attribute, const Ts... attributes) const
        {
            using Attribute = typename std::remove_pointer<T>::type;

            if (id == Attribute::type) {
                return (attribute != nullptr? *attribute : Buffer{});
            } else {
                return Descriptor(id, attributes...);
            }
        }
        Buffer Descriptor(const uint16_t) const
        {
            // Just to stop the recursive attribute serialisation.
            return {};
        }

    private:
        bool _enabled;
        Attribute::ServiceRecordHandle* _serviceRecordHandle;
        Attribute::ServiceClassIDList* _serviceClassIDList;
        Attribute::ProtocolDescriptorList* _protocolDescriptorList;
        Attribute::BrowseGroupList* _browseGroupList;
        Attribute::LanguageBaseAttributeIDList* _languageBaseAttributeIDList;
        Attribute::ProfileDescriptorList* _profileDescriptorList;
        std::set<AttributeDescriptor> _attributes;
    }; // class Service

    class EXTERNAL Tree {
    public:
        Tree(const Tree&) = delete;
        Tree& operator=(const Tree&) = delete;
        ~Tree() = default;

        Tree()
            : _services()
        {
        }

    public:
        const std::list<Service>& Services() const
        {
            return (_services);
        }
        const SDP::Service* Find(const uint32_t handle) const
        {
            auto const& it = std::find_if(_services.cbegin(), _services.cend(), [&](const Service& s) { return (s.Handle() == handle); });
            return (it == _services.cend()? nullptr : &(*it));
        }
        Service& Add(const uint32_t handle = 0)
        {
            _services.emplace_back(handle == 0? (0x10000 + _services.size()) : handle);
            Service& added = _services.back();
            return (added);
        }

    protected:
        std::list<Service> _services;
    }; // class Tree

    class EXTERNAL Client {
    public:
        Client() = delete;
        Client(const Client&) = delete;
        Client& operator=(const Client&) = delete;
        ~Client() = default;

        Client(ClientSocket& socket)
            : _socket(socket)
        {
        }

    public:
        uint32_t Discover(const std::list<UUID>& services, Tree& tree) const;

    public:
        uint32_t ServiceSearch(const std::list<UUID>& services, std::vector<uint32_t>& outHandles) const;

        uint32_t ServiceAttribute(const uint32_t serviceHandle,
                                  const std::list<uint32_t>& attributeIdRanges,
                                  std::list<std::pair<uint32_t, Buffer>>& outAttributes) const;

        uint32_t ServiceSearchAttribute(const std::list<UUID>& services,
                                        const std::list<uint32_t>& attributeIdRanges,
                                        std::list<std::pair<uint32_t, Buffer>>& outAttributes) const;

    private:
        uint32_t InternalServiceAttribute(const PDU::pduid id,
                                          const std::function<void(Payload&)>& buildCb,
                                          const std::list<uint32_t>& attributeIdRanges,
                                          std::list<std::pair<uint32_t, Buffer>>& outAttributes) const;

    private:
        uint32_t Execute(ClientSocket::Command& cmd, const Payload::Inspector& inspectorCb = nullptr) const;

    private:
        uint16_t Capacity(const PDU& pdu) const
        {
            const uint16_t bufferSize = std::min<uint16_t>(pdu.Capacity(), (_socket.InputMTU() - PDU::HeaderSize));
            ASSERT(bufferSize >= (1 + PDU::MaxContinuationSize));

            return (bufferSize - (1 + PDU::MaxContinuationSize));
        }

    private:
        ClientSocket& _socket;
    }; // class Client

    class EXTERNAL Server {
        using Handler = ServerSocket::ResponseHandler;

    public:
        Server() = default;
        Server(const Server&) = delete;
        Server& operator=(const Server&) = delete;
        virtual ~Server() = default;

    public:
        virtual void WithServiceTree(const std::function<void(const Tree&)>& inspectCb) = 0;

    public:
        void OnPDU(const ServerSocket& socket, const PDU& pdu, const Handler& handler)
        {
            switch (pdu.Type()) {
            case PDU::ServiceSearchRequest:
                OnServiceSearchRequest(socket, pdu, handler);
                break;
            case PDU::ServiceAttributeRequest:
                OnServiceAttributeRequest(socket, pdu, handler);
                break;
            case PDU::ServiceSearchAttributeRequest:
                OnServiceSearchAttributeRequest(socket, pdu, handler);
                break;
            default:
                TRACE_L1("SDP server: Usupported PDU %d", pdu.Type());
                handler(PDU::InvalidRequestSyntax);
                break;
            }
        }

    private:
        void OnServiceSearchRequest(const ServerSocket& socket, const PDU& pdu, const Handler& handler);
        void OnServiceAttributeRequest(const ServerSocket& socket, const PDU& pdu, const Handler& handler);
        void OnServiceSearchAttributeRequest(const ServerSocket& socket, const PDU& pdu, const Handler& handler);

    private:
        void InternalOnServiceSearchAttributeRequest(const PDU::pduid id,
                                                     const std::function<PDU::errorid(const Payload&, std::list<uint32_t>&)>& inspectCb,
                                                     const ServerSocket& socket,
                                                     const PDU& request,
                                                     const Handler& handlerCb);

        std::map<uint16_t, Buffer> SerializeAttributesByRange(const Service& service, const std::list<uint32_t>& attributeRanges, 
                                                              const uint16_t offset, uint16_t& count) const;
 

    private:
        uint16_t Capacity(const ServerSocket& socket, const PDU& pdu) const
        {
            const uint16_t bufferSize = std::min<uint16_t>(pdu.Capacity(), (socket.OutputMTU() - PDU::HeaderSize));
            ASSERT(bufferSize >= (1 + PDU::MaxContinuationSize));

            return (bufferSize - (1 + PDU::MaxContinuationSize));
        }

    }; // class Server

} // namespace SDP

} // namespace Bluetooth

}