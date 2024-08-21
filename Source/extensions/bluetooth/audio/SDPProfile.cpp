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

#include "SDPSocket.h"
#include "SDPProfile.h"

namespace Thunder {

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

namespace Bluetooth {

namespace SDP {

    // CLIENT ---

    uint32_t Client::Discover(const std::list<UUID>& services, Tree& tree) const
    {
        // Convenience method to discover and add services and their attributes to a service tree.

        uint32_t result = Core::ERROR_NONE;

        std::vector<uint32_t> handles;

        // First find all the requested services...

        if ((result = ServiceSearch(services, handles)) == Core::ERROR_NONE) {

            for (uint32_t& h : handles) {
                std::list<std::pair<uint32_t, Buffer>> attributes;

                // Then retrieve their attributes...

                if ((result = ServiceAttribute(h, std::list<uint32_t>{ 0x0000FFFF } /* all of them */, attributes)) == Core::ERROR_NONE) {

                    Service& service(tree.Add(h));

                    for (auto const& attr : attributes) {
                        service.Deserialize(attr.first, attr.second);
                    }
                } else {
                    break;
                }
            }
        }

        if (result != Core::ERROR_NONE) {
            TRACE(Trace::Error, (_T("Failed to retrieve Bluetooth services via SDP protocol")));
        }

        return (result);
    }

    uint32_t Client::ServiceSearch(const std::list<UUID>& services, std::vector<uint32_t>& outHandles) const
    {
        ASSERT((services.size() > 0) && (services.size() <= 12)); // As per spec

        ClientSocket::Command command(_socket);

        const uint16_t maxResults = ((Capacity(command.Result()) - (2 * sizeof(uint32_t))) / sizeof(uint32_t));
        TRACE_L5("capacity=%d handles", maxResults);

        uint32_t result = Core::ERROR_NONE;
        Buffer continuationData;

        outHandles.clear();

        do {
            // Handle fragmented packets by repeating the request until the continuation state is 0.

            command.Set(PDU::ServiceSearchRequest, [&](Payload& payload) {

                // ServiceSearchRequest frame format:
                // - ServiceSearchPattern (sequence of UUIDs)
                // - MaximumServiceRecordCount (word)
                // - ContinuationState

                payload.Push(use_descriptor, services);
                payload.Push<uint16_t>(maxResults - outHandles.size());

                ASSERT(continuationData.size() <= 16);
                payload.Push<uint8_t>(continuationData.size());

                if (continuationData.empty() == false) {
                    payload.Push(continuationData);
                }
            });

            result = Execute(command, [&](const Payload& payload) {

                // ServiceSearchResponse frame format:
                // - TotalServiceRecordCount (long)
                // - CurrentServiceRecordCount (long)
                // - ServiceRecordHandleList (list of longs, not a sequence!)
                // - ContinuationState

                if (payload.Available() < (2 * sizeof(uint16_t))) {
                    TRACE_L1("SDP: Truncated payload in ServiceSearchResponse!");
                    result = Core::ERROR_BAD_REQUEST;
                }
                else {
                    uint16_t totalCount{};
                    payload.Pop(totalCount);

                    uint16_t currentCount{};
                    payload.Pop(currentCount);

                    TRACE_L5("TotalServiceRecordCount=%d", totalCount);
                    TRACE_L5("CurrentServiceRecordCount=%d", currentCount);

                    // Clip this in case of a phony value.
                    totalCount = std::min<uint16_t>(32, totalCount);
                    currentCount = std::min<uint16_t>(32, currentCount);

                    outHandles.reserve(totalCount);

                    if (payload.Available() >= (currentCount * sizeof(uint32_t))) {

                        payload.Pop(outHandles, currentCount);

                        for (uint16_t i=0; i < outHandles.size(); i++) {
                            TRACE_L5("ServiceRecordHandleList[%d]=0x%08x", i, outHandles[i]);
                        }

                        if (payload.Available() >= sizeof(uint8_t)) {
                            uint8_t continuationDataLength{};
                            payload.Pop(continuationDataLength);

                            if ((continuationDataLength > 16) || (payload.Available() < continuationDataLength)) {
                                TRACE_L1("SDP: Invalid continuation data in ServiceSearchResponse!");
                                result = Core::ERROR_BAD_REQUEST;
                            }
                            else if (continuationDataLength > 0) {
                                payload.Pop(continuationData, continuationDataLength);
                            }
                        }
                        else {
                            TRACE_L1("SDP: Truncated payload in ServiceSearchResponse!");
                            result = Core::ERROR_BAD_REQUEST;
                        }

                        if (payload.Available() != 0) {
                            TRACE_L1("SDP: Unexpected data in ServiceSearchResponse payload!");
                        }
                    } else {
                        TRACE_L1("SDP: Truncated payload in ServiceSearchResponse!");
                        result = Core::ERROR_BAD_REQUEST;
                    }
                }
            });

        } while ((continuationData.size() != 0) && (result == Core::ERROR_NONE));

        TRACE_L1("SDP: ServiceSearch found %d matching service(s)", outHandles.size());

        return (result);
    }

    /* private */
    uint32_t Client::InternalServiceAttribute(const PDU::pduid id,
                                              const std::function<void(Payload&)>& buildCb,
                                              const std::list<uint32_t>& attributeIdRanges,
                                              std::list<std::pair<uint32_t, Buffer>>& outAttributes) const
    {
        ASSERT(buildCb != 0);
        ASSERT((attributeIdRanges.size() > 0) && (attributeIdRanges.size() <= 256));

        ClientSocket::Command command(_socket);

        const uint16_t maxByteCount = (Capacity(command.Result()) - sizeof(uint16_t));
        TRACE_L5("capacity=%d bytes", maxByteCount);

        // From client perspective ServiceAttribute and ServiceSearchAttribute are very similar,
        // hence use one method to handle both.

        uint32_t result = Core::ERROR_NONE;
        Buffer continuationData;

        do {
            // Handle fragmented packets by repeating the request until the continuation state is 0.

            command.Set(id, [&](Payload& payload) {

                // Here's the difference between ServiceAttribute and ServiceSearchAttribute handled
                // First sends a service handle, the second a list of UUIDs to scan for.
                buildCb(payload);

                payload.Push(maxByteCount);
                payload.Push(use_descriptor, attributeIdRanges);

                ASSERT(continuationData.size() <= 16);
                payload.Push<uint8_t>(continuationData.size());

                if (continuationData.empty() == false) {
                    payload.Push(continuationData);
                }
            });

            result = Execute(command, [&](const Payload& payload) {

                // ServiceAttributeResponse/ServiceSearchAttributeResponse frame format:
                // - AttributeListByteCount (word)
                // - AttributeList (sequence of long:data pairs, where data size is dependant on the attribute and may be a sequence)
                // - ContinuationState

                if (payload.Available() < 2) {
                    TRACE_L1("SDP: Truncated Service(Search)AttributeResponse payload!");
                    result = Core::ERROR_BAD_REQUEST;
                }
                else {
                    payload.Pop(use_length, [&](const Payload& buffer) {
                        buffer.Pop(use_descriptor, [&](const Payload& sequence) {
                            while (sequence.Available() >= 2) {
                                uint16_t attribute{};
                                Buffer value;

                                sequence.Pop(use_descriptor, attribute);
                                sequence.Pop(use_descriptor, value);

                                TRACE_L5("attribute %d=[%s]", attribute, value.ToString().c_str());

                                outAttributes.emplace_back(attribute, std::move(value));
                            }

                            if (sequence.Available() != 0) {
                                TRACE_L1("SDP: Unexpected data in Service(Search)AttributeResponse!");
                            }
                        });

                        if (buffer.Available() != 0) {
                            TRACE_L1("SDP: Unexpected data in Service(Search)AttributeResponse!");
                        }
                    });

                    if (payload.Available() >= 1) {
                        uint8_t continuationDataLength{};
                        payload.Pop(continuationDataLength);

                        if ((continuationDataLength > 16) || (payload.Available() < continuationDataLength)) {
                            TRACE_L1("SDP: Invalid continuation data in Service(Search)AttributeResponse!");
                            result = Core::ERROR_BAD_REQUEST;
                        }
                        else {
                            payload.Pop(continuationData, continuationDataLength);
                        }
                    }
                    else {
                        TRACE_L1("SDP: Truncated payload in Service(Search)AttributeResponse!");
                        result = Core::ERROR_BAD_REQUEST;
                    }

                    if (payload.Available() != 0) {
                        TRACE_L1("SDP: Unexpected data in ServiceSearchAttributeResponse!");
                    }
                }
            });
        } while ((continuationData.size() != 0) && (result == Core::ERROR_NONE));

        TRACE_L1("SDP: Service(Search)Attribute found %d matching attribute(s)", outAttributes.size());

        return (result);
    }

    uint32_t Client::ServiceAttribute(const uint32_t serviceHandle, const std::list<uint32_t>& attributeIdRanges,
                        std::list<std::pair<uint32_t, Buffer>>& outAttributes) const
    {
        ASSERT(serviceHandle != 0);

        return (InternalServiceAttribute(PDU::ServiceAttributeRequest, [&](Payload& payload) {

            // ServiceAttributeRequest frame format:
            // - ServiceRecordHandle (long)
            // - MaximumAttributeByteCount (word)
            // - AttributeIDList (sequence of UUID ranges or UUIDs) [only ranges supported in this client-side implementation]
            // - ContinuationState

            payload.Push(serviceHandle);

            // MaximumAttributeByteCount, AttributeIDList and ContinuationState is handled by InternalServiceAttribute().

        }, attributeIdRanges, outAttributes));
    }

    uint32_t Client::ServiceSearchAttribute(const std::list<UUID>& services, const std::list<uint32_t>& attributeIdRanges,
                        std::list<std::pair<uint32_t, Buffer>>& outAttributes) const
    {
        ASSERT((services.size() > 0) && (services.size() <= 12));

        return (InternalServiceAttribute(PDU::ServiceSearchAttributeRequest, [&](Payload& payload) {

            // ServiceSearchAttributeRequest frame format:
            // - ServiceSearchPattern (sequence of UUIDs)
            // - MaximumAttributeByteCount (word)
            // - AttributeIDList (sequence of UUID ranges or UUIDs) [only ranges supported in this client-side implementation]
            // - ContinuationState

            payload.Push(use_descriptor, services);

            // MaximumAttributeByteCount, AttributeIDList and ContinuationState is handled by InternalServiceAttribute().

        }, attributeIdRanges, outAttributes));
    }

    /* private */
    uint32_t Client::Execute(ClientSocket::Command& command, const Payload::Inspector& inspectorCb) const
    {
        uint32_t result = Core::ERROR_ASYNC_FAILED;

        if (_socket.Exchange(ClientSocket::CommunicationTimeout, command, command) == Core::ERROR_NONE) {

            if (command.Result().Error() != PDU::Success) {
                TRACE_L1("SDP server: Message %d failed! [%d]", command.Call().Type(), command.Result().Error());
            }
            else if (command.Result().TransactionId() != command.Call().TransactionId()) {
                TRACE_L1("SDP server: Mismatched message transaction ID!");
            }
            else {
                result = Core::ERROR_NONE;

                if (inspectorCb != nullptr) {
                    command.Result().InspectPayload(inspectorCb);
                }
            }
        }

        return (result);
    }

    // SERVER ---

    void Server::OnServiceSearchRequest(const ServerSocket& socket, const PDU& request, const Handler& handlerCb)
    {
        PDU::errorid result = PDU::Success;
        std::list<UUID> uuids;
        uint16_t maxCount{};
        uint16_t offset = 0;

        request.InspectPayload([&](const Payload& payload) {

            // ServiceSearchRequest frame format:
            // - ServiceSearchPattern (sequence of UUIDs)
            // - MaximumServiceRecordCount (word)
            // - ContinuationState

            // In this server implementation the continuation state is a word
            // value containing the index of last service sent in the previous
            // call.

            payload.Pop(use_descriptor, [&](const Payload& sequence) {

                // No count stored, need to read until end of the sequence...

                while (sequence.Available() >= 2 /* min UUID size */) {
                    UUID uuid;

                    sequence.Pop(use_descriptor, uuid);

                    TRACE_L5("ServiceSearchPattern[%d]=%s ('%s')", uuids.size(), uuid.ToString().c_str(), ClassID(uuid).Name().c_str());

                    if (uuid.IsValid() == true) {
                        uuids.push_back(uuid);
                    }
                    else {
                        TRACE_L1("SDP server: invalid UUID!");
                        uuids.clear();
                        break;
                    }
                }

                if (sequence.Available() != 0) {
                    TRACE_L1("SDP server: Unexpected data ServiceSearchRequest payload!");
                    // Let's continue anyway...
                }
            });

            payload.Pop(maxCount); // 0 if truncated, 0 is error

            TRACE_L5("MaximumServiceRecordCount=%d", maxCount);

            if ((uuids.empty() == true) || (maxCount == 0)) {
                TRACE_L1("SDP server: Invalid ServiceSearchRequest parameters!");
                result = PDU::InvalidRequestSyntax;
            }
            else if (payload.Available() >= sizeof(uint8_t)) {
                uint8_t continuationSize;
                payload.Pop(continuationSize);

                if (continuationSize == sizeof(uint16_t)) {
                    payload.Pop(offset);

                    if (offset == 0) {
                        TRACE_L1("SDP server: Invalid ServiceSearchRequest continuation state (zero)!");
                        result = PDU::InvalidContinuationState;
                    }
                }
                else if (continuationSize != 0) {
                    TRACE_L1("SDP server: Invalid ServiceSearchRequest continuation size!");
                    result = PDU::InvalidContinuationState;
                }
            }
            else {
                TRACE_L1("SDP server: Truncated ServiceSearchRequest payload!");
                result = PDU::InvalidPduSize;
            }
        });

        uint16_t count = 0;
        std::list<uint32_t> handles;

        if (result == PDU::Success) {

            // Sca all matching handles, even if can't fit them all in response,
            // because the total number of matches needs to be returned.

            WithServiceTree([&](const Tree& tree) {
                for (auto const& service : tree.Services()) {
                    for (auto const& uuid : uuids) {

                        if (service.Search(uuid) == true) {
                            if ((count >= offset) && (handles.size() < maxCount)) {
                                handles.push_back(service.Handle());
                            }

                            count++;
                        }
                    }
                }
            });

            if (offset > count) {
                TRACE_L1("SDP server: Invalid ServiceSearchRequest continuation state!");
                result = PDU::InvalidContinuationState;
            }
        }

        if (result == PDU::Success) {

            // Cap the max count to the actual possible capacity.
            maxCount = std::min<uint16_t>(maxCount, (Capacity(socket, request) - (2 * sizeof(uint32_t)) / sizeof(uint32_t)));
            TRACE_L5("capacity=%d handles", maxCount);

            handlerCb(PDU::ServiceSearchResponse, [&](Payload& payload) {

                // ServiceSearchResponse frame format:
                // - TotalServiceRecordCount (word)
                // - CurrentServiceRecordCount (word)
                // - ServiceRecordHandleList (list of longs, not sequence)
                // - ContinuationState

                payload.Push<uint16_t>(count);
                payload.Push<uint16_t>(handles.size());
                payload.Push(handles);

                if ((offset + handles.size()) < count) {
                    // Not all sent yet! Store offset for continuation.
                    payload.Push<uint8_t>(sizeof(uint16_t));
                    payload.Push<uint16_t>(offset + handles.size());
                }
                else {
                    // All sent!
                    payload.Push<uint8_t>(0);
                }

                TRACE_L1("SDP server: ServiceSearch found %d matching service(s) and will reply with %d service(s)", count, handles.size());
            });
        }
        else {
            TRACE_L1("SDP server: ServiceSearchRequest failed!");
            handlerCb(result);
        }
    }

    /* private */
    void Server::InternalOnServiceSearchAttributeRequest(const PDU::pduid id,
                                                         const std::function<PDU::errorid(const Payload&, std::list<uint32_t>&)>& inspectCb,
                                                         const ServerSocket& socket,
                                                         const PDU& request,
                                                         const Handler& handlerCb)
    {
        // Again, ServiceAttribute and ServiceSearchAttribute are very similar,
        // so use one method to handle both.

        PDU::errorid result = PDU::Success;

        uint16_t maxByteCount{};
        std::list<uint32_t> attributeRanges;
        std::list<uint32_t> handles;
        uint16_t offset = 0;

        request.InspectPayload([&](const Payload& payload) {

            // ServiceAttributeRequest/ServiceSearch frame format:
            // - ServiceRecordHandle (long) OR ServiceSearchPattern (sequence of UUIDs)
            // - MaximumAttributeByteCount (word)
            // - AttributeIDList (sequence of UUID ranges or UUIDs)
            // - ContinuationState

            // In this server implementation the continuation state is a word
            // value containing the index of last attribute sent in the previous
            // call.

            if (inspectCb(payload, handles) == PDU::Success) {

                payload.Pop(maxByteCount);
                TRACE_L5("MaximumAttributeByteCount=%d", maxByteCount);

                if (maxByteCount > 9 /* i.e. descriptors and one result entry */) {
                    payload.Pop(use_descriptor, [&](const Payload& sequence) {
                        while (sequence.Available() >= (sizeof(uint16_t) + 1)) {
                            uint32_t range{};
                            uint32_t size{};

                            sequence.Pop(use_descriptor, range, &size);

                            if (size == sizeof(uint32_t)) {
                                // A range indeed.
                                TRACE_L5("AttributeIDList[%d]=%04x..%04x", attributeRanges.size(), (range >> 16), (range & 0xFFFF));

                                attributeRanges.push_back(range);
                            }
                            else if (size == sizeof(uint16_t)) {
                                // Not a range, but single 16-bit UUID.
                                TRACE_L5("AttributeIDList[%d]=%04x ('%s')", attributeRanges.size(), range, ClassID(UUID(range)).Name().c_str());

                                attributeRanges.push_back((static_cast<uint16_t>(range) << 16) | static_cast<uint16_t>(range));
                            }
                            else {
                                TRACE_L1("SDP server: Invalid UUID list!");
                                result = PDU::InvalidRequestSyntax;
                            }
                        }

                        if (sequence.Available() != 0) {
                            TRACE_L1("SDP server: Unexpected data in Service(Search)AttributeRequest payload!");
                        }
                    });
                }

                if ((maxByteCount < 9) || (attributeRanges.empty() == true)) {
                    TRACE_L1("SDP server: Invalid Service(Search)AttributeRequest parameters!");
                    result = PDU::InvalidRequestSyntax;
                }
                else if (payload.Available() >= sizeof(uint8_t)) {
                    uint8_t continuationSize;
                    payload.Pop(continuationSize);

                    if (continuationSize == sizeof(uint16_t)) {
                        payload.Pop(offset);

                        if (offset == 0) {
                            TRACE_L1("SDP server: Invalid Service(Search)AttributeRequest continuation state (zero)!");
                            result = PDU::InvalidContinuationState;
                        }
                    }
                    else if (continuationSize != 0) {
                        TRACE_L1("SDP server: Invalid Service(Search)AttributeRequest continuation size!");
                        result = PDU::InvalidContinuationState;
                    }
                }
                else {
                    TRACE_L1("SDP server: Truncated Service(Search)AttributeRequest payload!");
                    result = PDU::InvalidPduSize;
                }
            }
        });

        std::list<std::map<uint16_t, Buffer>> attributes;
        uint16_t totalAttributes = 0;

        if (result == PDU::Success) {

            WithServiceTree([&](const Tree& tree) {

                for (auto const& handle : handles) {
                    const Service* service = tree.Find(handle);

                    if (service != nullptr) {
                        attributes.emplace_back(SerializeAttributesByRange(*service, attributeRanges, offset, totalAttributes));
                    }
                }
            });

            if (result == PDU::Success) {

                // Cap the max count to the actual possible capacity.
                maxByteCount = std::min<uint16_t>(maxByteCount, (Capacity(socket, request) - sizeof(uint16_t)));
                TRACE_L5("capacity=%d bytes", maxByteCount);

                handlerCb(id, [&](Payload& payload) {

                    // ServiceAttributeResponse/ServiceSearchAttribute frame format:
                    // - AttributeListByteCount (word)
                    // - AttributeList (sequence of long:data pairs, where data size is dependant on the attribute and may be a sequence)
                    // - ContinuationState

                    bool allDone = true;
                    uint16_t attributesWritten = 0;

                    auto PushLists = [&](Payload& buffer) {
                        for (auto const& service : attributes) {
                            buffer.Push(use_descriptor, [&](Payload& sequence) {
                                for (auto const& attr : service) {
                                    if ((sequence.Position() + attr.second.size() + sizeof(uint16_t)) < maxByteCount) {

                                        if (attr.second.size() != 0) {
                                            sequence.Push([&](Payload& record) {
                                                record.Push(use_descriptor, attr.first);
                                                record.Push(attr.second); // Already packed as necessary, thus no use_descriptor here.
                                            });

                                            attributesWritten++;
                                        }
                                    }
                                    else {
                                        allDone = false;
                                        break;
                                    }
                                }
                            });

                            if (allDone == false) {
                                break;
                            }
                        }
                    };

                    payload.Push(use_length, [&](Payload& buffer) {
                        if (id == PDU::ServiceAttributeResponse) {
                            ASSERT(handles.size() == 1);
                            PushLists(buffer);
                        }
                        else {
                            // In case of ServiceSearchAtributes this is a list of lists.
                            buffer.Push(use_descriptor, [&](Payload& listBuffer) {
                                PushLists(listBuffer);
                            });
                        }
                    });

                    if (allDone == false) {
                        payload.Push<uint8_t>(sizeof(uint16_t));
                        payload.Push<uint16_t>(offset + attributesWritten);
                    }
                    else {
                        payload.Push<uint8_t>(0);
                    }

                    TRACE_L1("SDP server: Service(Search)Attribute found %d attribute(s) and will reply with %d attribute(s)", totalAttributes, attributesWritten);
                });
            }
        }
        else {
            TRACE_L1("SDP server: Service(Search)Attribute failed!");
            handlerCb(result);
        }
    }

    void Server::OnServiceAttributeRequest(const ServerSocket& socket, const PDU& request, const Handler& handlerCb)
    {
        InternalOnServiceSearchAttributeRequest(PDU::ServiceAttributeResponse,
        [&](const Payload& payload, std::list<uint32_t>& handles) -> PDU::errorid {

            // ServiceAttributeRequest frame format:
            // - ServiceRecordHandle (long)
            // - MaximumAttributeByteCount (word)
            // - AttributeIDList (sequence of UUID ranges or UUIDs)
            // - ContinuationState

            uint32_t handle{};
            payload.Pop(handle); // 0 if truncated

            TRACE_L1("ServiceRecordHandle=0x%08x", handle);

            if (handle != 0) {
                // Easy, one handle is given, directly.
                handles.push_back(handle);
            }

            // MaximumAttributeByteCount, AttributeIDList and ContinuationState is handled by InternalOnServiceSearchAttributeRequest().

            return (PDU::Success);

        }, socket, request, handlerCb);
    }

    void Server::OnServiceSearchAttributeRequest(const ServerSocket& socket, const PDU& request, const Handler& handlerCb)
    {
        InternalOnServiceSearchAttributeRequest(PDU::ServiceSearchAttributeResponse,
        [&](const Payload& payload, std::list<uint32_t>& handles) -> PDU::errorid {

            // ServiceSearchAttribute frame format:
            // - ServiceSearchPattern (sequence of UUIDs)
            // - MaximumAttributeByteCount (word)
            // - AttributeIDList (sequence of UUID ranges or UUIDs)
            // - ContinuationState

            std::list<UUID> uuids;

            // Pick up UUIDs of interest...
            payload.Pop(use_descriptor, [&](const Payload& sequence) {
                while (sequence.Available() >= 2 /* min UUID size */) {
                    UUID uuid;

                    sequence.Pop(use_descriptor, uuid);

                    TRACE_L1("ServiceSearchPattern[%d]=%s '%s'", uuids.size(), uuid.ToString().c_str(), ClassID(uuid).Name().c_str());

                    if (uuid.IsValid() == true) {
                        uuids.push_back(uuid);
                    }
                    else {
                        TRACE_L1("SDP server: Invalid UUID in ServiceSearchRequest payload!");
                    }
                }

                if (sequence.Available() != 0) {
                    TRACE_L1("SDP server: Unexpected data ServiceSearchRequest payload!");
                }
            });

            if (uuids.empty() == false) {
                // Now have to find the handles that have anything to do with the UUIDs recieved.

                WithServiceTree([&](const Tree& tree) {
                    for (auto const& service : tree.Services()) {
                        for (auto const& uuid : uuids) {
                            if (service.Search(uuid) == true) {
                                handles.push_back(service.Handle());
                            }
                        }
                    }
                });
            }

            TRACE_L1("SDP server: Found %d service(s) matching the search patterns", handles.size());

            return (uuids.empty() == true? PDU::InvalidRequestSyntax : PDU::Success);

        }, socket, request, handlerCb);
    }

    /* private */
    std::map<uint16_t, Buffer> Server::SerializeAttributesByRange(const Service& service, const std::list<uint32_t>& attributeRanges, const uint16_t offset, uint16_t& count) const
    {
        std::map<uint16_t, Buffer> attributes;

        for (uint32_t range : attributeRanges) {
            const uint16_t standardLeft = std::max<uint16_t>(Service::AttributeDescriptor::ServiceRecordHandle, (range >> 16));
            const uint16_t standardRight = std::min<uint16_t>(Service::AttributeDescriptor::IconURL, (range & 0xFFFF));

            // Universal attributes.
            for (uint16_t id = standardLeft; id <= standardRight; id++) {
                Buffer buffer = service.Serialize(id);

                if (buffer.empty() == false) {
                    if (count >= offset) {
                        attributes.emplace(id, std::move(buffer));
                    }

                    count++;
                }
            }

            // custom attributes
            for (auto const& attr : service.Attributes()) {
                if ((attr.Id() >= std::max<uint16_t>(0x100, (range >> 16))) && (attr.Id() <= (range & 0xFFFF))) {
                    if (count >= offset) {
                        attributes.emplace(attr.Id(), attr.Value());
                    }

                    count++;
                }
            }
        }

        return (attributes);
    }

} // namespace SDP

} // namespace Bluetooth

}