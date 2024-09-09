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

#pragma once

#include "Module.h"
#include "GATTSocket.h"

namespace Thunder {

namespace Bluetooth {

    class EXTERNAL GATTProfile {
    private:
        static constexpr uint16_t PRIMARY_SERVICE_UUID = 0x2800;
        static constexpr uint16_t CHARACTERISTICS_UUID = 0x2803;

    public:
        class EXTERNAL Service {
        public:
            enum type : uint16_t {
                GenericAccess               = 0x1800,
                AlertNotificationService    = 0x1811,
                AutomationIO                = 0x1815,
                BatteryService              = 0x180F,
                BinarySensor                = 0x183B,
                BloodPressure               = 0x1810,
                BodyComposition             = 0x181B,
                BondManagementService       = 0x181E,
                ContinuousGlucoseMonitoring = 0x181F,
                CurrentTimeService          = 0x1805,
                CyclingPower                = 0x1818,
                CyclingSpeedAndCadence      = 0x1816,
                DeviceInformation           = 0x180A,
                EmergencyConfiguration      = 0x183C,
                EnvironmentalSensing        = 0x181A,
                FitnessMachine              = 0x1826,
                GenericAttribute            = 0x1801,
                Glucose                     = 0x1808,
                HealthThermometer           = 0x1809,
                HeartRate                   = 0x180D,
                HTTPProxy                   = 0x1823,
                HumanInterfaceDevice        = 0x1812,
                ImmediateAlert              = 0x1802,
                IndoorPositioning           = 0x1821,
                InsulinDelivery             = 0x183A,
                InternetProtocolSupport     = 0x1820,
                LinkLoss                    = 0x1803,
                LocationAndNavigation       = 0x1819,
                MeshProvisioningService     = 0x1827,
                MeshProxyService            = 0x1828,
                NextDSTChangeService        = 0x1807,
                ObjectTransferService       = 0x1825,
                PhoneAlertStatusService     = 0x180E,
                PulseOximeterService        = 0x1822,
                ReconnectionConfiguration   = 0x1829,
                ReferenceTimeUpdateService  = 0x1806,
                RunningSpeedAndCadence      = 0x1814,
                ScanParameters              = 0x1813,
                TransportDiscovery          = 0x1824,
                TxPower                     = 0x1804,
                UserData                    = 0x181C,
                WeightScale                 = 0x181D,
            };

            class EXTERNAL Characteristic {
            public:
                enum type : uint16_t {

                    AerobicHeartRateLowerLimit                = 0x2A7E,
                    AerobicHeartRateUpperLimit                = 0x2A84,
                    AerobicThreshold                          = 0x2A7F,
                    Age                                       = 0x2A80,
                    Aggregate                                 = 0x2A5A,
                    AlertCategoryID                           = 0x2A43,
                    AlertCategoryIDBitMask                    = 0x2A42,
                    AlertLevel                                = 0x2A06,
                    AlertNotificationControlPoint             = 0x2A44,
                    AlertStatus                               = 0x2A3F,
                    Altitude                                  = 0x2AB3,
                    AnaerobicHeartRateLowerLimit              = 0x2A81,
                    AnaerobicHeartRateUpperLimit              = 0x2A82,
                    AnaerobicThreshold                        = 0x2A83,
                    Analog                                    = 0x2A58,
                    AnalogOutput                              = 0x2A59,
                    ApparentWindDirection                     = 0x2A73,
                    ApparentWindSpeed                         = 0x2A72,
                    Appearance                                = 0x2A01,
                    BarometricPressureTrend                   = 0x2AA3,
                    BatteryLevel                              = 0x2A19,
                    BatteryLevelState                         = 0x2A1B,
                    BatteryPowerState                         = 0x2A1A,
                    BloodPressureFeature                      = 0x2A49,
                    BloodPressureMeasurement                  = 0x2A35,
                    BodyCompositionFeature                    = 0x2A9B,
                    BodyCompositionMeasurement                = 0x2A9C,
                    BodySensorLocation                        = 0x2A38,
                    BondManagementControlPoint                = 0x2AA4,
                    BondManagementFeatures                    = 0x2AA5,
                    BootKeyboardInputReport                   = 0x2A22,
                    BootKeyboardOutputReport                  = 0x2A32,
                    BootMouseInputReport                      = 0x2A33,
                    BSSControlPoint                           = 0x2B2B,
                    BSSResponse                               = 0x2B2C,
                    CGMFeature                                = 0x2AA8,
                    CGMMeasurement                            = 0x2AA7,
                    CGMSessionRunTime                         = 0x2AAB,
                    CGMSessionStartTime                       = 0x2AAA,
                    CGMSpecificOpsControlPoint                = 0x2AAC,
                    CGMStatus                                 = 0x2AA9,
                    CrossTrainerData                          = 0x2ACE,
                    CSCFeature                                = 0x2A5C,
                    CSCMeasurement                            = 0x2A5B,
                    CurrentTime                               = 0x2A2B,
                    CyclingPowerControlPoint                  = 0x2A66,
                    CyclingPowerFeature                       = 0x2A65,
                    CyclingPowerMeasurement                   = 0x2A63,
                    CyclingPowerVector                        = 0x2A64,
                    DatabaseChangeIncrement                   = 0x2A99,
                    DateofBirth                               = 0x2A85,
                    DateofThresholdAssessment                 = 0x2A86,
                    DateTime                                  = 0x2A08,
                    DateUTC                                   = 0x2AED,
                    DayDateTime                               = 0x2A0A,
                    DayofWeek                                 = 0x2A09,
                    DescriptorValueChanged                    = 0x2A7D,
                    DewPoint                                  = 0x2A7B,
                    Digital                                   = 0x2A56,
                    DigitalOutput                             = 0x2A57,
                    DSTOffset                                 = 0x2A0D,
                    Elevation                                 = 0x2A6C,
                    EmailAddress                              = 0x2A87,
                    EmergencyID                               = 0x2B2D,
                    EmergencyText                             = 0x2B2E,
                    ExactTime100                              = 0x2A0B,
                    ExactTime256                              = 0x2A0C,
                    FatBurnHeartRateLowerLimit                = 0x2A88,
                    FatBurnHeartRateUpperLimit                = 0x2A89,
                    FirmwareRevisionString                    = 0x2A26,
                    FirstName                                 = 0x2A8A,
                    FitnessMachineControlPoint                = 0x2AD9,
                    FitnessMachineFeature                     = 0x2ACC,
                    FitnessMachineStatus                      = 0x2ADA,
                    FiveZoneHeartRateLimits                   = 0x2A8B,
                    FloorNumber                               = 0x2AB2,
                    CentralAddressResolution                  = 0x2AA6,
                    DeviceName                                = 0x2A00,
                    PeripheralPreferredConnectionParameters   = 0x2A04,
                    PeripheralPrivacyFlag                     = 0x2A02,
                    ReconnectionAddress                       = 0x2A03,
                    ServiceChanged                            = 0x2A05,
                    Gender                                    = 0x2A8C,
                    GlucoseFeature                            = 0x2A51,
                    GlucoseMeasurement                        = 0x2A18,
                    GlucoseMeasurementContext                 = 0x2A34,
                    GustFactor                                = 0x2A74,
                    HardwareRevisionString                    = 0x2A27,
                    HeartRateControlPoint                     = 0x2A39,
                    HeartRateMax                              = 0x2A8D,
                    HeartRateMeasurement                      = 0x2A37,
                    HeatIndex                                 = 0x2A7A,
                    Height                                    = 0x2A8E,
                    HIDControlPoint                           = 0x2A4C,
                    HIDInformation                            = 0x2A4A,
                    HipCircumference                          = 0x2A8F,
                    HTTPControlPoint                          = 0x2ABA,
                    HTTPEntityBody                            = 0x2AB9,
                    HTTPHeaders                               = 0x2AB7,
                    HTTPStatusCode                            = 0x2AB8,
                    HTTPSSecurity                             = 0x2ABB,
                    Humidity                                  = 0x2A6F,
                    IDDAnnunciationStatus                     = 0x2B22,
                    IDDCommandControlPoint                    = 0x2B25,
                    IDDCommandData                            = 0x2B26,
                    IDDFeatures                               = 0x2B23,
                    IDDHistoryData                            = 0x2B28,
                    IDDRecordAccessControlPoint               = 0x2B27,
                    IDDStatus                                 = 0x2B21,
                    IDDStatusChanged                          = 0x2B20,
                    IDDStatusReaderControlPoint               = 0x2B24,
                    IndoorBikeData                            = 0x2AD2,
                    IndoorPositioningConfiguration            = 0x2AAD,
                    IntermediateCuffPressure                  = 0x2A36,
                    IntermediateTemperature                   = 0x2A1E,
                    Irradiance                                = 0x2A77,
                    Language                                  = 0x2AA2,
                    LastName                                  = 0x2A90,
                    Latitude                                  = 0x2AAE,
                    LNControlPoint                            = 0x2A6B,
                    LNFeature                                 = 0x2A6A,
                    LocalEastCoordinate                       = 0x2AB1,
                    LocalNorthCoordinate                      = 0x2AB0,
                    LocalTimeInformation                      = 0x2A0F,
                    LocationandSpeedCharacteristic            = 0x2A67,
                    LocationName                              = 0x2AB5,
                    Longitude                                 = 0x2AAF,
                    MagneticDeclination                       = 0x2A2C,
                    MagneticFluxDensity_2D                    = 0x2AA0,
                    MagneticFluxDensity_3D                    = 0x2AA1,
                    ManufacturerNameString                    = 0x2A29,
                    MaximumRecommendedHeartRate               = 0x2A91,
                    MeasurementInterval                       = 0x2A21,
                    ModelNumberString                         = 0x2A24,
                    Navigation                                = 0x2A68,
                    NetworkAvailability                       = 0x2A3E,
                    NewAlert                                  = 0x2A46,
                    ObjectActionControlPoint                  = 0x2AC5,
                    ObjectChanged                             = 0x2AC8,
                    ObjectFirst_Created                       = 0x2AC1,
                    ObjectID                                  = 0x2AC3,
                    ObjectLast_Modified                       = 0x2AC2,
                    ObjectListControlPoint                    = 0x2AC6,
                    ObjectListFilter                          = 0x2AC7,
                    ObjectName                                = 0x2ABE,
                    ObjectProperties                          = 0x2AC4,
                    ObjectSize                                = 0x2AC0,
                    ObjectType                                = 0x2ABF,
                    OTSFeature                                = 0x2ABD,
                    PLXContinuousMeasurementCharacteristic    = 0x2A5F,
                    PLXFeatures                               = 0x2A60,
                    PLXSpot_CheckMeasurement                  = 0x2A5E,
                    PnPID                                     = 0x2A50,
                    PollenConcentration                       = 0x2A75,
                    Position2D                                = 0x2A2F,
                    Position3D                                = 0x2A30,
                    PositionQuality                           = 0x2A69,
                    Pressure                                  = 0x2A6D,
                    ProtocolMode                              = 0x2A4E,
                    PulseOximetryControlPoint                 = 0x2A62,
                    Rainfall                                  = 0x2A78,
                    RCFeature                                 = 0x2B1D,
                    RCSettings                                = 0x2B1E,
                    ReconnectionConfigurationControlPoint     = 0x2B1F,
                    RecordAccessControlPoint                  = 0x2A52,
                    RegulatoryCertificationDataList           = 0x2A2A,
                    ReferenceTimeInformation                  = 0x2A14,
                    Removable                                 = 0x2A3A,
                    Report                                    = 0x2A4D,
                    ReportMap                                 = 0x2A4B,
                    ResolvablePrivateAddressOnly              = 0x2AC9,
                    RestingHeartRate                          = 0x2A92,
                    RingerControlpoint                        = 0x2A40,
                    RingerSetting                             = 0x2A41,
                    RowerData                                 = 0x2AD1,
                    RSCFeature                                = 0x2A54,
                    RSCMeasurement                            = 0x2A53,
                    SCControlPoint                            = 0x2A55,
                    ScanIntervalWindow                        = 0x2A4F,
                    ScanRefresh                               = 0x2A31,
                    ScientificTemperatureCelsius              = 0x2A3C,
                    SecondaryTimeZone                         = 0x2A10,
                    SensorLocation                            = 0x2A5D,
                    SerialNumberString                        = 0x2A25,
                    ServiceRequired                           = 0x2A3B,
                    SoftwareRevisionString                    = 0x2A28,
                    SportTypeforAerobicandAnaerobicThresholds = 0x2A93,
                    StairClimberData                          = 0x2AD0,
                    StepClimberData                           = 0x2ACF,
                    String                                    = 0x2A3D,
                    SupportedHeartRateRange                   = 0x2AD7,
                    SupportedInclinationRange                 = 0x2AD5,
                    SupportedNewAlertCategory                 = 0x2A47,
                    SupportedPowerRange                       = 0x2AD8,
                    SupportedResistanceLevelRange             = 0x2AD6,
                    SupportedSpeedRange                       = 0x2AD4,
                    SupportedUnreadAlertCategory              = 0x2A48,
                    SystemID                                  = 0x2A23,
                    TDSControlPoint                           = 0x2ABC,
                    Temperature                               = 0x2A6E,
                    TemperatureCelsius                        = 0x2A1F,
                    TemperatureFahrenheit                     = 0x2A20,
                    TemperatureMeasurement                    = 0x2A1C,
                    TemperatureType                           = 0x2A1D,
                    ThreeZoneHeartRateLimits                  = 0x2A94,
                    TimeAccuracy                              = 0x2A12,
                    TimeBroadcast                             = 0x2A15,
                    TimeSource                                = 0x2A13,
                    TimeUpdateControlPoint                    = 0x2A16,
                    TimeUpdateState                           = 0x2A17,
                    TimewithDST                               = 0x2A11,
                    TimeZone                                  = 0x2A0E,
                    TrainingStatus                            = 0x2AD3,
                    TreadmillData                             = 0x2ACD,
                    TrueWindDirection                         = 0x2A71,
                    TrueWindSpeed                             = 0x2A70,
                    TwoZoneHeartRateLimit                     = 0x2A95,
                    TxPowerLevel                              = 0x2A07,
                    Uncertainty                               = 0x2AB4,
                    UnreadAlertStatus                         = 0x2A45,
                    URI                                       = 0x2AB6,
                    UserControlPoint                          = 0x2A9F,
                    UserIndex                                 = 0x2A9A,
                    UVIndex                                   = 0x2A76,
                    VO2Max                                    = 0x2A96,
                    WaistCircumference                        = 0x2A97,
                    Weight                                    = 0x2A98,
                    WeightMeasurement                         = 0x2A9D,
                    WeightScaleFeature                        = 0x2A9E,
                    WindChill                                 = 0x2A79
                };

                class EXTERNAL Descriptor {
                public:
                    enum type : uint16_t {
                        CharacteristicAggregateFormat         = 0x2905,
                        CharacteristicExtendedPropertie       = 0x2900,
                        CharacteristicPresentationFormat      = 0x2904,
                        CharacteristicUserDescription         = 0x2901,
                        ClientCharacteristicConfiguration     = 0x2902,
                        EnvironmentalSensingConfiguration     = 0x290B,
                        EnvironmentalSensingMeasurement	      = 0x290C,
                        EnvironmentalSensingTriggerSetting    = 0x290D,
                        ExternalReportReference               = 0x2907,
                        NumberOfDigital                       = 0x2909,
                        ReportReference	                      = 0x2908,
                        ServerCharacteristicConfiguration     = 0x2903,
                        TimeTriggerSetting                    = 0x290E,
                        ValidRange                            = 0x2906,
                        ValueTriggerSetting                   = 0x290A
                    };

                public:
                    Descriptor(const Descriptor&) = delete;
                    Descriptor& operator= (const Descriptor&) = delete;

                    Descriptor(const uint16_t handle, const UUID& uuid)
                        : _handle(handle)
                        , _uuid(uuid) {
                    }
                    ~Descriptor() {
                    }

                public:
                    bool operator== (const UUID& id) const {
                        return (id == _uuid);
                    }
                    bool operator!= (const UUID& id) const {
                        return (!operator== (id));
                    }
                    const UUID& Type() const {
                        return (_uuid);
                    }
                    string Name() const {
                        string result;
                        if (_uuid.HasShort() == false) {
                            result = _uuid.ToString();
                        }
                        else {
                            type input = static_cast<type>(_uuid.Short());
                            Core::EnumerateType<type> value (input);
                            result = (value.IsSet() == true ? string(value.Data()) : _uuid.ToString(false));
                        }
                        return (result);
                    }
                    uint16_t Handle() const {
                        return (_handle);
                    }
 
                private:
                    uint16_t _handle;
                    UUID _uuid;
                };

            public: 
                typedef Core::IteratorType< const std::list<Descriptor>, const Descriptor&, std::list<Descriptor>::const_iterator> Iterator;

                Characteristic(const Characteristic&) = delete;
                Characteristic& operator= (const Characteristic&) = delete;

                Characteristic(const uint16_t end, const uint8_t rights, const uint16_t value, const UUID& attribute)
                    : _handle(value)
                    , _rights(rights)
                    , _end(end)
                    , _type(attribute)
                    , _error(0)
                    , _value() {
                }
                ~Characteristic() {
                }

            public:
                bool operator== (const UUID& id) const {
                    return (id == _type);
                }
                bool operator!= (const UUID& id) const {
                    return (!operator== (id));
                }
                uint8_t Error() const {
                    return (_error);
                }
                string ToString() const {
                    string result;
                    result.reserve(_value.length() + 1);
                    for (const char& c : _value) {
                        if (::isprint(c)) {
                            result = result + c;
                        }
                        else {
                            result = result + '.';
                        }
                    }
                    return (result);
                }
                const UUID& Type() const {
                    return (_type);
                }
                uint8_t Rights() const {
                    return (_rights);
                }
                string Name() const {
                    string result;
                    if (_type.HasShort() == false) {
                        result = _type.ToString();
                    }
                    else {
                        type input = static_cast<type>(_type.Short());
                        Core::EnumerateType<type> value (input);
                        result = (value.IsSet() == true ? string(value.Data()) : _type.ToString(false));
                    }
                    return (result);
                }
                Iterator Descriptors() const {
                    return (Iterator(_descriptors));
                }
                const Descriptor* operator[] (const UUID& id) const {
                    std::list<Descriptor>::const_iterator index (_descriptors.begin());

                    while ((index != _descriptors.end()) && (*index != id)) { index++; }

                    return (index != _descriptors.end() ? &(*index) : nullptr);
                }
                uint16_t Handle() const {
                    return (_handle);
                }
                uint16_t Max() const {
                    return (_end);
                }
               
            private:
                friend class GATTProfile;
                void Descriptors (GATTSocket::Command::Response& response) {
                    while (response.Next() == true) {
                        UUID descriptor(response.Attribute());
                        _descriptors.emplace_back(response.Handle(), descriptor);
                    }
                }
                uint16_t Value(GATTSocket::Command::Response& response) {
                    _error = response.Error();
                    if ( (_error != 0) || (response.Data() == nullptr)) {
                        _value.clear();
                    }
                    else {
                        _value = std::string(reinterpret_cast<const char*>(response.Data()), response.Length());
                    }
                    return(0);
                }

            private:
                uint16_t _handle;
                uint8_t _rights;
                uint16_t _end;
                UUID _type;
                std::list<Descriptor> _descriptors;
                uint8_t _error;
                std::string _value;
            };

        public:
            typedef Core::IteratorType< const std::list<Characteristic>, const Characteristic&, std::list<Characteristic>::const_iterator> Iterator;
            typedef Core::IteratorType< std::list<Characteristic>, Characteristic&> Index;

            Service(const Service&) = delete;
            Service& operator= (const Service&) = delete;

            Service(const UUID& serviceId, const uint16_t handle, const uint16_t group)
                : _handle(handle)
                , _group(group)
                , _serviceId(serviceId)
                , _characteristics() {
            }
            ~Service() {
            }

        public:
            bool operator== (const UUID& id) const {
                return (id == _serviceId);
            }
            bool operator!= (const UUID& id) const {
                return (!operator== (id));
            }
            const UUID& Type() const {
                return (_serviceId);
            }
            string Name() const {
                string result;
                if (_serviceId.HasShort() == false) {
                    result = _serviceId.ToString();
                }
                else {
                    Core::EnumerateType<type> value(static_cast<type>(_serviceId.Short()));
                    result = (value.IsSet() == true ? string(value.Data()) : _serviceId.ToString(false));
                }
                return (result);
            }
            Iterator Characteristics() const {
                return (Iterator(_characteristics));
            }
            const Characteristic* operator[] (const UUID& id) const {
                std::list<Characteristic>::const_iterator index (_characteristics.begin());

                while ((index != _characteristics.end()) && (*index != id)) { index++; }

                return (index != _characteristics.end() ? &(*index) : nullptr);
            }
            uint16_t Handle() const {
                return (_handle);
            }
            uint16_t Max() const {
                return (_group);
            }
 
        private:
            friend class GATTProfile;
            void AddCharacteristics (GATTSocket::Command::Response& response) {
                if (response.Next() == true) {
                    do  {
                        uint16_t value (response.Group());
                        uint8_t rights (response.Rights());
                        UUID attribute (response.Attribute());

                        // Where does the next one start ?
                        uint16_t end = (response.Next() ? response.Handle() - 1 : Max());

                        _characteristics.emplace_back(end, rights, value, attribute);

                    } while (response.IsValid() == true);
                }
            }
            Index Filler() {
                return (Index(_characteristics));
            }

        private:
            uint16_t _handle;
            uint16_t _group;
            UUID _serviceId;
            std::list<Characteristic> _characteristics;
        };

    public:
        typedef std::function<void(const uint32_t)> Handler;
        typedef Core::IteratorType< const std::list<Service>, const Service&, std::list<Service>::const_iterator> Iterator;

        GATTProfile (const GATTProfile&) = delete;
        GATTProfile& operator= (const GATTProfile&) = delete;

        GATTProfile(const bool includeVendorCharacteristics)
            : _adminLock()
            , _services()
            , _index()
            , _custom(includeVendorCharacteristics)
            , _socket(nullptr)
            , _command()
            , _handler()
            , _expired(0) {
        }
        ~GATTProfile() {
        }

    public:
        uint32_t Discover(const uint32_t waitTime, GATTSocket& socket, const Handler& handler) {
            uint32_t result = Core::ERROR_INPROGRESS;

            _adminLock.Lock();
            if (_socket == nullptr) {
                result = Core::ERROR_NONE;
                _socket = &socket;
                _expired = Core::Time::Now().Add(waitTime).Ticks();
                _handler = handler;
                _services.clear();
                _command.ReadByGroupType(0x0001, 0xFFFF, UUID(PRIMARY_SERVICE_UUID));
                _socket->Execute(waitTime, _command, [&](const GATTSocket::Command& cmd) { OnServices(cmd); });
            }
            _adminLock.Unlock();

            return(result);
        }
        void Abort () {
            Report(Core::ERROR_ASYNC_ABORTED);
        }
        bool IsValid() const {
            return ((_services.size() > 0) && (_expired == Core::ERROR_NONE));
        }
        Iterator Services() const {
            return (Iterator(_services));
        }
        const Service* operator[] (const UUID& id) const {
            std::list<Service>::const_iterator index (_services.begin());

            while ((index != _services.end()) && (*index != id)) { index++; }

            return (index != _services.end() ? &(*index) : nullptr);
        }
        void Find(const UUID& serviceUuid, const UUID& charUuid, std::list<const Service::Characteristic*>& characteristics) const
        {
            const Service* service = (*this)[serviceUuid];
            if (service != nullptr) {
                auto it = service->Characteristics();
                while (it.Next() == true) {
                    if (it.Current() == charUuid) {
                        characteristics.push_back(&it.Current());
                    }
                }
            }
        }
        uint16_t FindHandle(const Service::Characteristic& characteristic, const UUID& descUuid) const
        {
            uint16_t handle = 0;
            const Service::Characteristic::Descriptor* descriptor = characteristic[descUuid];
            if (descriptor != nullptr) {
                handle = descriptor->Handle();
            }
            return (handle);
        }
        uint16_t FindHandle(const UUID& serviceUuid, const UUID& charUuid) const
        {
            const Service::Characteristic* characteristic = FindCharacteristic(serviceUuid, charUuid);
            return (characteristic == nullptr ? 0 : characteristic->Handle());
        }
        uint16_t FindHandle(const UUID& serviceUuid, const UUID& charUuid, const UUID& descUuid) const
        {
            uint16_t handle = 0;
            const Service::Characteristic* characteristic = FindCharacteristic(serviceUuid, charUuid);
            if (characteristic != nullptr) {
                const Service::Characteristic::Descriptor* descriptor = (*characteristic)[descUuid];
                if (descriptor != nullptr) {
                    handle = descriptor->Handle();
                }
            }
            return (handle);
        }

    private:
        const Service::Characteristic* FindCharacteristic(const UUID& serviceUuid, const UUID& charUuid) const
        {
            const Service::Characteristic* result = nullptr;
            const Service* service = (*this)[serviceUuid];
            if (service != nullptr) {
                result = (*service)[charUuid];
            }
            return (result);
        }
        std::list<Service>::iterator ValidService(const std::list<Service>::iterator& input) {
            std::list<Service>::iterator index (input);
            while ( (index != _services.end()) && 
                     ( (index->Handle() >= index->Max()) || ((_custom == false) && (index->Type().HasShort() == false)) ) ) {
                index++;
            }

            return (index);
        }
        bool NextCharacteristic() {
            do {
                if ( _characteristics.Next() == false) {
                    _index = ValidService(++_index);

                    if (_index != _services.end()) {
                        _characteristics = _index->Filler();
                    }
                }

            } while ((_index != _services.end()) && (_characteristics.IsValid() == false));

            return (_characteristics.IsValid());
        }
        void LoadCharacteristics(uint32_t waitTime) {
            uint16_t begin = _characteristics.Current().Handle();
            uint16_t end = _characteristics.Current().Max();

            _adminLock.Lock();

            if (_socket != nullptr) {
                if (begin < end){
                    _command.FindInformation(begin + 1, end);
                    _socket->Execute(waitTime, _command, [&](const GATTSocket::Command& cmd) { OnDescriptors(cmd); });
                }
                else {
                    _command.Read(_characteristics.Current().Handle());
                    _socket->Execute(waitTime, _command, [&](const GATTSocket::Command& cmd) { OnAttribute(cmd); });
                }
            }
            _adminLock.Unlock();
        }
        void OnServices(const GATTSocket::Command& cmd) {
            ASSERT (&cmd == &_command);

            if ( (cmd.Error() != Core::ERROR_NONE) && (cmd.Result().Error() != 0) ) {
                // Seems like the services could not be discovered, report it..
                Report(Core::ERROR_GENERAL);
            }
            else {
                uint32_t waitTime = AvailableTime();

                if (waitTime > 0) {
                    GATTSocket::Command::Response& response(_command.Result());

                    while (response.Next() == true) {
                        const uint8_t* service = response.Data();
                        if (response.Length() == 2) {
                            _services.emplace_back( UUID(service[0] | (service[1] << 8)), response.Handle(), response.Group() );
                        }
                        else if (response.Length() == 16) {
                            _services.emplace_back( UUID(service), response.Handle(), response.Group() );
                        }
                    }

                    if (_services.size() == 0) {
                        Report (Core::ERROR_UNAVAILABLE);
                    }
                    else {
                        _index = ValidService(_services.begin());

                        if (_index == _services.end()) {
                            Report (Core::ERROR_NONE);
                        }
                        else {
                            _adminLock.Lock();
                            if (_socket != nullptr) {
                                _command.ReadByType(_index->Handle()+1, _index->Max(), UUID(CHARACTERISTICS_UUID));
                                _socket->Execute(waitTime, _command, [&](const GATTSocket::Command& cmd) { OnCharacteristics(cmd); });
                            }
                            _adminLock.Unlock();
                        }
                    }
                }
            }
        }
        void OnCharacteristics(const GATTSocket::Command& cmd) {
            ASSERT (&cmd == &_command);

            if ( (cmd.Error() != Core::ERROR_NONE) && (cmd.Result().Error() != 0) ) {
                // Seems like the services could not be discovered, report it..
                Report(Core::ERROR_GENERAL);
            }
            else {
                uint32_t waitTime = AvailableTime();

                if (waitTime > 0) {
                    GATTSocket::Command::Response& response(_command.Result());

                    _index->AddCharacteristics(response);
                    _index = ValidService(++_index);

                    if (_index != _services.end()) {
                        _adminLock.Lock();
                        if (_socket != nullptr) {
                            _command.ReadByType(_index->Handle()+1, _index->Max(), UUID(CHARACTERISTICS_UUID));
                            _socket->Execute(waitTime, _command, [&](const GATTSocket::Command& cmd) { OnCharacteristics(cmd); });
                        }
                        _adminLock.Unlock();
                    }
                    else {

                        // Time to start reading the attributes on the services!!
                        _index = _services.begin();

                        // If we get here, there must be services, otherwise we would have bailed out on OnServices!! 
                        ASSERT (_index != _services.end());

                        _characteristics = _index->Filler();

                        if (NextCharacteristic() == false) {
                            Report(Core::ERROR_NONE);
                        }
                        else {
                            LoadCharacteristics(waitTime);
                       }
                    }
                }
            }
        }
        void OnDescriptors(const GATTSocket::Command& cmd) {
            ASSERT (&cmd == &_command);

            if ( (cmd.Error() != Core::ERROR_NONE) && (cmd.Result().Error() != 0) ) {
                // Seems like the services could not be discovered, report it..
                Report(Core::ERROR_GENERAL);
            }
            else {
                uint32_t waitTime = AvailableTime();

                if (waitTime > 0) {
                    _characteristics.Current().Descriptors(_command.Result());

                    _adminLock.Lock();

                    if (_socket != nullptr) {

                        _command.Read(_characteristics.Current().Handle());
                        _socket->Execute(waitTime, _command, [&](const GATTSocket::Command& cmd) { OnAttribute(cmd); });
                    }

                    _adminLock.Unlock();
                }
            }
        }
        void OnAttribute(const GATTSocket::Command& cmd) {
            ASSERT (&cmd == &_command);

            if (cmd.Error() != Core::ERROR_NONE) {
                // Seems like the services could not be discovered, report it..
                Report(Core::ERROR_GENERAL);
            }
            else {
                uint32_t waitTime = AvailableTime();

                if (waitTime > 0) {

                    _characteristics.Current().Value(_command.Result());

                    if (NextCharacteristic() == false) {
                        Report(Core::ERROR_NONE);
                    }
                    else {
                        LoadCharacteristics(waitTime);
                    }
                }
            }
        }
        void Report(const uint32_t result) {
            _adminLock.Lock();
            if (_socket != nullptr) {
                Handler caller = _handler;
                _socket = nullptr;
                _handler = nullptr;
                _expired = result;

                caller(result);
            }
            _adminLock.Unlock();
        }
        uint32_t AvailableTime () {
            uint64_t now = Core::Time::Now().Ticks();
            uint32_t result = (now >= _expired ? 0 : static_cast<uint32_t>((_expired - now) / Core::Time::TicksPerMillisecond));

            if (result == 0) {
                Report(Core::ERROR_TIMEDOUT);
            }
            return (result);
        }

    private:
        Core::CriticalSection _adminLock;
        std::list<Service> _services;
        std::list<Service>::iterator _index;
        Service::Index _characteristics;
        bool _custom;
        GATTSocket* _socket;
        GATTSocket::Command _command;
        Handler _handler;
        uint64_t _expired;
    };

} // namespace Bluetooth

} // namespace Thunder

