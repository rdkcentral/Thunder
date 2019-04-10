#include "Definitions.h"
#include "ProgramTable.h"
#include "TunerAdministrator.h"

#include <linux/dvb/frontend.h>

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
    static constexpr conversion_entry _tableBandwidth[] = {
        { .from = 0, .to = BANDWIDTH_AUTO },
        { .from = 1712000, .to = BANDWIDTH_1_712_MHZ },
        { .from = 5000000, .to = BANDWIDTH_5_MHZ },
        { .from = 6000000, .to = BANDWIDTH_6_MHZ },
        { .from = 7000000, .to = BANDWIDTH_7_MHZ },
        { .from = 8000000, .to = BANDWIDTH_8_MHZ },
        { .from = 10000000, .to = BANDWIDTH_10_MHZ }
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

    class __attribute__((visibility("hidden"))) Tuner : public ITuner {
    private:
        Tuner() = delete;
        Tuner(const Tuner&) = delete;
        Tuner& operator=(const Tuner&) = delete;

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
                Core::JSON::EnumType<ITuner::Annex> Annex;
                Core::JSON::EnumType<ITuner::Modus> Modus; 
                Core::JSON::Boolean Scan;
                Core::JSON::String Callsign;
            };

            Information()
                : _frontends(0)
                , _standard()
                , _annex()
                , _modus()
                , _type()
                , _scan(false)
            {
            }

        public:
            static Information& Instance()
            {
                printf("%s:%d %s\n", __FILE__, __LINE__, __FUNCTION__);
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

                printf("%s:%d %s\n", __FILE__, __LINE__, __FUNCTION__);
            }
            void Deinitialize()
            {
            }

        public:
            inline ITuner::DTVStandard Standard() const
            {
                return (_standard);
            }
            inline ITuner::Annex Annex() const
            {
                return (_annex);
            }
            inline ITuner::Modus Modus() const
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
            ITuner::Annex _annex;
            ITuner::Modus _modus;
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
            , _callback(nullptr)
        {
            _callback = TunerAdministrator::Instance().Announce(this);
printf("%s:%d %s\n", __FILE__, __LINE__, __FUNCTION__);
            if (Tuner::Information::Instance().Type() != SYS_UNDEFINED) {
                char deviceName[32];

                uint16_t info = IndexToFrontend(index);

                ::snprintf(deviceName, sizeof(deviceName), "/dev/dvb/adapter%d/frontend%d", (info >> 8), (info & 0xFF));

                _frontend = open(deviceName, O_RDWR);

                //ASSERT(_frontend != -1)

                if (_frontend != -1) {
                    printf("%s:%d %s\n", __FILE__, __LINE__, __FUNCTION__);
                    if (::ioctl(_frontend, FE_GET_INFO, &_info) == -1) {
                        TRACE_L1("Can not get information about the frontend. Error %d.", errno);
                        close(_frontend);
                        _frontend = -1;
                        printf("%s:%d %s\n", __FILE__, __LINE__, __FUNCTION__);
                    }
                    TRACE_L1("Opened frontend %s.", _info.name);
                    printf("%s:%d %s\n", __FILE__, __LINE__, __FUNCTION__);
                } else {
                    TRACE_L1("Can not open frontend %s error: %d.", deviceName, errno);
                    printf("%s:%d %s\n", __FILE__, __LINE__, __FUNCTION__);
                }

printf("%s:%d %s\n", __FILE__, __LINE__, __FUNCTION__);
                _transmission = Convert(_tableTransmission, transmission, TRANSMISSION_MODE_AUTO);
                _guard = Convert(_tableGuard, guard, GUARD_INTERVAL_AUTO);
                _hierarchy = Convert(_tableHierarchy, hierarchy, HIERARCHY_AUTO);                
            }
        }

    public:
        ~Tuner()
        {
            TunerAdministrator::Instance().Revoke(this);

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

printf("%s:%d %s\n", __FILE__, __LINE__, __FUNCTION__);

            result = new Tuner(index);

printf("%s:%d %s\n", __FILE__, __LINE__, __FUNCTION__);

            if ((result != nullptr) && (result->IsValid() == false)) {
                delete result;
                result = nullptr;
            }
printf("%s:%d %s\n", __FILE__, __LINE__, __FUNCTION__);
            return (result);
        }

    public:
        bool IsValid() const
        {
            return (_frontend != -1);
        }
        const char* Name() const
        {
            return (_info.name);
        }

        // Currently locked on ID
        // This method return a unique number that will identify the locked on Transport stream. The ID will always
        // identify the uniquely locked on to Tune request. ID => 0 is reserved and means not locked on to anything.
        virtual uint16_t Id() const override
        {
            printf("%s:%d %s\n", __FILE__, __LINE__, __FUNCTION__);
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
            printf("%s:%d %s\n", __FILE__, __LINE__, __FUNCTION__);
            return _state;
        }

        // Using the next method, the allocated Frontend will try to lock the channel that is found at the given parameters.
        // Frequency is always in MHz.
        virtual uint32_t Tune(const uint16_t frequency, const Modulation modulation, const uint32_t symbolRate, const uint16_t fec, const SpectralInversion inversion) override
        {
            printf("%s:%d %s\n", __FILE__, __LINE__, __FUNCTION__);
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
                Property(props[6], DTV_BANDWIDTH_HZ, Convert(_tableBandwidth, symbolRate, BANDWIDTH_AUTO));
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

            printf("%s:%d %s\n", __FILE__, __LINE__, __FUNCTION__);

            if (ioctl(_frontend, FE_SET_PROPERTY, &dtv_prop) == -1) {
                perror("ioctl");
            } else {
                TRACE_L1("Tuning request send out !!!\n");
            }

            printf("%s:%d %s\n", __FILE__, __LINE__, __FUNCTION__);

            return (Core::ERROR_NONE);
        }

        // In case the tuner needs to be tuned to s apecific programId, please list it here. Once the PID's associated to this
        // programId have been found, and set, the Tuner will reach its PREPARED state.
        virtual uint32_t Prepare(const uint16_t programId) override
        {
            printf("%s:%d %s\n", __FILE__, __LINE__, __FUNCTION__);
            return 0;
        }

        // A Tuner can be used to filter PSI/SI. Using the next call a callback can be installed to receive sections associated
        // with a table. Each valid section received will be offered as a single section on the ISection interface for the user
        // to process.
        virtual uint32_t Filter(const uint16_t pid, const uint8_t tableId, ISection* callback) override
        {
            printf("%s:%d %s\n", __FILE__, __LINE__, __FUNCTION__);
            return 0;
        }

        // Using the next two methods, the frontends will be hooked up to decoders or file, and be removed from a decoder or file.
        virtual uint32_t Attach(const uint8_t index) override
        {
            printf("%s:%d %s\n", __FILE__, __LINE__, __FUNCTION__);
            return 0;
        }
        virtual uint32_t Detach(const uint8_t index) override
        {
            printf("%s:%d %s\n", __FILE__, __LINE__, __FUNCTION__);
            return 0;
        }

    private:
        Core::StateTrigger<state> _state;
        int _frontend;
        int _transmission;
        int _guard;
        int _hierarchy;
        struct dvb_frontend_info _info;
        TunerAdministrator::ICallback* _callback;
    };

    /* static */ Tuner::Information Tuner::Information::_instance;

    // The following methods will be called before any create is called. It allows for an initialization,
    // if requires, and a deinitialization, if the Tuners will no longer be used.
    /* static */ uint32_t ITuner::Initialize(const string& configuration)
    {
        printf("%s:%d %s\n", __FILE__, __LINE__, __FUNCTION__);
        Tuner::Information::Instance().Initialize(configuration);
        return (Core::ERROR_NONE);
    }

    /* static */ uint32_t ITuner::Deinitialize()
    {
        printf("%s:%d %s\n", __FILE__, __LINE__, __FUNCTION__);
        Tuner::Information::Instance().Deinitialize();
        return (Core::ERROR_NONE);
    }

    // Accessor to create a tuner.
    /* static */ ITuner* ITuner::Create(const string& configuration)
    {
        printf("%s:%d %s\n", __FILE__, __LINE__, __FUNCTION__);
        return (Tuner::Create(configuration));
        printf("%s:%d %s\n", __FILE__, __LINE__, __FUNCTION__);
    }
} // namespace Broadcast
} // namespace WPEFramework
