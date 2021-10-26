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

#include "Module.h"
#include "SDPProfile.h"

namespace WPEFramework {

ENUM_CONVERSION_BEGIN(Bluetooth::SDP::ClassID::id)
    { Bluetooth::SDP::ClassID::Undefined,   _TXT("(undefined)") },
    // Protocols
    { Bluetooth::SDP::ClassID::SDP,         _TXT("SDP") },
    { Bluetooth::SDP::ClassID::UDP,         _TXT("UDP") },
    { Bluetooth::SDP::ClassID::RFCOMM,      _TXT("RFCOMM") },
    { Bluetooth::SDP::ClassID::TCP,         _TXT("TCP") },
    { Bluetooth::SDP::ClassID::TCS_BIN,     _TXT("TCS-Binary") },
    { Bluetooth::SDP::ClassID::TCS_AT,      _TXT("TCS-AT") },
    { Bluetooth::SDP::ClassID::ATT,         _TXT("ATT") },
    { Bluetooth::SDP::ClassID::OBEX,        _TXT("OBEX") },
    { Bluetooth::SDP::ClassID::IP,          _TXT("IP") },
    { Bluetooth::SDP::ClassID::FTP,         _TXT("FTP") },
    { Bluetooth::SDP::ClassID::HTTP,        _TXT("HTTP") },
    { Bluetooth::SDP::ClassID::WSP,         _TXT("WSP") },
    { Bluetooth::SDP::ClassID::BNEP,        _TXT("BNEP") },
    { Bluetooth::SDP::ClassID::UPNP,        _TXT("UPNP/ESDP") },
    { Bluetooth::SDP::ClassID::HIDP,        _TXT("HIDP") },
    { Bluetooth::SDP::ClassID::HCRP_CTRL,   _TXT("HCRP-ControlChannel") },
    { Bluetooth::SDP::ClassID::HCRP_DATA,   _TXT("HCRP-DataChannel") },
    { Bluetooth::SDP::ClassID::HCRP_NOTE,   _TXT("HCRP-Notification") },
    { Bluetooth::SDP::ClassID::AVCTP,       _TXT("AVCTP") },
    { Bluetooth::SDP::ClassID::AVDTP,       _TXT("AVDTP") },
    { Bluetooth::SDP::ClassID::CMTP,        _TXT("CMTP") },
    { Bluetooth::SDP::ClassID::UDI,         _TXT("UDI") },
    { Bluetooth::SDP::ClassID::MCAP_CTRL,   _TXT("MCAP-ControlChannel") },
    { Bluetooth::SDP::ClassID::MCAP_DATA,   _TXT("MCAP-DataChannel") },
    { Bluetooth::SDP::ClassID::L2CAP,       _TXT("L2CAP") },
    // SDP services
    { Bluetooth::SDP::ClassID::ServiceDiscoveryServer,           _TXT("Service Discovery Server") },
    { Bluetooth::SDP::ClassID::BrowseGroupDescriptor,            _TXT("Browse Group Descriptor") },
    { Bluetooth::SDP::ClassID::PublicBrowseRoot,                 _TXT("Public Browse Root") },
    // Services and Profiles
    { Bluetooth::SDP::ClassID::SerialPort,                       _TXT("Serial Port (SPP)") },
    { Bluetooth::SDP::ClassID::LANAccessUsingPPP,                _TXT("LAN Access Using PPP (LAP)") },
    { Bluetooth::SDP::ClassID::DialupNetworking,                 _TXT("Dial-Up Networking (DUN)") },
    { Bluetooth::SDP::ClassID::IrMCSync,                         _TXT("IrMC Sync (SYNCH)") },
    { Bluetooth::SDP::ClassID::OBEXObjectPush,                   _TXT("OBEX Obect Push (OPP)") },
    { Bluetooth::SDP::ClassID::OBEXFileTransfer,                 _TXT("OBEX File Transfer (FTP)") },
    { Bluetooth::SDP::ClassID::IrMCSyncCommand,                  _TXT("IrMC Sync Command (SYNCH)") },
    { Bluetooth::SDP::ClassID::HeadsetHSP,                       _TXT("Headset (HSP)") },
    { Bluetooth::SDP::ClassID::CordlessTelephony,                _TXT("Cordless Telephony (CTP)") },
    { Bluetooth::SDP::ClassID::AudioSource,                      _TXT("Audio Source (A2DP)") },
    { Bluetooth::SDP::ClassID::AudioSink,                        _TXT("Audio Sink (A2DP)") },
    { Bluetooth::SDP::ClassID::AVRemoteControlTarget,            _TXT("A/V Remote Control Target (AVRCP)") },
    { Bluetooth::SDP::ClassID::AdvancedAudioDistribution,        _TXT("Advanced Audio Distribution (A2DP)") },
    { Bluetooth::SDP::ClassID::AVRemoteControl,                  _TXT("A/V Remote Control (AVRCP)") },
    { Bluetooth::SDP::ClassID::AVRemoteControlController,        _TXT("A/V Remote Control Controller (AVRCP)") },
    { Bluetooth::SDP::ClassID::Intercom,                         _TXT("Intercom (ICP)") },
    { Bluetooth::SDP::ClassID::Fax,                              _TXT("Fax (FAX)") },
    { Bluetooth::SDP::ClassID::HeadsetAudioGateway,              _TXT("Headset Audio Gateway (HSP)") },
    { Bluetooth::SDP::ClassID::WAP,                              _TXT("WAP (WAPB)") },
    { Bluetooth::SDP::ClassID::WAPClient,                        _TXT("WAP Client (WAPB)") },
    { Bluetooth::SDP::ClassID::PANU,                             _TXT("PAN User (PAN)") },
    { Bluetooth::SDP::ClassID::NAP,                              _TXT("Network Access Point (PAN)") },
    { Bluetooth::SDP::ClassID::GN,                               _TXT("Group Ad-hoc Network (PAN)") },
    { Bluetooth::SDP::ClassID::DirectPrinting,                   _TXT("Direct Printing (BPP)") },
    { Bluetooth::SDP::ClassID::ReferencePrinting,                _TXT("Reference Printing (BPP)") },
    { Bluetooth::SDP::ClassID::BasicImagingProfile,              _TXT("Basic Imaging (BIP)") },
    { Bluetooth::SDP::ClassID::ImagingResponder,                 _TXT("Imaging Responder (BIP)") },
    { Bluetooth::SDP::ClassID::ImagingAutomaticArchive,          _TXT("Imaging Automatic Archive (BIP)") },
    { Bluetooth::SDP::ClassID::ImagingReferencedObjects,         _TXT("Imaging Referenced Objects (BIP)") },
    { Bluetooth::SDP::ClassID::Handsfree,                        _TXT("Hands-Free (HFP)") },
    { Bluetooth::SDP::ClassID::HandsfreeAudioGateway,            _TXT("Hands-Free Audio Gateway (HFP)") },
    { Bluetooth::SDP::ClassID::DirectPrintingReferenceObjects,   _TXT("Direct Printing Reference Objects (BPP)") },
    { Bluetooth::SDP::ClassID::ReflectedUI,                      _TXT("Reflected UI (BPP)") },
    { Bluetooth::SDP::ClassID::BasicPrinting,                    _TXT("Basic Printing (BPP)") },
    { Bluetooth::SDP::ClassID::PrintingStatus,                   _TXT("Printing Status (BPP)") },
    { Bluetooth::SDP::ClassID::HumanInterfaceDeviceService,      _TXT("Human Interface Device (HID)") },
    { Bluetooth::SDP::ClassID::HardcopyCableReplacement,         _TXT("Hardcopy Cable Replacement (HCRP)") },
    { Bluetooth::SDP::ClassID::HCRPrint,                         _TXT("HCRP Print (HCRP) ") },
    { Bluetooth::SDP::ClassID::HCRScan,                          _TXT("HCRP Scan (HCRP)") },
    { Bluetooth::SDP::ClassID::CommonISDNAccess,                 _TXT("Common ISDN Access (CIP)") },
    { Bluetooth::SDP::ClassID::SIMAccess,                        _TXT("SIM Access (SAP)") },
    { Bluetooth::SDP::ClassID::PhonebookAccessPCE,               _TXT("Phonebook Client Equipment (PBAP)") },
    { Bluetooth::SDP::ClassID::PhonebookAccessPSE,               _TXT("Phonebook Server Equipment (PBAP)") },
    { Bluetooth::SDP::ClassID::PhonebookAccess,                  _TXT("Phonebook Access (PBAP)") },
    { Bluetooth::SDP::ClassID::HeadsetHS,                        _TXT("Headset v1.2 (HSP)") },
    { Bluetooth::SDP::ClassID::MessageAccessServer,              _TXT("Message Access Server (MAP)") },
    { Bluetooth::SDP::ClassID::MessageNotificationServer,        _TXT("Message Notification Server (MAP)") },
    { Bluetooth::SDP::ClassID::MessageAccess,                    _TXT("Message Access(MAP)") },
    { Bluetooth::SDP::ClassID::GNSS,                             _TXT("Global Navigation Satellite System (GNSS)") },
    { Bluetooth::SDP::ClassID::GNSSServer,                       _TXT("GNSS Server (GNSS)") },
    { Bluetooth::SDP::ClassID::ThreeDDisplay,                    _TXT("3D Display (3DSP)") },
    { Bluetooth::SDP::ClassID::ThreeDGlasses,                    _TXT("3D Glasses (3DSP)") },
    { Bluetooth::SDP::ClassID::ThreeDSynchronisation,            _TXT("3D Synchronisation (3DSP)") },
    { Bluetooth::SDP::ClassID::MPS,                              _TXT("Multi-Profile Specification (MPS)") },
    { Bluetooth::SDP::ClassID::MPSSC,                            _TXT("MPS SC (MPS)") },
    { Bluetooth::SDP::ClassID::CTNAccessService,                 _TXT("CTN Access (CTN)") },
    { Bluetooth::SDP::ClassID::CTNNotificationService,           _TXT("CTN Notification (CTN)") },
    { Bluetooth::SDP::ClassID::CTN,                              _TXT("Calendar Tasks and Notes (CTN)") },
    { Bluetooth::SDP::ClassID::PnPInformation,                   _TXT("PnP Information (DID") },
    { Bluetooth::SDP::ClassID::GenericNetworking,                _TXT("Generic Networking") },
    { Bluetooth::SDP::ClassID::GenericFileTransfer,              _TXT("Generic File Transfer") },
    { Bluetooth::SDP::ClassID::GenericAudio,                     _TXT("Generic Audio") },
    { Bluetooth::SDP::ClassID::GenericTelephony,                 _TXT("Generic Telephony") },
    { Bluetooth::SDP::ClassID::UPNPService,                      _TXT("UPNP (ESDP)") },
    { Bluetooth::SDP::ClassID::UPNPIPService,                    _TXT("UPNP IP (ESDP)") },
    { Bluetooth::SDP::ClassID::ESDPUPNPIPPAN,                    _TXT("UPNP IP PAN (ESDP)") },
    { Bluetooth::SDP::ClassID::ESDPUPNPIPLAP,                    _TXT("UPNP IP LAP (ESDP)") },
    { Bluetooth::SDP::ClassID::ESDPUPNPL2CAP,                    _TXT("UPNP L2CAP (ESDP)") },
    { Bluetooth::SDP::ClassID::VideoSource,                      _TXT("Video Source (VDP)") },
    { Bluetooth::SDP::ClassID::VideoSink,                        _TXT("Video Sink (VDP)") },
    { Bluetooth::SDP::ClassID::VideoDistribution,                _TXT("Video Distribution Profile (VDP)") },
    { Bluetooth::SDP::ClassID::HDP,                              _TXT("Health Device (HDP)") },
    { Bluetooth::SDP::ClassID::HDPSource,                        _TXT("HDP Source (HDP)") },
    { Bluetooth::SDP::ClassID::HDPSink,                          _TXT("HDP Sink (HDP)") },
ENUM_CONVERSION_END(Bluetooth::SDP::ClassID::id)

ENUM_CONVERSION_BEGIN(Bluetooth::SDP::Service::AttributeDescriptor::id)
    { Bluetooth::SDP::Service::AttributeDescriptor::ServiceRecordHandle,         _TXT("ServiceRecordHandle") },
    { Bluetooth::SDP::Service::AttributeDescriptor::ServiceClassIDList,          _TXT("ServiceClassIDList") },
    { Bluetooth::SDP::Service::AttributeDescriptor::ServiceRecordState,          _TXT("ServiceRecordState") },
    { Bluetooth::SDP::Service::AttributeDescriptor::ServiceID,                   _TXT("ServiceID") },
    { Bluetooth::SDP::Service::AttributeDescriptor::ProtocolDescriptorList,      _TXT("ProtocolDescriptorList") },
    { Bluetooth::SDP::Service::AttributeDescriptor::BrowseGroupList,             _TXT("BrowseGroupList") },
    { Bluetooth::SDP::Service::AttributeDescriptor::LanguageBaseAttributeIDList, _TXT("LanguageBaseAttributeIDList") },
    { Bluetooth::SDP::Service::AttributeDescriptor::ServiceInfoTimeToLive,       _TXT("ServiceInfoTimeToLive") },
    { Bluetooth::SDP::Service::AttributeDescriptor::ServiceAvailability,         _TXT("ServiceAvailability") },
    { Bluetooth::SDP::Service::AttributeDescriptor::ProfileDescriptorList,       _TXT("BluetoothProfileDescriptorList") },
    { Bluetooth::SDP::Service::AttributeDescriptor::DocumentationURL,            _TXT("DocumentationURL") },
    { Bluetooth::SDP::Service::AttributeDescriptor::ClientExecutableURL,         _TXT("ClientExecutableURL") },
    { Bluetooth::SDP::Service::AttributeDescriptor::IconURL,                     _TXT("IconURL") },
ENUM_CONVERSION_END(Bluetooth::SDP::Service::AttributeDescriptor::id)

}