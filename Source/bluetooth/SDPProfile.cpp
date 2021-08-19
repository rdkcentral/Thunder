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

#include "Module.h"
#include "SDPProfile.h"

namespace WPEFramework {

ENUM_CONVERSION_BEGIN(Bluetooth::SDPProfile::ClassID::id)
    { Bluetooth::SDPProfile::ClassID::Undefined,   _TXT("(undefined)") },
    // Protocols
    { Bluetooth::SDPProfile::ClassID::SDP,         _TXT("SDP") },
    { Bluetooth::SDPProfile::ClassID::UDP,         _TXT("UDP") },
    { Bluetooth::SDPProfile::ClassID::RFCOMM,      _TXT("RFCOMM") },
    { Bluetooth::SDPProfile::ClassID::TCP,         _TXT("TCP") },
    { Bluetooth::SDPProfile::ClassID::TCS_BIN,     _TXT("TCS-Binary") },
    { Bluetooth::SDPProfile::ClassID::TCS_AT,      _TXT("TCS-AT") },
    { Bluetooth::SDPProfile::ClassID::ATT,         _TXT("ATT") },
    { Bluetooth::SDPProfile::ClassID::OBEX,        _TXT("OBEX") },
    { Bluetooth::SDPProfile::ClassID::IP,          _TXT("IP") },
    { Bluetooth::SDPProfile::ClassID::FTP,         _TXT("FTP") },
    { Bluetooth::SDPProfile::ClassID::HTTP,        _TXT("HTTP") },
    { Bluetooth::SDPProfile::ClassID::WSP,         _TXT("WSP") },
    { Bluetooth::SDPProfile::ClassID::BNEP,        _TXT("BNEP") },
    { Bluetooth::SDPProfile::ClassID::UPNP,        _TXT("UPNP/ESDP") },
    { Bluetooth::SDPProfile::ClassID::HIDP,        _TXT("HIDP") },
    { Bluetooth::SDPProfile::ClassID::HCRP_CTRL,   _TXT("HCRP-ControlChannel") },
    { Bluetooth::SDPProfile::ClassID::HCRP_DATA,   _TXT("HCRP-DataChannel") },
    { Bluetooth::SDPProfile::ClassID::HCRP_NOTE,   _TXT("HCRP-Notification") },
    { Bluetooth::SDPProfile::ClassID::AVCTP,       _TXT("AVCTP") },
    { Bluetooth::SDPProfile::ClassID::AVDTP,       _TXT("AVDTP") },
    { Bluetooth::SDPProfile::ClassID::CMTP,        _TXT("CMTP") },
    { Bluetooth::SDPProfile::ClassID::UDI,         _TXT("UDI") },
    { Bluetooth::SDPProfile::ClassID::MCAP_CTRL,   _TXT("MCAP-ControlChannel") },
    { Bluetooth::SDPProfile::ClassID::MCAP_DATA,   _TXT("MCAP-DataChannel") },
    { Bluetooth::SDPProfile::ClassID::L2CAP,       _TXT("L2CAP") },
    // Services and Profiles
    { Bluetooth::SDPProfile::ClassID::SerialPort,                       _TXT("Serial Port (SPP)") },
    { Bluetooth::SDPProfile::ClassID::LANAccessUsingPPP,                _TXT("LAN Access Using PPP (LAP)") },
    { Bluetooth::SDPProfile::ClassID::DialupNetworking,                 _TXT("Dial-Up Networking (DUN)") },
    { Bluetooth::SDPProfile::ClassID::IrMCSync,                         _TXT("IrMC Sync (SYNCH)") },
    { Bluetooth::SDPProfile::ClassID::OBEXObjectPush,                   _TXT("OBEX Obect Push (OPP)") },
    { Bluetooth::SDPProfile::ClassID::OBEXFileTransfer,                 _TXT("OBEX File Transfer (FTP)") },
    { Bluetooth::SDPProfile::ClassID::IrMCSyncCommand,                  _TXT("IrMC Sync Command (SYNCH)") },
    { Bluetooth::SDPProfile::ClassID::HeadsetHSP,                       _TXT("Headset (HSP)") },
    { Bluetooth::SDPProfile::ClassID::CordlessTelephony,                _TXT("Cordless Telephony (CTP)") },
    { Bluetooth::SDPProfile::ClassID::AudioSource,                      _TXT("Audio Source (A2DP)") },
    { Bluetooth::SDPProfile::ClassID::AudioSink,                        _TXT("Audio Sink (A2DP)") },
    { Bluetooth::SDPProfile::ClassID::AVRemoteControlTarget,            _TXT("A/V Remote Control Target (AVRCP)") },
    { Bluetooth::SDPProfile::ClassID::AdvancedAudioDistribution,        _TXT("Advanced Audio Distribution (A2DP)") },
    { Bluetooth::SDPProfile::ClassID::AVRemoteControl,                  _TXT("A/V Remote Control (AVRCP)") },
    { Bluetooth::SDPProfile::ClassID::AVRemoteControlController,        _TXT("A/V Remote Control Controller (AVRCP)") },
    { Bluetooth::SDPProfile::ClassID::Intercom,                         _TXT("Intercom (ICP)") },
    { Bluetooth::SDPProfile::ClassID::Fax,                              _TXT("Fax (FAX)") },
    { Bluetooth::SDPProfile::ClassID::HeadsetAudioGateway,              _TXT("Headset Audio Gateway (HSP)") },
    { Bluetooth::SDPProfile::ClassID::WAP,                              _TXT("WAP (WAPB)") },
    { Bluetooth::SDPProfile::ClassID::WAPClient,                        _TXT("WAP Client (WAPB)") },
    { Bluetooth::SDPProfile::ClassID::PANU,                             _TXT("PAN User (PAN)") },
    { Bluetooth::SDPProfile::ClassID::NAP,                              _TXT("Network Access Point (PAN)") },
    { Bluetooth::SDPProfile::ClassID::GN,                               _TXT("Group Ad-hoc Network (PAN)") },
    { Bluetooth::SDPProfile::ClassID::DirectPrinting,                   _TXT("Direct Printing (BPP)") },
    { Bluetooth::SDPProfile::ClassID::ReferencePrinting,                _TXT("Reference Printing (BPP)") },
    { Bluetooth::SDPProfile::ClassID::BasicImagingProfile,              _TXT("Basic Imaging (BIP)") },
    { Bluetooth::SDPProfile::ClassID::ImagingResponder,                 _TXT("Imaging Responder (BIP)") },
    { Bluetooth::SDPProfile::ClassID::ImagingAutomaticArchive,          _TXT("Imaging Automatic Archive (BIP)") },
    { Bluetooth::SDPProfile::ClassID::ImagingReferencedObjects,         _TXT("Imaging Referenced Objects (BIP)") },
    { Bluetooth::SDPProfile::ClassID::Handsfree,                        _TXT("Hands-Free (HFP)") },
    { Bluetooth::SDPProfile::ClassID::HandsfreeAudioGateway,            _TXT("Hands-Free Audio Gateway (HFP)") },
    { Bluetooth::SDPProfile::ClassID::DirectPrintingReferenceObjects,   _TXT("Direct Printing Reference Objects (BPP)") },
    { Bluetooth::SDPProfile::ClassID::ReflectedUI,                      _TXT("Reflected UI (BPP)") },
    { Bluetooth::SDPProfile::ClassID::BasicPrinting,                    _TXT("Basic Printing (BPP)") },
    { Bluetooth::SDPProfile::ClassID::PrintingStatus,                   _TXT("Printing Status (BPP)") },
    { Bluetooth::SDPProfile::ClassID::HumanInterfaceDeviceService,      _TXT("Human Interface Device (HID)") },
    { Bluetooth::SDPProfile::ClassID::HardcopyCableReplacement,         _TXT("Hardcopy Cable Replacement (HCRP)") },
    { Bluetooth::SDPProfile::ClassID::HCRPrint,                         _TXT("HCRP Print (HCRP) ") },
    { Bluetooth::SDPProfile::ClassID::HCRScan,                          _TXT("HCRP Scan (HCRP)") },
    { Bluetooth::SDPProfile::ClassID::CommonISDNAccess,                 _TXT("Common ISDN Access (CIP)") },
    { Bluetooth::SDPProfile::ClassID::SIMAccess,                        _TXT("SIM Access (SAP)") },
    { Bluetooth::SDPProfile::ClassID::PhonebookAccessPCE,               _TXT("Phonebook Client Equipment (PBAP)") },
    { Bluetooth::SDPProfile::ClassID::PhonebookAccessPSE,               _TXT("Phonebook Server Equipment (PBAP)") },
    { Bluetooth::SDPProfile::ClassID::PhonebookAccess,                  _TXT("Phonebook Access (PBAP)") },
    { Bluetooth::SDPProfile::ClassID::HeadsetHS,                        _TXT("Headset v1.2 (HSP)") },
    { Bluetooth::SDPProfile::ClassID::MessageAccessServer,              _TXT("Message Access Server (MAP)") },
    { Bluetooth::SDPProfile::ClassID::MessageNotificationServer,        _TXT("Message Notification Server (MAP)") },
    { Bluetooth::SDPProfile::ClassID::MessageAccess,                    _TXT("Message Access(MAP)") },
    { Bluetooth::SDPProfile::ClassID::GNSS,                             _TXT("Global Navigation Satellite System (GNSS)") },
    { Bluetooth::SDPProfile::ClassID::GNSSServer,                       _TXT("GNSS Server (GNSS)") },
    { Bluetooth::SDPProfile::ClassID::ThreeDDisplay,                    _TXT("3D Display (3DSP)") },
    { Bluetooth::SDPProfile::ClassID::ThreeDGlasses,                    _TXT("3D Glasses (3DSP)") },
    { Bluetooth::SDPProfile::ClassID::ThreeDSynchronisation,            _TXT("3D Synchronisation (3DSP)") },
    { Bluetooth::SDPProfile::ClassID::MPS,                              _TXT("Multi-Profile Specification (MPS)") },
    { Bluetooth::SDPProfile::ClassID::MPSSC,                            _TXT("MPS SC (MPS)") },
    { Bluetooth::SDPProfile::ClassID::CTNAccessService,                 _TXT("CTN Access (CTN)") },
    { Bluetooth::SDPProfile::ClassID::CTNNotificationService,           _TXT("CTN Notification (CTN)") },
    { Bluetooth::SDPProfile::ClassID::CTN,                              _TXT("Calendar Tasks and Notes (CTN)") },
    { Bluetooth::SDPProfile::ClassID::PnPInformation,                   _TXT("PnP Information (DID") },
    { Bluetooth::SDPProfile::ClassID::GenericNetworking,                _TXT("Generic Networking") },
    { Bluetooth::SDPProfile::ClassID::GenericFileTransfer,              _TXT("Generic File Transfer") },
    { Bluetooth::SDPProfile::ClassID::GenericAudio,                     _TXT("Generic Audio") },
    { Bluetooth::SDPProfile::ClassID::GenericTelephony,                 _TXT("Generic Telephony") },
    { Bluetooth::SDPProfile::ClassID::UPNPService,                      _TXT("UPNP (ESDP)") },
    { Bluetooth::SDPProfile::ClassID::UPNPIPService,                    _TXT("UPNP IP (ESDP)") },
    { Bluetooth::SDPProfile::ClassID::ESDPUPNPIPPAN,                    _TXT("UPNP IP PAN (ESDP)") },
    { Bluetooth::SDPProfile::ClassID::ESDPUPNPIPLAP,                    _TXT("UPNP IP LAP (ESDP)") },
    { Bluetooth::SDPProfile::ClassID::ESDPUPNPL2CAP,                    _TXT("UPNP L2CAP (ESDP)") },
    { Bluetooth::SDPProfile::ClassID::VideoSource,                      _TXT("Video Source (VDP)") },
    { Bluetooth::SDPProfile::ClassID::VideoSink,                        _TXT("Video Sink (VDP)") },
    { Bluetooth::SDPProfile::ClassID::VideoDistribution,                _TXT("Video Distribution Profile (VDP)") },
    { Bluetooth::SDPProfile::ClassID::HDP,                              _TXT("Health Device (HDP)") },
    { Bluetooth::SDPProfile::ClassID::HDPSource,                        _TXT("HDP Source (HDP)") },
    { Bluetooth::SDPProfile::ClassID::HDPSink,                          _TXT("HDP Sink (HDP)") },
ENUM_CONVERSION_END(Bluetooth::SDPProfile::ClassID::id)

ENUM_CONVERSION_BEGIN(Bluetooth::SDPProfile::Service::AttributeDescriptor::id)
    { Bluetooth::SDPProfile::Service::AttributeDescriptor::ServiceRecordHandle,             _TXT("ServiceRecordHandle") },
    { Bluetooth::SDPProfile::Service::AttributeDescriptor::ServiceClassIDList,              _TXT("ServiceClassIDList") },
    { Bluetooth::SDPProfile::Service::AttributeDescriptor::ServiceRecordState,              _TXT("ServiceRecordState") },
    { Bluetooth::SDPProfile::Service::AttributeDescriptor::ServiceID,                       _TXT("ServiceID") },
    { Bluetooth::SDPProfile::Service::AttributeDescriptor::ProtocolDescriptorList,          _TXT("ProtocolDescriptorList") },
    { Bluetooth::SDPProfile::Service::AttributeDescriptor::BrowseGroupList,                 _TXT("BrowseGroupList") },
    { Bluetooth::SDPProfile::Service::AttributeDescriptor::LanguageBaseAttributeIDList,     _TXT("LanguageBaseAttributeIDList") },
    { Bluetooth::SDPProfile::Service::AttributeDescriptor::ServiceInfoTimeToLive,           _TXT("ServiceInfoTimeToLive") },
    { Bluetooth::SDPProfile::Service::AttributeDescriptor::ServiceAvailability,             _TXT("ServiceAvailability") },
    { Bluetooth::SDPProfile::Service::AttributeDescriptor::BluetoothProfileDescriptorList,  _TXT("BluetoothProfileDescriptorList") },
    { Bluetooth::SDPProfile::Service::AttributeDescriptor::DocumentationURL,                _TXT("DocumentationURL") },
    { Bluetooth::SDPProfile::Service::AttributeDescriptor::ClientExecutableURL,             _TXT("ClientExecutableURL") },
    { Bluetooth::SDPProfile::Service::AttributeDescriptor::IconURL,                         _TXT("IconURL") },
ENUM_CONVERSION_END(Bluetooth::SDPProfile::Service::AttributeDescriptor::id)

namespace Bluetooth {

    void SDPProfile::Service::DeserializeAttributes()
    {
        for (auto const& attr : _attributes) {
            const uint16_t& id = attr.first;
            const SDPSocket::Payload value(attr.second.Value());

            // Lets deserialize some of the universal attributes...
            switch (static_cast<AttributeDescriptor::id>(id)) {
            case AttributeDescriptor::ServiceRecordHandle:
                value.Pop(SDPSocket::use_descriptor, _handle);
                break;
            case AttributeDescriptor::id::ServiceClassIDList:
                value.Pop(SDPSocket::use_descriptor, [&](const SDPSocket::Payload& sequence) {
                    while (sequence.Available()) {
                        UUID uuid;
                        sequence.Pop(SDPSocket::use_descriptor, uuid);
                        _classes.emplace_back(uuid);
                    }
                });
                break;
            case AttributeDescriptor::BluetoothProfileDescriptorList:
                value.Pop(SDPSocket::use_descriptor, [&](const SDPSocket::Payload& sequence) {
                    while (sequence.Available()) {
                        sequence.Pop(SDPSocket::use_descriptor, [&](const SDPSocket::Payload& descriptor) {
                            UUID uuid;
                            uint16_t version = 0;
                            descriptor.Pop(SDPSocket::use_descriptor, uuid);
                            descriptor.Pop(SDPSocket::use_descriptor, version);
                            _profiles.emplace_back(uuid, version);
                        });
                    }
                });
                break;
            case AttributeDescriptor::ProtocolDescriptorList:
                value.Pop(SDPSocket::use_descriptor, [&](const SDPSocket::Payload& sequence) {
                    while (sequence.Available()) {
                        sequence.Pop(SDPSocket::use_descriptor, [&](const SDPSocket::Payload& descriptor) {
                            UUID uuid;
                            Buffer params;
                            descriptor.Pop(SDPSocket::use_descriptor, uuid);
                            descriptor.Pop(SDPSocket::use_descriptor, params);
                            _protocols.emplace_back(uuid, params);
                        });
                    }
                });
                break;
            case AttributeDescriptor::LanguageBaseAttributeIDList:
                value.Pop(SDPSocket::use_descriptor, [&](const SDPSocket::Payload& sequence) {
                    while (sequence.Available() >= 9 /* sizeof language descriptor triplet */) {
                        uint16_t lang = 0;
                        uint16_t charset = 0;
                        uint16_t base = 0;

                        sequence.Pop(SDPSocket::use_descriptor, lang); // TODO: Take lang and charset into consideration...?
                        sequence.Pop(SDPSocket::use_descriptor, charset);
                        sequence.Pop(SDPSocket::use_descriptor, base);

                        auto FetchString = [&](const uint16_t offset, string& output) {
                            auto it = _attributes.find(base + offset);
                            if (it != _attributes.end()) {
                                SDPSocket::Payload str((*it).second.Value());
                                str.Pop(SDPSocket::use_descriptor, output);
                            }
                        };

                        string name;
                        FetchString(AttributeDescriptor::OFFSET_ServiceName, name);

                        string desc;
                        FetchString(AttributeDescriptor::OFFSET_ServiceDescription, desc);

                        string provider;
                        FetchString(AttributeDescriptor::OFFSET_ProviderName, desc);

                        _metadatas.emplace_back(lang, charset, name, desc, provider);
                    }
                });
                break;
            default:
                break;
            }
        }
    }

}

}