
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

#include "Definitions.h"
#include "ProgramTable.h"
#include "TunerAdministrator.h"

#include <linux/dvb/frontend.h>
#include <linux/dvb/dmx.h>
#include <linux/dvb/version.h>

#if (DVB_API_VERSION < 5)
#error "Not supported DVB API version"
#endif

#define __DEBUG__

// --------------------------------------------------------------------
// SOURCE: https://linuxtv.org/downloads/v4l-dvb-apis/uapi/dvb
// --------------------------------------------------------------------
namespace WPEFramework {
namespace Broadcast {

    struct conversion_entry {
        int from; // WPEFramework Broadcast/ITuner value
        int to; // LinuxDVB API value
    };

    static constexpr conversion_entry _tableSystemType[] = {
        { .from = ITuner::DVB | ITuner::Cable | ITuner::B, .to = SYS_DVBC_ANNEX_B },
        { .from = ITuner::DVB | ITuner::Cable | ITuner::C, .to = SYS_DVBC_ANNEX_C },
        { .from = ITuner::DVB | ITuner::Terrestrial | ITuner::NoAnnex, .to = SYS_DVBT },
        { .from = ITuner::DVB | ITuner::Terrestrial | ITuner::A, .to = SYS_DVBT2 },
        { .from = ITuner::DVB | ITuner::Satellite | ITuner::NoAnnex, .to = SYS_DVBS },
        { .from = ITuner::DVB | ITuner::Satellite | ITuner::A, .to = SYS_DVBS2 },
        { .from = ITuner::ISDB | ITuner::Satellite | ITuner::NoAnnex, .to = SYS_ISDBS },
        { .from = ITuner::ISDB | ITuner::Terrestrial | ITuner::NoAnnex, .to = SYS_ISDBT },
        { .from = ITuner::ISDB | ITuner::Cable | ITuner::NoAnnex, .to = SYS_ISDBC }
    };

    static constexpr conversion_entry _tableInversion[] = {
        { .from = Broadcast::Auto, .to = INVERSION_AUTO },
        { .from = Broadcast::Normal, .to = INVERSION_OFF },
        { .from = Broadcast::Inverted, .to = INVERSION_ON }
    };
    static constexpr conversion_entry _tableFEC[] = {
        { .from = Broadcast::FEC_INNER_NONE, .to = FEC_NONE },
        { .from = Broadcast::FEC_INNER_UNKNOWN, .to = FEC_AUTO },
        { .from = Broadcast::FEC_1_2, .to = FEC_1_2 },
        { .from = Broadcast::FEC_2_3, .to = FEC_2_3 },
        { .from = Broadcast::FEC_2_5, .to = FEC_2_5 },
        { .from = Broadcast::FEC_3_4, .to = FEC_3_4 },
        { .from = Broadcast::FEC_3_5, .to = FEC_3_5 },
        { .from = Broadcast::FEC_4_5, .to = FEC_4_5 },
        { .from = Broadcast::FEC_5_6, .to = FEC_5_6 },
        { .from = Broadcast::FEC_6_7, .to = FEC_6_7 },
        { .from = Broadcast::FEC_7_8, .to = FEC_7_8 },
        { .from = Broadcast::FEC_8_9, .to = FEC_8_9 },
        { .from = Broadcast::FEC_9_10, .to = FEC_9_10 }
    };
    static constexpr conversion_entry _tableModulation[] = {
        { .from = Broadcast::HORIZONTAL_QPSK, .to = QPSK },
        { .from = Broadcast::VERTICAL_QPSK, .to = QPSK },
        { .from = Broadcast::LEFT_QPSK, .to = QPSK },
        { .from = Broadcast::RIGHT_QPSK, .to = QPSK },
        { .from = Broadcast::HORIZONTAL_8PSK, .to = PSK_8 },
        { .from = Broadcast::VERTICAL_8PSK, .to = PSK_8 },
        { .from = Broadcast::LEFT_8PSK, .to = PSK_8 },
        { .from = Broadcast::RIGHT_8PSK, .to = PSK_8 },
        { .from = Broadcast::QAM16, .to = QAM_16 },
        { .from = Broadcast::QAM32, .to = QAM_32 },
        { .from = Broadcast::QAM64, .to = QAM_64 },
        { .from = Broadcast::QAM128, .to = QAM_128 },
        { .from = Broadcast::QAM256, .to = QAM_256 },
    };
    static constexpr conversion_entry _tableTransmission[] = {
        { .from = Broadcast::TRANSMISSION_AUTO, .to = TRANSMISSION_MODE_AUTO },
        { .from = Broadcast::TRANSMISSION_1K, .to = TRANSMISSION_MODE_1K },
        { .from = Broadcast::TRANSMISSION_2K, .to = TRANSMISSION_MODE_2K },
        { .from = Broadcast::TRANSMISSION_4K, .to = TRANSMISSION_MODE_4K },
        { .from = Broadcast::TRANSMISSION_8K, .to = TRANSMISSION_MODE_8K },
        { .from = Broadcast::TRANSMISSION_16K, .to = TRANSMISSION_MODE_16K },
        { .from = Broadcast::TRANSMISSION_32K, .to = TRANSMISSION_MODE_32K },
        { .from = Broadcast::TRANSMISSION_C3780, .to = TRANSMISSION_MODE_C3780 },
        { .from = Broadcast::TRANSMISSION_C1, .to = TRANSMISSION_MODE_C1 }
    };
    static constexpr conversion_entry _tableGuard[] = {
        { .from = Broadcast::GUARD_AUTO, .to = GUARD_INTERVAL_AUTO },
        { .from = Broadcast::GUARD_1_4, .to = GUARD_INTERVAL_1_4 },
        { .from = Broadcast::GUARD_1_8, .to = GUARD_INTERVAL_1_8 },
        { .from = Broadcast::GUARD_1_16, .to = GUARD_INTERVAL_1_16 },
        { .from = Broadcast::GUARD_1_32, .to = GUARD_INTERVAL_1_32 },
        { .from = Broadcast::GUARD_1_128, .to = GUARD_INTERVAL_1_128 },
        { .from = Broadcast::GUARD_19_128, .to = GUARD_INTERVAL_19_128 },
        { .from = Broadcast::GUARD_19_256, .to = GUARD_INTERVAL_19_256 }
    };
    static constexpr conversion_entry _tableHierarchy[] = {
        { .from = Broadcast::NoHierarchy, .to = HIERARCHY_NONE },
        { .from = Broadcast::AutoHierarchy, .to = HIERARCHY_AUTO },
        { .from = Broadcast::Hierarchy1, .to = HIERARCHY_1 },
        { .from = Broadcast::Hierarchy2, .to = HIERARCHY_2 },
        { .from = Broadcast::Hierarchy4, .to = HIERARCHY_4 }
    };
    /*
static constexpr conversion_entry _tablePilot[] = {
    { .from = Broadcast::PILOT_AUTO,              .to = PILOT_AUTO },
    { .from = Broadcast::PILOT_ON,                .to = PILOT_ON   },
    { .from = Broadcast::PILOT_OFF,               .to = PILOT_OFF  }
};
static constexpr conversion_entry _tableRollOff[] = {
    { .from = Broadcast::HIERARCHY_AUTO,          .to = ROLLOFF_AUTO },
    { .from = Broadcast::ROLLOFF_20,              .to = ROLLOFF_20   },
    { .from = Broadcast::ROLLOFF_25,              .to = ROLLOFF_25   },
    { .from = Broadcast::ROLLOFF_35,              .to = ROLLOFF_35   }
};
*/

    template <size_t N>
    int Convert(const conversion_entry (&table)[N], const int from, const int ifnotfound)
    {
        uint16_t index = 0;
        while ((index < N) && (from != table[index].from)) {
            index++;
        }
        return (index < N ? table[index].to : ifnotfound);
    }

    void Property(dtv_property& property, const int command, const int value)
    {
        property.cmd = command;
        property.u.data = value;
    }

    static uint16_t IndexToFrontend(const uint8_t /* index */)
    {
        uint8_t adapter = 0;
        uint8_t frontend = 0;

        // Count the number of frontends you have per adapter, substract those from the index..

        return ((adapter << 8) | frontend);
    }

    class __attribute__((visibility("hidden"))) Tuner : public ITuner  {
    private:
        Tuner() = delete;
        Tuner(const Tuner&) = delete;
        Tuner& operator=(const Tuner&) = delete;

        class Observer : public Core::Thread {
        public:
            Observer(const Observer&) = delete;
            Observer& operator=(const Observer&) = delete;

            Observer() : Core::Thread(Thread::DefaultStackSize(), _T("Tuner")), _adminLock(), _entries() {
            }
            ~Observer() override {
            }

        public:
            static Observer& Instance()
            {
                static Observer& _instance = Core::SingletonType<Observer>::Instance();
                return (_instance);
            }
            void Register(Tuner& callback) {
                _adminLock.Lock();
                ASSERT (std::find(_entries.begin(), _entries.end(), &callback) == _entries.end());
                _entries.push_back(&callback);
                Thread::Run();
                _adminLock.Unlock();
            }
            void Unregister(Tuner& callback) {
                _adminLock.Lock();
                std::list<Tuner*>::iterator index (std::find(_entries.begin(), _entries.end(), &callback));
                if (index != _entries.end()) {
                    _entries.erase(index);
                }
                _adminLock.Unlock();
            }

        private:
            uint32_t Worker() override {
                Block();
                _adminLock.Lock();
                std::list<Tuner*>::iterator index (_entries.begin());
                uint32_t result = (_entries.size() > 0 ? 100 : Core::infinite); // Slots of 100 mS;
                while (index != _entries.end()) {
                    if ((*index)->Dispatch() == true) {
                        index = _entries.erase(index);
                    }
                    else {
                        index++;
                    }
                }
                _adminLock.Unlock();
                return (result);
            }

        private:
            Core::CriticalSection _adminLock;
            std::list<Tuner*> _entries;
        };
        class MuxFilter : public Core::IResource {
        public:
            MuxFilter() = delete;
            MuxFilter(const MuxFilter&) = delete;
            MuxFilter& operator= (const MuxFilter&) = delete;

            MuxFilter(const string& path, const uint8_t index, const uint16_t pid, const uint8_t tableId, ISection* callback) 
                : _mux(-1)
                , _offset(0)
                , _length(0)
                , _size(1024)
                , _buffer(reinterpret_cast<uint8_t*>(::malloc(_size)))
                , _callback(callback) {

                char deviceName[32];

                strncpy (deviceName, path.c_str(), sizeof(deviceName));
              
                ::snprintf(&(deviceName[path.length()]), (sizeof(deviceName) - path.length()), "demux%d", index);

                _mux = open(deviceName, O_RDWR|O_NONBLOCK);

                if (_mux == -1) {
                    TRACE_L1("Could not open the filter[%s]: %d\n", deviceName, errno);
                }
                else {
                
                    struct dmx_sct_filter_params sctFilterParams;

                    sctFilterParams.pid = pid;
                    ::memset(&sctFilterParams.filter, 0, sizeof(sctFilterParams.filter));
                    sctFilterParams.timeout = 0;
                    sctFilterParams.flags = DMX_IMMEDIATE_START|DMX_CHECK_CRC;
                    sctFilterParams.filter.filter[0] = tableId;
                    sctFilterParams.filter.mask[0] = 0xFF;

                    if (ioctl(_mux, DMX_SET_FILTER, &sctFilterParams) < 0) {
                        TRACE_L1("Could not configue the filter[%d,%d]: %d\n", pid, tableId, errno);
                        ::close(_mux);
                        _mux = -1;
                    }
                    else {
                        Core::ResourceMonitor::Instance().Register(*this);
                    }
                }
            }
            ~MuxFilter() {
                if (_mux != -1) {
                    Core::ResourceMonitor::Instance().Unregister(*this);
                    ::close(_mux);
                }
                ::free(_buffer);
            }

        public:
            bool IsValid() const {
                return (_mux != -1);
            }
            handle Descriptor() const override {
                return (_mux);
            }
            uint16_t Events() override {
                return (POLLPRI);
            }
            void Handle(const uint16_t events) override {
               if (LoadHeader() == true) {
                    int loaded = ::read(_mux, &(_buffer[_offset]), (_length - (_offset - 3)));
                    if (loaded < 0) {
                        int result = errno;
                        if ((result != EAGAIN) || (result != EWOULDBLOCK)) {
                            _offset = 0;
                        }
                    }
                    else {
                        _offset += loaded;
                        if ((_offset - 3) == _length) {
                            MPEG::Section newSection(Core::DataElement(_offset, _buffer));
                            _callback->Handle(newSection);
                            _offset = 0;
                        }
                    }
                }
            }

        private:
            bool LoadHeader() 
            {
                bool moreToLoad = true;
                if (_offset < 3) {
                    int loaded = ::read(_mux, &(_buffer[_offset]), (3 - _offset));
                    if (loaded < 0) {
                        int result = errno;
                        if ((result != EAGAIN) || (result != EWOULDBLOCK)) {
                            _offset = 0;
                        }
                        moreToLoad = false;
                    }
                    else {
                        _offset += loaded;
                        if (_offset != 3) {
                            moreToLoad = false;
                        }
                        else {
                            _length = (_buffer[1] << 8) | (_buffer[2] & 0xFF);
                            if ((_length + 3) > _size) {
                                uint8_t copy = _buffer[0];
                                ::free (_buffer);
                                _size = _length + 3;
                                _buffer = reinterpret_cast<uint8_t*>(::malloc(_size));
                                _buffer[0] = copy;
                                _buffer[1] = (_length >> 8) & 0xFF;
                                _buffer[2] = _length & 0xFF;
                            }
                        }
                    }
                }
                return (moreToLoad);
            }
 

        private:
            int _mux;
            uint16_t _offset;
            uint16_t _length;
            uint16_t _size;
            uint8_t* _buffer;
            ISection* _callback;
        };

    public:
        class Information {
        private:
            Information(const Information&) = delete;
            Information& operator=(const Information&) = delete;

        private:
            class Config : public Core::JSON::Container {
            private:
                Config(const Config&);
                Config& operator=(const Config&);

            public:
                Config()
                    : Core::JSON::Container()
                    , Frontends(1)
                    , Decoders(1)
                    , Standard(ITuner::DVB)
                    , Annex(ITuner::A)
                    , Modus(ITuner::Terrestrial)
                    , Scan(false)
                    , Callsign("Streamer")
                {
                    Add(_T("frontends"), &Frontends);
                    Add(_T("decoders"), &Decoders);
                    Add(_T("standard"), &Standard);
                    Add(_T("annex"), &Annex);
                    Add(_T("modus"), &Modus); 
                    Add(_T("scan"), &Scan);
                    Add(_T("callsign"), &Callsign);
                }
                ~Config()
                {
                }

            public:
                Core::JSON::DecUInt8 Frontends;
                Core::JSON::DecUInt8 Decoders;
                Core::JSON::EnumType<ITuner::DTVStandard> Standard;
                Core::JSON::EnumType<ITuner::annex> Annex;
                Core::JSON::EnumType<ITuner::modus> Modus; 
                Core::JSON::Boolean Scan;
                Core::JSON::String Callsign;
            };

            Information()
                : _frontends(0)
                , _standard()
                , _annex()
                , _modus()
                , _type(SYS_UNDEFINED)
                , _scan(false)
            {
            }

        public:
            static Information& Instance()
            {
                return (_instance);
            }
            ~Information()
            {
            }
            void Initialize(const string& configuration)
            {

                Config config;
                config.FromString(configuration);

                _frontends = config.Frontends.Value();
                _standard = config.Standard.Value();
                _annex = config.Annex.Value();
                _scan = config.Scan.Value();
                _modus = config.Modus.Value();

                _type = Convert(_tableSystemType, _standard | _modus | _annex, SYS_UNDEFINED);

                ASSERT(_type != SYS_UNDEFINED);

            }
            void Deinitialize()
            {
            }

        public:
            inline bool IsSupported(const ITuner::modus mode)
            {
                return ((_type != SYS_UNDEFINED) && (mode == _modus));
            }
            inline ITuner::DTVStandard Standard() const
            {
                return (_standard);
            }
            inline ITuner::annex Annex() const
            {
                return (_annex);
            }
            inline ITuner::modus Modus() const
            {
                return (_modus);
            }
            inline int Type() const
            {
                return (_type);
            }
            inline bool Scan() const
            {
                return (_scan);
            }

        private:
            uint8_t _frontends;
            ITuner::DTVStandard _standard;
            ITuner::annex _annex;
            ITuner::modus _modus;
            int _type;
            bool _scan;

            static Information _instance;
        };

    private:
        Tuner(uint8_t index, Broadcast::transmission transmission = Broadcast::TRANSMISSION_AUTO, Broadcast::guard guard = Broadcast::GUARD_AUTO, Broadcast::hierarchy hierarchy = Broadcast::AutoHierarchy)
            : _state(IDLE)
            , _frontend()
            , _transmission()
            , _guard()
            , _hierarchy()
            , _info({ 0 })
            , _devicePath()
            , _frontindex(0)
            , _callback(nullptr)
        {
            _callback = TunerAdministrator::Instance().Announce(this);
            if (Tuner::Information::Instance().Type() != SYS_UNDEFINED) {
                char deviceName[32];

                uint16_t info = IndexToFrontend(index);
                _frontindex = (info & 0xFF);

                ::snprintf(deviceName, sizeof(deviceName), "/dev/dvb/adapter%d/", (info >> 8));

                _devicePath = deviceName;
              
                ::snprintf(&(deviceName[_devicePath.length()]), (sizeof(deviceName) - _devicePath.length()), "frontend%d", _frontindex);

                _frontend = open(deviceName, O_RDWR);

                if (_frontend != -1) {
                    if (::ioctl(_frontend, FE_GET_INFO, &_info) == -1) {
                        TRACE_L1("Can not get information about the frontend. Error %d.", errno);
                        close(_frontend);
                        _frontend = -1;
                    }
                    TRACE_L1("Opened frontend %s. Second Generation Support: %s", _info.name, (IsSecondGeneration() ? _T("true") : _T("false")));
                    TRACE_L1("Support auto FEC: %s", (HasAutoFEC() ? _T("true") : _T("false")));
                } else {
                    TRACE_L1("Can not open frontend %s error: %d.", deviceName, errno);
                }

                _transmission = Convert(_tableTransmission, transmission, TRANSMISSION_MODE_AUTO);
                _guard = Convert(_tableGuard, guard, GUARD_INTERVAL_AUTO);
                _hierarchy = Convert(_tableHierarchy, hierarchy, HIERARCHY_AUTO);                
            }
        }

    public:
        ~Tuner()
        {
            TunerAdministrator::Instance().Revoke(this);

            Observer::Instance().Unregister(*this);

            Detach(0);

            if (_frontend != -1) {
                close(_frontend);
            }

            _callback = nullptr;
        }

        static ITuner* Create(const string& info)
        {
            Tuner* result = nullptr;

            uint8_t index = Core::NumberType<uint8_t>(Core::TextFragment(info)).Value();

            result = new Tuner(index);

            if ((result != nullptr) && (result->IsValid() == false)) {
                delete result;
                result = nullptr;
            }

            return (result);
        }

    public:
        bool HasAutoFEC() const {
            return ((_info.caps & FE_CAN_FEC_AUTO) != 0);
        }
        bool IsSecondGeneration() const {
            return ((_info.caps & FE_CAN_2G_MODULATION) != 0);
        }
        bool IsValid() const
        {
            return (_frontend != -1);
        }
        const char* Name() const
        {
            return (_info.name);
        }

        virtual uint32_t Properties() const override
        {
            Information& instance = Information::Instance();
            return (instance.Annex() | instance.Standard() |
            #ifdef SATELITE
            ITuner::modus::Satellite 
            #else
            ITuner::modus::Terrestrial
            #endif
            );

            // ITuner::modus::Cable
        }

        // Currently locked on ID
        // This method return a unique number that will identify the locked on Transport stream. The ID will always
        // identify the uniquely locked on to Tune request. ID => 0 is reserved and means not locked on to anything.
        virtual uint16_t Id() const override
        {
            return 0; //(_state == IDLE ? 0 : _settings.frequency / 1000000);
        }

        // Using these methods the state of the tuner can be viewed.
        // IDLE:      Means there is no request, or the frequency requested (with other parameters) can not be locked.
        // LOCKED:    The stream has been locked, frequency, modulation, symbolrate and spectral inversion seem to be fine.
        // PREPARED:  The program that was requetsed to prepare fore, has been found in PAT/PMT, the needed information,
        //            like PIDS is loaded. If Priming is available, this means that the priming has started!
        // STREAMING: This means that the requested program is being streamed to decoder or file, depending on implementation/inpuy.
        virtual state State() const override
        {
            return _state;
        }

        // Using the next method, the allocated Frontend will try to lock the channel that is found at the given parameters.
        // Frequency is always in MHz.
        virtual uint32_t Tune(const uint16_t frequency, const Modulation modulation, const uint32_t symbolRate, const uint16_t fec, const SpectralInversion inversion) override
        {
            uint8_t propertyCount;
            struct dtv_property props[16];
            ::memset(&props, 0, sizeof(props));

            Property(props[0], DTV_DELIVERY_SYSTEM, Tuner::Information::Instance().Type());
            Property(props[1], DTV_FREQUENCY, frequency * 1000000);
            Property(props[2], DTV_MODULATION, Convert(_tableModulation, modulation, QAM_64));
            Property(props[3], DTV_INVERSION, Convert(_tableInversion, inversion, INVERSION_AUTO));
            Property(props[4], DTV_SYMBOL_RATE, symbolRate);
            Property(props[5], DTV_INNER_FEC, Convert(_tableFEC, fec, FEC_AUTO));
            propertyCount = 6;

            if (Tuner::Information::Instance().Modus() == ITuner::Terrestrial){
                Property(props[6], DTV_BANDWIDTH_HZ, symbolRate);
                Property(props[7], DTV_CODE_RATE_HP, Convert(_tableFEC, (fec & 0xFF), FEC_AUTO));
                Property(props[8], DTV_CODE_RATE_LP, Convert(_tableFEC, ((fec >> 8) & 0xFF), FEC_AUTO));
                Property(props[9], DTV_TRANSMISSION_MODE, _transmission);
                Property(props[10], DTV_GUARD_INTERVAL, _guard);
                Property(props[11], DTV_HIERARCHY, _hierarchy);
                propertyCount = 12;
            }

            Property(props[propertyCount++], DTV_TUNE, 0);

            struct dtv_properties dtv_prop = {
                .num = propertyCount, .props = props
            };

            _state = IDLE;

            if (ioctl(_frontend, FE_SET_PROPERTY, &dtv_prop) < 0) {
                perror("ioctl");
            } else {
                #ifdef __DEBUG__
                TRACE_L1("Tuning request send out !!!");
                TRACE_L1("    Delivery:       %d ", props[0].u.data);
                TRACE_L1("    Frequency:      %d Hz", props[1].u.data);
                TRACE_L1("    Modulation:     %d ", props[2].u.data);
                TRACE_L1("    Inversion:      %d ", props[3].u.data);
                TRACE_L1("    Symbolrate:     %d ", props[4].u.data);
                TRACE_L1("    Inner FEC:      %d ", props[5].u.data);
                if (propertyCount > 6) {
                    TRACE_L1("    Bandwidth:      %d ", props[6].u.data);
                    TRACE_L1("    Coderate HP:    %d ", props[7].u.data);
                    TRACE_L1("    Coderate LP:    %d ", props[8].u.data);
                    TRACE_L1("    Transmission:   %d ", props[9].u.data);
                    TRACE_L1("    Guard Interval: %d ", props[10].u.data);
                    TRACE_L1("    Hierarchy:      %d ", props[11].u.data);
                }
                _lastState = 0;
                #endif
                Observer::Instance().Register(*this);
            }

            return (Core::ERROR_NONE);
        }

        // In case the tuner needs to be tuned to a specific programId, please list it here. Once the PID's associated to this
        // programId have been found, and set, the Tuner will reach its PREPARED state.
        virtual uint32_t Prepare(const uint16_t programId) override
        {
            TRACE_L1("%s:%d %s", __FILE__, __LINE__, __FUNCTION__);
            return 0;
        }

        // A Tuner can be used to filter PSI/SI. Using the next call a callback can be installed to receive sections associated
        // with a table. Each valid section received will be offered as a single section on the ISection interface for the user
        // to process.
        virtual uint32_t Filter(const uint16_t pid, const uint8_t tableId, ISection* callback) override
        {
            uint32_t result = Core::ERROR_UNAVAILABLE; 
            uint32_t id = (pid << 16) | tableId; 
 
            _state.Lock();

            if (callback != nullptr) { 
                auto entry = _filters.emplace(
                                 std::piecewise_construct, 
                                 std::forward_as_tuple(id), 
                                 std::forward_as_tuple(_devicePath, _frontindex, pid, tableId, callback)); 

                if (entry.first->second.IsValid() == true) {
                    result = Core::ERROR_NONE;
                }
                else {
                    _filters.erase(entry.first);
                    result =  Core::ERROR_GENERAL;
                }
            } else { 
                std::map<uint32_t, MuxFilter>::iterator index(_filters.find(id)); 
                if (index != _filters.end()) { 
                    _filters.erase(index); 
                    result = Core::ERROR_NONE; 
                } 
            } 

            _state.Unlock();

            return (result); 
        } 
        // Using the next two methods, the frontends will be hooked up to decoders or file, and be removed from a decoder or file.
        virtual uint32_t Attach(const uint8_t index) override
        {
            TRACE_L1("%s:%d %s\n", __FILE__, __LINE__, __FUNCTION__);
            return 0;
        }
        virtual uint32_t Detach(const uint8_t index) override
        {
            TRACE_L1("%s:%d %s\n", __FILE__, __LINE__, __FUNCTION__);
            return 0;
        }

    private:
        void Lock() {
            _state.Lock();
        }
        void Unlock() {
            _state.Unlock();
        }
        bool Dispatch() {
            bool remove = false;
            unsigned int status;

            // IDLE = 0x01,
            //   FE_TIMEDOUT No lock within the last about 2 seconds.
            // LOCKED = 0x02,
            //   FE_HAS_CARRIER Has found a signal.
            //   FE_HAS_LOCK Digital TV were locked and everything is working.
            //   FE_HAS_SIGNAL Has found something above the noise level.
            //   FE_HAS_SYNC Synchronization bytes was found.
            //   FE_HAS_VITERBI FEC inner coding (Viterbi, LDPC or other inner code). is stable.
            //   FE_REINIT Frontend was reinitialized, application is recommended to reset DiSEqC, tone and parameters.
            // PREPARED = 0x04,
            // STREAMING = 0x08

            if (::ioctl(_frontend, FE_READ_STATUS, &status) < 0) {
                TRACE_L1("Status could not be read!. Error: %d", errno);
            }
            else {
                TRACE_L1("FE_HAS_LOCK: %s", status & FE_HAS_LOCK ? _T("true") : _T("false"));
                TRACE_L1("FE_TIMEDOUT: %s", status & FE_TIMEDOUT ? _T("true") : _T("false"));

                if ((status & FE_HAS_LOCK) != 0) {
                    remove = true;
                    _state = ITuner::LOCKED;
                }
                else if ((status & FE_TIMEDOUT) != 0) {
                    _state = ITuner::IDLE;
                    remove = true;
                }

                #ifdef __DEBUG__
                if ((status & FE_HAS_LOCK) != 0) {
                    uint16_t snr, signal;
                    uint32_t ber, uncorrected_blocks;

                    TRACE_L1("Signal,SNR,BER,UNC,Status: %d,%d,%d,%d,%d", signal, snr, ber, uncorrected_blocks, status);

                    if (::ioctl(_frontend, FE_READ_SIGNAL_STRENGTH, &signal) < 0) {
                        signal = ~0;
                    }
                    if (::ioctl(_frontend, FE_READ_SNR, &snr) < 0) {
                        snr = ~0;
                    }
                    if (::ioctl(_frontend, FE_READ_BER, &ber) < 0) {
                        ber = ~0;
                    }
                    if (::ioctl(_frontend, FE_READ_UNCORRECTED_BLOCKS, &uncorrected_blocks) < 0) {
                        uncorrected_blocks = ~0;
                    }

                    unsigned int delta = _lastState ^ status;
                    if (delta & FE_HAS_SIGNAL)  TRACE_L1 ("FE_HAS_SIGNAL:  %s", status & FE_HAS_SIGNAL  ? _T("true") : _T("false"));
                    if (delta & FE_HAS_CARRIER) TRACE_L1 ("FE_HAS_CARRIER: %s", status & FE_HAS_CARRIER ? _T("true") : _T("false"));
                    if (delta & FE_HAS_VITERBI) TRACE_L1 ("FE_HAS_VITERBI: %s", status & FE_HAS_VITERBI ? _T("true") : _T("false"));
                    if (delta & FE_HAS_SYNC)    TRACE_L1 ("FE_HAS_SYNC:    %s", status & FE_HAS_SYNC    ? _T("true") : _T("false"));
                    if (delta & FE_TIMEDOUT)    TRACE_L1 ("FE_TIMEDOUT:    %s", status & FE_TIMEDOUT    ? _T("true") : _T("false"));
                    if (delta & FE_REINIT)      TRACE_L1 ("FE_REINIT:      %s", status & FE_REINIT      ? _T("true") : _T("false"));
                    if (delta & FE_HAS_LOCK)    TRACE_L1 ("FE_HAS_LOCK:    %s", status & FE_HAS_LOCK    ? _T("true") : _T("false"));
                    _lastState = status;
                }
                #endif
            }

            return (remove);
        }

    private:
        Core::StateTrigger<state> _state;
        int _frontend;
        int _transmission;
        int _guard;
        int _hierarchy;
        struct dvb_frontend_info _info;
        std::map<uint32_t,MuxFilter> _filters;
        string _devicePath;
        uint8_t _frontindex;
        TunerAdministrator::ICallback* _callback;
        #ifdef __DEBUG__
        unsigned int _lastState;
        #endif
    };

    /* static */ Tuner::Information Tuner::Information::_instance;

    // The following methods will be called before any create is called. It allows for an initialization,
    // if requires, and a deinitialization, if the Tuners will no longer be used.
    /* static */ uint32_t ITuner::Initialize(const string& configuration)
    {
        Tuner::Information::Instance().Initialize(configuration);

        return (Core::ERROR_NONE);
    }

    /* static */ uint32_t ITuner::Deinitialize()
    {
        Tuner::Information::Instance().Deinitialize();
        return (Core::ERROR_NONE);
    }

    // See if the tuner supports the requested mode, or is configured for the requested mode. This method 
    // only returns proper values if the Initialize has been called before.
    /* static */ bool ITuner::IsSupported(const ITuner::modus mode)
    {
        return (Tuner::Information::Instance().IsSupported(mode));
    }

    // Accessor to create a tuner.
    /* static */ ITuner* ITuner::Create(const string& configuration)
    {
        return (Tuner::Create(configuration));
    }

} // namespace Broadcast
} // namespace WPEFramework
