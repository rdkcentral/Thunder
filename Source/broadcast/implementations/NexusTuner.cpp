#include "Definitions.h"
#include "ProgramTable.h"
#include "TunerAdministrator.h"

#include <nexus_audio_decoder.h>
#include <nexus_config.h>
#include <nexus_display.h>
#include <nexus_frontend.h>
#include <nexus_message.h>
#include <nexus_parser_band.h>
#include <nexus_platform.h>
#include <nexus_simple_audio_decoder.h>
#include <nexus_simple_video_decoder.h>
#include <nexus_simple_video_decoder_primer.h>
#include <nexus_video_decoder_primer.h>
#include <nexus_video_window.h>
#include <nxclient.h>
#include <ts_psi.h>

namespace WPEFramework {

namespace Broadcast {

    // Frequency for tuning...
    // VHF: 30MHz - 300MHz
    // UHF: 300MHz - 3GHz
    // SHF: 3GHz - 30 GHz
    // So the frequency, should be passed in MHz wich requires a range from 30MHz - 30000MHz. Which
    // does not exceed the uint16_t value, so the key (uint32_t) should be sufficient to combine the
    // Frequency and the ProgramNumber.

    static const unsigned J83ASymbolRateUpper = 7200000;
    static const unsigned J83ASymbolRateLower = 4000000;
    static const unsigned J83ASymbolRateDefault = 6952000;
    static const unsigned J83ASymbolRateQAM64 = 5056941;
    static const unsigned J83ASymbolRateQAM256 = 5360537;

    // BASE:
    // https://github.com/Metrological/bcm-refsw/blob/17.4-uma/nexus/examples/multiprocess/fcc_client.c
    // #define NEXUS_SATELITE 1

    static void DumpData(const uint8_t data[], const uint16_t size)
    {
        printf("Filter data [%d]:\n", size);
        for (uint16_t teller = 0; teller < size; teller++) {
            printf("%02X ", data[teller]);
            if (((teller + 1) & 0x7) == 0) {
                printf("\n");
            }
        }
        printf("\n\n");
    }

    class __attribute__((visibility("hidden"))) Tuner : public ITuner {
    private:
        Tuner() = delete;
        Tuner(const Tuner&) = delete;
        Tuner& operator=(const Tuner&) = delete;

        enum psi_state {
            NONE,
            PAT,
            PMT
        };

    public:
        class NexusInformation {
        private:
            NexusInformation(const NexusInformation&) = delete;
            NexusInformation& operator=(const NexusInformation&) = delete;

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
                    , Annex(ITuner::Annex::A)
                    , Scan(false)
                    , Callsign("Streamer")
                {
                    Add(_T("frontends"), &Frontends);
                    Add(_T("decoders"), &Decoders);
                    Add(_T("standard"), &Standard);
                    Add(_T("annex"), &Annex);
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
                Core::JSON::Boolean Scan;
                Core::JSON::String Callsign;
            };
            NexusInformation()
                : _rc(-1)
            {
            }

        public:
            static NexusInformation& Instance()
            {
                return (_instance);
            }
            ~NexusInformation()
            {
            }
            void Initialize(const string& configuration)
            {

                Config config;
                config.FromString(configuration);

                _frontends = config.Frontends.Value();
                _standard = config.Standard.Value();
                _annex = config.Annex.Value();
                _eScan = config.Scan.Value();

                NxClient_JoinSettings joinSettings;
                NxClient_GetDefaultJoinSettings(&joinSettings);
                strcpy(joinSettings.name, config.Callsign.Value().c_str());
                _rc = NxClient_Join(&joinSettings);
                BDBG_ASSERT(!_rc);

                NxClient_AllocSettings allocSettings;
                NxClient_GetDefaultAllocSettings(&allocSettings);
                allocSettings.simpleVideoDecoder = config.Decoders.Value();
                allocSettings.simpleAudioDecoder = 1;
                _rc = NxClient_Alloc(&allocSettings, &_allocResults);
                BDBG_ASSERT(!_rc);
            }
            void Deinitialize()
            {
                NxClient_Uninit();
            }

        public:
            inline bool Joined() const { return (_rc == 0); }
            inline ITuner::DTVStandard Standard() const { return (_standard); }
            inline ITuner::Annex Annex() const { return (_annex); }
            inline bool Scan() const { return (_eScan); }
            inline int AudioDecoder() const { return (_allocResults.simpleAudioDecoder.id); }
            inline int VideoDecoder(const uint8_t index) const { return (_allocResults.simpleVideoDecoder[index].id); }

        private:
            uint8_t _frontends;
            ITuner::DTVStandard _standard;
            ITuner::Annex _annex;
            bool _eScan;
            NEXUS_Error _rc;
            NxClient_AllocResults _allocResults;

            static NexusInformation _instance;
        };

    private:
        class Audio {
        private:
            Audio() = delete;
            Audio(const Audio&) = delete;
            Audio& operator=(const Audio&) = delete;

        public:
            Audio(Tuner& parent)
                : _parent(parent)
                , _audio(0)
                , _pidChannel(nullptr)
                , _decoder(nullptr)
            {
            }
            ~Audio()
            {
                Detach();
                Close();
            }

        public:
            uint32_t Open(const uint16_t pid, const uint16_t stream)
            {
                _audio = (pid << 16) | stream;
                return (Core::ERROR_NONE);
            }
            uint32_t Close()
            {
                _audio = 0;
                return (Core::ERROR_NONE);
            }

            uint32_t Attach()
            {
                uint32_t result = Core::ERROR_NONE;

                if (_audio != 0) {

                    result = Core::ERROR_UNAVAILABLE;

                    ASSERT(_decoder == nullptr);

                    _pidChannel = NEXUS_PidChannel_Open(_parent.ParserBand(), ((_audio >> 16) & 0xFFFF), nullptr);

                    if (_pidChannel != nullptr) {

                        _decoder = NEXUS_SimpleAudioDecoder_Acquire(NexusInformation::Instance().AudioDecoder());

                        if (_decoder != nullptr) {

                            NEXUS_SimpleAudioDecoderStartSettings audioProgram;
                            NEXUS_SimpleAudioDecoder_GetDefaultStartSettings(&audioProgram);

                            audioProgram.primary.pidChannel = _pidChannel;

                            switch (_audio & 0xFFFF) {
                            case TS_PSI_ST_11172_3_Audio:
                            case TS_PSI_ST_13818_3_Audio:
                                audioProgram.primary.codec = NEXUS_AudioCodec_eMpeg;
                                break;
                            case TS_PSI_ST_13818_7_AAC:
                                audioProgram.primary.codec = NEXUS_AudioCodec_eAac;
                                break;
                            default:
                                TRACE_L1("Unknown stream type %d pid=%d (Using default AC3)", (_audio & 0xFFFF), (_audio >> 16));
                                audioProgram.primary.codec = NEXUS_AudioCodec_eAc3;
                            }

                            NEXUS_SimpleStcChannelSettings& settings(_parent.Settings());

                            if (settings.modeSettings.pcr.pidChannel == nullptr) {
                                // If the PCR pid is not set, this is an Audio only stream, do the settings on the stcChannel !!
                                int rc = NEXUS_SimpleStcChannel_SetSettings(_parent.Channel(), &settings);
                                BDBG_ASSERT(!rc);
                            }

                            NEXUS_SimpleAudioDecoder_SetStcChannel(_decoder, _parent.Channel());

                            NEXUS_SimpleAudioDecoder_Start(_decoder, &audioProgram);

                            result = Core::ERROR_NONE;
                        }
                    }
                }
                return (result);
            }

            uint32_t Detach()
            {
                if (_decoder != nullptr) {

                    NEXUS_SimpleAudioDecoder_Stop(_decoder);
                    NEXUS_SimpleAudioDecoder_Release(_decoder);

                    if (_pidChannel != nullptr) {
                        NEXUS_PidChannel_Close(_pidChannel);
                        _pidChannel = nullptr;
                    }

                    _decoder = nullptr;
                }

                return (Core::ERROR_NONE);
            }

        private:
            Tuner& _parent;
            uint32_t _audio;
            NEXUS_PidChannelHandle _pidChannel;
            NEXUS_SimpleAudioDecoderHandle _decoder;
        };

        class Primer {
        private:
            Primer() = delete;
            Primer(const Primer&) = delete;
            Primer& operator=(const Primer&) = delete;

        public:
            Primer(Tuner& parent)
                : _parent(parent)
                , _pidChannel(nullptr)
                , _decoder(nullptr)
            {
            }
            ~Primer()
            {
                Detach();
                Close();
            }

        public:
            uint32_t Open(const uint16_t pid, const uint16_t stream, const uint8_t index)
            {
                uint32_t result = Core::ERROR_UNAVAILABLE;
                _pidChannel = NEXUS_PidChannel_Open(_parent.ParserBand(), pid, nullptr);

                if (_pidChannel != nullptr) {
                    NEXUS_SimpleStcChannelSettings& settings(_parent.Settings());

                    if (settings.modeSettings.pcr.pidChannel == nullptr) {
                        settings.modeSettings.pcr.pidChannel = _pidChannel;
                    }

                    int rc = NEXUS_SimpleStcChannel_SetSettings(_parent.Channel(), &settings);
                    BDBG_ASSERT(!rc);

                    _decoder = NEXUS_SimpleVideoDecoder_Acquire(NexusInformation::Instance().VideoDecoder(index));

                    if (_decoder == nullptr) {
                        TRACE_L1("Could not get a Simple Video Decoder index=%u", index);
                    } else {

                        NEXUS_VideoDecoderSettings settings;
                        NEXUS_SimpleVideoDecoderStartSettings program;

                        NEXUS_SimpleVideoDecoder_GetSettings(_decoder, &settings);
                        settings.firstPtsPassed.callback = FirstPTSPassed;
                        settings.firstPtsPassed.context = this;
                        rc = NEXUS_SimpleVideoDecoder_SetSettings(_decoder, &settings);
                        BDBG_ASSERT(!rc);

                        NEXUS_SimpleVideoDecoder_GetDefaultStartSettings(&program);
                        program.settings.pidChannel = _pidChannel;

                        switch (stream) {
                        case TS_PSI_ST_11172_2_Video:
                        case TS_PSI_ST_13818_2_Video:
                            _codec = NEXUS_VideoCodec_eMpeg2;
                            break;
                        case TS_PSI_ST_14496_10_Video:
                            _codec = NEXUS_VideoCodec_eH264;
                            break;
                        case TS_PSI_ST_23008_2_Video:
                            _codec = NEXUS_VideoCodec_eH265;
                        default:
                            TRACE_L1("Unknown stream type %d pid=%d  (Using default Mpeg2)", stream, pid);
                            _codec = NEXUS_VideoCodec_eMpeg2;
                        }

                        program.settings.codec = _codec;

                        NEXUS_SimpleVideoDecoder_SetStcChannel(_decoder, _parent.Channel());

                        rc = NEXUS_SimpleVideoDecoder_StartPrimer(_decoder, &program);
                        result = (rc == 0 ? Core::ERROR_NONE : Core::ERROR_OPENING_FAILED);
                    }
                }

                return (result);
            }
            uint32_t Close()
            {
                if (_decoder != nullptr) {
                    NEXUS_SimpleVideoDecoder_Release(_decoder);
                    _decoder = nullptr;
                }
                if (_pidChannel != nullptr) {
                    NEXUS_PidChannel_Close(_pidChannel);
                    _pidChannel = nullptr;
                }
                return (Core::ERROR_NONE);
            }

            uint32_t Attach(const uint8_t index)
            {
                uint32_t result = Core::ERROR_NONE;

                if (_pidChannel != nullptr) {

                    int rc = NEXUS_SimpleVideoDecoder_StopPrimerAndStartDecode(_decoder);
                    BDBG_ASSERT(!rc);

                    result = (rc != 0 ? Core::ERROR_INPROGRESS : Core::ERROR_NONE);
                }

                return (result);
            }
            uint32_t Detach()
            {
                if (_decoder != nullptr) {

                    NEXUS_SimpleVideoDecoder_StopAndFree(_decoder);

                    NEXUS_SimpleVideoDecoderStartSettings program;

                    NEXUS_SimpleVideoDecoder_GetDefaultStartSettings(&program);
                    program.settings.pidChannel = _pidChannel;
                    program.settings.codec = _codec;

                    int rc = NEXUS_SimpleVideoDecoder_StartPrimer(_decoder, &program);
                    BDBG_ASSERT(!rc);
                }

                return (Core::ERROR_NONE);
            }

        private:
            void FirstPTSPassed(int param)
            {
                TRACE_L1("First PTS Passed (%d)", param);
            }
            static void FirstPTSPassed(void* context, int param)
            {
                reinterpret_cast<Primer*>(context)->FirstPTSPassed(param);
            }

        private:
            Tuner& _parent;
            NEXUS_PidChannelHandle _pidChannel;
            NEXUS_SimpleVideoDecoderHandle _decoder;
            NEXUS_VideoCodec _codec;
        };

        class Section {
        private:
            Section(const Section&) = delete;
            Section& operator=(const Section&) = delete;

        public:
            Section()
                : _adminLock()
                , _pidChannel(nullptr)
                , _messages(nullptr)
                , _callback(nullptr)
            {
                NEXUS_MessageSettings openSettings;

                NEXUS_Message_GetDefaultSettings(&openSettings);
                openSettings.dataReady.callback = MessageCallback;
                openSettings.dataReady.context = this;
                openSettings.dataReady.param = 0;

                _messages = NEXUS_Message_Open(&openSettings);
            }
            ~Section()
            {
                Close();

                if (_messages != nullptr) {

                    NEXUS_Message_Close(_messages);
                    _messages = nullptr;
                }
            }

        public:
            uint32_t Open(NEXUS_ParserBand& parser, const uint16_t pid, const uint8_t tableId, ISection* callback)
            {
                uint32_t result = Core::ERROR_INPROGRESS;

                if (_messages != nullptr) {

                    ASSERT((callback != nullptr) && (_callback == nullptr));

                    _pidChannel = NEXUS_PidChannel_Open(parser, pid, nullptr);

                    if (_pidChannel != nullptr) {
                        NEXUS_MessageStartSettings startSettings;
                        NEXUS_Message_GetDefaultStartSettings(_messages, &startSettings);
                        startSettings.pidChannel = _pidChannel;
                        if (tableId != 0) {
                            startSettings.filter.coefficient[0] = tableId;
                            startSettings.filter.mask[0] = 0x0;
                            // startSettings.filter.exclusion[0] = 0x0;
                        }
                        _callback = callback;
                        NEXUS_Error rc = NEXUS_Message_Start(_messages, &startSettings);
                        if (rc != 0) {
                            NEXUS_PidChannel_Close(_pidChannel);
                            _pidChannel = nullptr;
                            _callback = nullptr;
                        } else {
                            result = Core::ERROR_NONE;
                        }
                    }
                }

                return (result);
            }
            void Close()
            {
                if (_messages != nullptr) {
                    NEXUS_Message_Stop(_messages);
                }
                if (_pidChannel != nullptr) {
                    NEXUS_PidChannel_Close(_pidChannel);
                    _pidChannel = nullptr;
                }
                _adminLock.Lock();
                _callback = nullptr;
                _adminLock.Unlock();
            }

        private:
            static void MessageCallback(void* context, int32_t param)
            {
                reinterpret_cast<Section*>(context)->MessageCallback(param);
            }
            void MessageCallback(int32_t param)
            {
                size_t sectionSize = 0;
                const void* section = nullptr;

                int rc = NEXUS_Message_GetBuffer(_messages, &section, &sectionSize);

                if ((rc == 0) && (sectionSize != 0)) {
                    uint8_t* data = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(section));
                    size_t handled = 0;

                    while (handled < sectionSize) {

                        MPEG::Section newSection(Core::DataElement(sectionSize - handled, &(data[handled])));

                        // Looks like the sections are rounded to 32 bits boundaries.
                        size_t sectionLength = ((newSection.Length() + 3) & (static_cast<size_t>(~0) ^ 0x03));

                        if (newSection.IsValid() == true) {
                            _adminLock.Lock();
                            if (_callback != nullptr) {
                                _callback->Handle(newSection);
                            }
                            _adminLock.Unlock();
                        } else {
                            TRACE_L1("Invalid frame (%d - %d)!!!!\n", (sectionSize - handled), sectionLength);
                            DumpData(&(data[handled]), sectionSize - handled);
                        }

                        handled += sectionLength;
                    }

                    NEXUS_Message_ReadComplete(_messages, sectionSize);
                }
            }

        private:
            Core::CriticalSection _adminLock;
            NEXUS_PidChannelHandle _pidChannel;
            NEXUS_MessageHandle _messages;
            ISection* _callback;
        };

        class Collector : public IMonitor, public ISection {
        private:
            Collector() = delete;
            Collector(const Collector&) = delete;
            Collector& operator=(const Collector&) = delete;

        public:
            Collector(Tuner& parent)
                : _parent(parent)
                , _handler()
                , _callback(nullptr)
            {
            }
            ~Collector() { Close(); }

        public:
            uint32_t Open(const uint16_t frequency)
            {
                uint32_t result = Core::ERROR_UNAVAILABLE;

                if (_callback == nullptr) {
                    // Lets start collecting information (PAT/PMT)
                    _callback = ProgramTable::Instance().Register(this, frequency);
                }

                if (_callback == nullptr) {
                    TRACE_L1("Could not create a Program filter!! %d", __LINE__);
                } else {
                    _handler.Close();
                    _handler.Open(_parent.ParserBand(), 0, 0, this);
                    result = Core::ERROR_NONE;
                }
                return (result);
            }
            uint32_t Close()
            {
                // Just in case we where listening to something else :-)
                _handler.Close();

                if (_callback != nullptr) {
                    ProgramTable::Instance().Unregister(this);
                    _callback = nullptr;
                }

                return (Core::ERROR_NONE);
            }
            bool Program(const uint16_t frequency, const uint16_t programId, MPEG::PMT& pmt) const
            {
                return (ProgramTable::Instance().Program(frequency, programId, pmt));
            }

        private:
            virtual void ChangePid(const uint16_t newpid, ISection* observer) override
            {
                ASSERT(observer == _callback);

                _handler.Close();

                if (newpid != static_cast<uint16_t>(~0)) {
                    _handler.Open(_parent.ParserBand(), newpid, MPEG::PMT::ID, this);
                } else {
                    ProgramTable::Instance().Unregister(this);
                    _callback = nullptr;
                }
            }

            virtual void Handle(const MPEG::Section& section) override
            {
                if (_callback != nullptr) {
                    _callback->Handle(section);
                }
                _parent.EvaluateProgramId();
            }

        private:
            Tuner& _parent;
            Section _handler;
            ISection* _callback;
        };

        typedef std::map<uint32_t, Section> Sections;

        Tuner(uint8_t index)
            : _index(index)
            , _state(IDLE)
            , _lockDuration(0)
            , _parserBand(0)
            , _pidChannel(nullptr)
            , _videoDecoder(*this)
            , _audioDecoder(*this)
            , _collector(*this)
            , _programId(~0)
            , _sections()
            , _callback(nullptr)
        {

            NEXUS_FrontendAcquireSettings frontendAcquireSettings;
            NEXUS_Frontend_GetDefaultAcquireSettings(&frontendAcquireSettings);
            frontendAcquireSettings.capabilities.qam = true;
            _frontend = NEXUS_Frontend_Acquire(&frontendAcquireSettings);

            if ((_frontend == nullptr) || ((_stcChannel = NEXUS_SimpleStcChannel_Create(nullptr)) == nullptr)) {
                TRACE_L1("Unable to find QAM-capable frontend: %d", __LINE__);

                if (_frontend != nullptr) {
                    NEXUS_Frontend_Release(_frontend);
                    _frontend = nullptr;
                }
            } else {
                _parserBand = NEXUS_ParserBand_Open(NEXUS_ANY_ID);

                NEXUS_Frontend_GetDefaultQamSettings(&_settings);

                if (NexusInformation::Instance().Standard() == DVB) {
                    _settings.annex = (NexusInformation::Instance().Annex() == A ? NEXUS_FrontendQamAnnex_eA : NEXUS_FrontendQamAnnex_eB);
                    _settings.bandwidth = NEXUS_FrontendQamBandwidth_e8Mhz;
                    _settings.scan.upperBaudSearch = J83ASymbolRateUpper;
                    _settings.scan.lowerBaudSearch = J83ASymbolRateLower;
                } else {
                    _settings.annex = (NexusInformation::Instance().Annex() == B ? NEXUS_FrontendQamAnnex_eB : NEXUS_FrontendQamAnnex_eA);
                    _settings.bandwidth = NEXUS_FrontendQamBandwidth_e6Mhz;
                    _settings.scan.upperBaudSearch = 5360537;
                    _settings.scan.lowerBaudSearch = 5056941;
                }
                _settings.lockCallback.callback = LockCallback;
                _settings.lockCallback.context = this;
                _settings.lockCallback.param = 0;

                _settings.asyncStatusReadyCallback.callback = StatusCallback;
                _settings.asyncStatusReadyCallback.context = this;
                _settings.autoAcquire = true;
                _settings.enablePowerMeasurement = true;

                // eScan - Not supported on Pace 7002.
                if (NexusInformation::Instance().Scan() == true) {
                    _settings.acquisitionMode = NEXUS_FrontendQamAcquisitionMode_eScan;
                }

                _settings.enableNullPackets = false;

                NEXUS_SimpleStcChannel_GetSettings(_stcChannel, &_stcSettings);
                _stcSettings.mode = NEXUS_StcChannelMode_ePcr;
                _stcSettings.modeSettings.pcr.offsetThreshold = 0xFF;
                _stcSettings.modeSettings.pcr.pidChannel = nullptr;

                _callback = TunerAdministrator::Instance().Announce(this);
            }
        }

    public:
        ~Tuner()
        {
            TunerAdministrator::Instance().Revoke(this);

            Detach(0);

            if (_frontend != nullptr) {
                NEXUS_Frontend_Release(_frontend);
                _frontend = nullptr;
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
        inline bool IsValid() const { return (_frontend != nullptr); }

        // Using these methods the state of the tuner can be viewed.
        // IDLE:      Means there is no request,, or the frequency requested (with other parameters) can not be locked.
        // LOCKED:    The stream has been locked, frequency, modulation, symbolrate and spectral inversion seem to be fine.
        // PREPARED:  The program that was requetsed to prepare fore, has been found in PAT/PMT, the needed information, like PIDS is loaded.
        //            If Priming is available, this means that the priming has started!
        // STREAMING: This means that the requested program is being streamed to decoder or file, depending on implementation/inpuy.
        virtual state State() const override
        {
            return (_state);
        }

        // Currently locked on ID
        // This method return a unique number that will identify the locked on Transport stream. The ID will always
        // identify the uniquely locked on to Tune request. ID => 0 is reserved and means not locked on to anything.
        virtual uint16_t Id() const override
        {
            return (_state == IDLE ? 0 : _settings.frequency / 1000000);
        }

        // Using the next method, the allocated Frontend will try to lock the channel that is found at the given parameters.
        // Frequency is always in MHz.
        virtual uint32_t Tune(const uint16_t frequency, const Modulation modulation, const uint32_t symbolRate, const uint16_t fec, const SpectralInversion inversion) override
        {
            uint32_t result = Core::ERROR_UNAVAILABLE;

            if (_frontend != nullptr) {
                if (_state != IDLE) {
                    _state = IDLE;
                    _callback->StateChange(this);
                }
                _psiLoaded = NONE;

                NEXUS_ParserBandSettings parserBandSettings;
                NEXUS_FrontendUserParameters userParams;

                NEXUS_Frontend_GetUserParameters(_frontend, &userParams);
                NEXUS_ParserBand_GetSettings(_parserBand, &parserBandSettings);

                if (userParams.isMtsif) {
                    parserBandSettings.sourceType = NEXUS_ParserBandSourceType_eMtsif;
                    parserBandSettings.sourceTypeSettings.mtsif = NEXUS_Frontend_GetConnector(_frontend);
                } else {
                    parserBandSettings.sourceType = NEXUS_ParserBandSourceType_eInputBand;
                    parserBandSettings.sourceTypeSettings.inputBand = userParams.param1;
                }
                parserBandSettings.transportType = NEXUS_TransportType_eTs;

                if (NEXUS_ParserBand_SetSettings(_parserBand, &parserBandSettings) == 0) {

                    uint32_t value = frequency;
                    _settings.symbolRate = symbolRate;
                    _settings.frequency = value * 1000000;
                    switch (modulation) {
                    case QAM16:
                        _settings.mode = NEXUS_FrontendQamMode_e16;
                        break;
                    case QAM32:
                        _settings.mode = NEXUS_FrontendQamMode_e32;
                        break;
                    case QAM64:
                        _settings.mode = NEXUS_FrontendQamMode_e64;
                        break;
                    case QAM128:
                        _settings.mode = NEXUS_FrontendQamMode_e128;
                        break;
                    case QAM256:
                        _settings.mode = NEXUS_FrontendQamMode_e256;
                        break;
                    case QAM512:
                        _settings.mode = NEXUS_FrontendQamMode_e512;
                        break;
                    case QAM1024:
                        _settings.mode = NEXUS_FrontendQamMode_e1024;
                        break;
                    case QAM2048:
                        _settings.mode = NEXUS_FrontendQamMode_e2048;
                        break;
                    case QAM4096:
                        _settings.mode = NEXUS_FrontendQamMode_e4096;
                        break;
                    default:
                        _settings.mode = NEXUS_FrontendQamMode_eAuto_64_256;
                        break;
                    }
                    _settings.scan
                        .mode[NEXUS_FrontendQamAnnex_eB][NEXUS_FrontendQamMode_e64]
                        = true;
                    _settings.scan
                        .mode[NEXUS_FrontendQamAnnex_eB][NEXUS_FrontendQamMode_e256]
                        = true;
                    _settings.scan
                        .mode[NEXUS_FrontendQamAnnex_eA][NEXUS_FrontendQamMode_e64]
                        = true;
                    _settings.scan
                        .mode[NEXUS_FrontendQamAnnex_eA][NEXUS_FrontendQamMode_e256]
                        = true;
                    _settings.spectrumMode = (inversion == Auto ? NEXUS_FrontendQamSpectrumMode_eAuto
                                                                : NEXUS_FrontendQamSpectrumMode_eManual);

                    if (inversion != Auto) {
                        _settings.spectralInversion = (inversion == Normal
                                ? NEXUS_FrontendQamSpectralInversion_eNormal
                                : NEXUS_FrontendQamSpectralInversion_eInverted);
                    }

                    if (_settings.symbolRate == 0) {
                        _settings.symbolRate = (_settings.bandwidth == NEXUS_FrontendQamBandwidth_e8Mhz
                                ?
                                /* DVB */ J83ASymbolRateDefault
                                :
                                /* ATSC */ J83ASymbolRateQAM256);
                    }

                    TRACE_L1("Tuning to %u MHz mode=%d sym=%d Annex=%s spectrumMode=%s Inversion=%s",
                        frequency, _settings.mode, _settings.symbolRate,
                        _settings.annex == NEXUS_FrontendQamAnnex_eA ? "A" : "B",
                        _settings.spectrumMode == NEXUS_FrontendQamSpectrumMode_eAuto
                            ? "Auto"
                            : "Manual",
                        _settings.spectralInversion == NEXUS_FrontendQamSpectralInversion_eNormal
                            ? "Normal"
                            : "Inverted");

                    NEXUS_Error rc = NEXUS_Frontend_TuneQam(_frontend, &_settings);

                    if (rc != 0) {
                        TRACE_L1("NEXUS_Frontend_TuneQam Failed rc=%d", rc);
                        result = Core::ERROR_GENERAL;
                    } else {
                        _lockDuration = Core::Time::Now().Ticks();

                        // Start parsing the PAT/PMT...

                        result = Core::ERROR_NONE;
                    }
                }
            }

            return (result);
        }

        // In case the tuner needs to be tuned to s apecific programId, please list it here. Once the PID's associated to this programId have been
        // found, and set, the Tuner will reach its PREPARED state.
        virtual uint32_t Prepare(const uint16_t programId) override
        {
            _state.Lock();

            if (_state != STREAMING) {
                _programId = programId;
            }

            _state.Unlock();

            EvaluateProgramId();

            return (_programId == programId ? Core::ERROR_NONE : Core::ERROR_UNAVAILABLE);
        }

        // A Tuner can be used to filter PSI/SI. Using the next call a callback can be installed to receive sections associated with a table.
        // Each valid section received will be offered as a single section on the ISection interface for the user to process.
        virtual uint32_t Filter(const uint16_t pid, const uint8_t tableId, ISection* callback) override
        {
            uint32_t result = Core::ERROR_UNAVAILABLE;
            uint32_t id = (pid << 16) | tableId;

            if (callback != nullptr) {
                auto entry = _sections.emplace(std::piecewise_construct, std::forward_as_tuple(id),
                    std::forward_as_tuple());
                result = entry.first->second.Open(ParserBand(), pid, tableId, callback);

                if (result != Core::ERROR_NONE) {
                    _sections.erase(entry.first);
                }
            } else {
                Sections::iterator index(_sections.find(id));
                if (index != _sections.end()) {
                    index->second.Close();
                    _sections.erase(index);
                }
                result = Core::ERROR_NONE;
            }
            return (result);
        }

        // Using the next two methods, the frontends will be hooked up to decoders or file, and be removed from a decoder or file.
        virtual uint32_t Attach(const uint8_t index) override
        {
            NxClient_ConnectSettings connectSettings;
            NxClient_GetDefaultConnectSettings(&connectSettings);

            if (_stcSettings.modeSettings.pcr.pidChannel != nullptr)
                connectSettings.simpleVideoDecoder[0].id = NexusInformation::Instance().VideoDecoder(_index);

            // TODO: Should be conditional, only if we have Audio
            connectSettings.simpleAudioDecoder.id = NexusInformation::Instance().AudioDecoder();

            int rc = NxClient_Connect(&connectSettings, &_connectId);
            BDBG_ASSERT(!rc);

            _videoDecoder.Attach(index);
            _audioDecoder.Attach();

            return (Core::ERROR_NONE);
        }

        virtual uint32_t Detach(const uint8_t index) override
        {
            _videoDecoder.Detach();
            _audioDecoder.Detach();

            NxClient_Disconnect(_connectId);

            return (Core::ERROR_NONE);
        }

    private:
        inline NEXUS_ParserBand& ParserBand() { return (_parserBand); }
        inline NEXUS_SimpleStcChannelHandle Channel() { return (_stcChannel); }
        inline NEXUS_SimpleStcChannelSettings& Settings() { return (_stcSettings); }
        void LockCallback(int32_t param)
        {

            NEXUS_FrontendFastStatus status;
            NEXUS_Frontend_GetFastStatus(_frontend, &status);

            if (status.lockStatus == NEXUS_FrontendLockStatus_eLocked) {
                NEXUS_Error rc = NEXUS_Frontend_RequestQamAsyncStatus(_frontend);

                _collector.Open(_settings.frequency / 1000000);

                if (rc != 0) {
                    TRACE_L1("QAM locked, but could not request Status, Error: %d !!", rc);
                }
            } else if (status.lockStatus == NEXUS_FrontendLockStatus_eNoSignal) {
                TRACE_L1("No signal on the tuned frequency!! %d", __LINE__);
            }
        }

        void StatusCallback(int32_t param)
        {
            NEXUS_Frontend_GetQamAsyncStatus(_frontend, &_status);

            if (_status.fecLock > 0) {

                _state = LOCKED;
                _callback->StateChange(this);

                // Stop the time. We are tuned and have all info...
                _lockDuration = Core::Time::Now().Ticks() - _lockDuration;

                TRACE_L1("QAM locked, Status read. Locking took %d ms !!",
                    static_cast<uint32_t>(_lockDuration / Core::Time::TicksPerMillisecond));

                EvaluateProgramId();
            }
        }

        void EvaluateProgramId()
        {
            _state.Lock();

            if ((_state == LOCKED) || (_state == PREPARED)) {
                uint16_t identifier = _settings.frequency / 1000000;

                bool updated((_psiLoaded == NONE) && (ProgramTable::Instance().NITPid(identifier) != static_cast<uint16_t>(~0)));

                if (updated == true) {
                    _psiLoaded = PAT;
                }

                if (_collector.Program(identifier, _programId, _program) == true) {

                    // Just in case it was tuned to something..
                    Close();

                    if (_program.IsValid() == true) {

                        // Go start priming
                        Open();

                        if (_state == LOCKED) {
                            _state = PREPARED;
                            _psiLoaded = PMT;
                            updated = true;
                        }
                    } else {
                        _state = LOCKED;
                        updated = true;
                    }
                } else if (_state == PREPARED) {
                    if (_program.IsValid() == false) {
                        Close();
                        _state = LOCKED;
                        updated = true;
                    }
                }

                if (updated == true) {
                    _callback->StateChange(this);
                }
            }

            _state.Unlock();
        }

        void Open()
        {
            uint32_t video = 0, audio = 0;

            MPEG::PMT::StreamIterator streams(_program.Streams());

            while (streams.Next() == true) {

                switch (streams.StreamType()) {
                case TS_PSI_ST_13818_2_Video:
                case TS_PSI_ST_14496_10_Video: // 2: Mpeg 2, 27: H.264.
                case TS_PSI_ST_23008_2_Video:
                    if (video == 0) {
                        if (_program.PCRPid() != streams.Pid()) {
                            // Seems like the PCRPid is not the video pid. Create a  PCR channel...
                            _pidChannel = NEXUS_PidChannel_Open(ParserBand(), _program.PCRPid(), nullptr);
                            _stcSettings.modeSettings.pcr.pidChannel = _pidChannel;
                        }

                        _videoDecoder.Open(streams.Pid(), streams.StreamType(), _index);
                        video = (streams.Pid() << 16) | (streams.StreamType() & 0xFFFF);
                    }
                    break;
                case TS_PSI_ST_11172_3_Audio:
                case TS_PSI_ST_13818_3_Audio:
                case TS_PSI_ST_13818_7_AAC:
                case TS_PSI_ST_BD_AC3:
                    if (audio == 0) {
                        _audioDecoder.Open(streams.Pid(), streams.StreamType());
                        audio = (streams.Pid() << 16) | (streams.StreamType() & 0xFFFF);
                    }
                    break;
                default:
                    break;
                }
            }
        }

        void Close()
        {
            _videoDecoder.Close();
            _audioDecoder.Close();

            if (_pidChannel != nullptr) {
                NEXUS_PidChannel_Close(_pidChannel);
                _pidChannel = nullptr;
            }
            _stcSettings.modeSettings.pcr.pidChannel = nullptr;
        }

        // Link the Nexus C world to the C++ world.
        static void LockCallback(void* context, int32_t param)
        {
            reinterpret_cast<Tuner*>(context)->LockCallback(param);
        }
        static void StatusCallback(void* context, int32_t param)
        {
            reinterpret_cast<Tuner*>(context)->StatusCallback(param);
        }

    private:
        uint8_t _index;
        Core::StateTrigger<state> _state;
        uint64_t _lockDuration;
        NEXUS_FrontendHandle _frontend;
        NEXUS_ParserBand _parserBand;
        NEXUS_SimpleStcChannelHandle _stcChannel;
        NEXUS_SimpleStcChannelSettings _stcSettings;
        NEXUS_PidChannelHandle _pidChannel;
        Primer _videoDecoder;
        Audio _audioDecoder;
        Collector _collector;
        uint32_t _programId;
        uint32_t _connectId;
        Sections _sections;
        MPEG::PMT _program;

#ifdef NEXUS_SATELITE
        NEXUS_FrontendSatelliteSettings _settings;
        NEXUS_FrontendSatelliteStatus _status;
        NEXUS_FrontendDiseqcSettings _diseq;
#else
        NEXUS_FrontendQamSettings _settings;
        NEXUS_FrontendQamStatus _status;
#endif

        NEXUS_FrontendDeviceStatus _deviceStatus;

        psi_state _psiLoaded;
        TunerAdministrator::ICallback* _callback;
    };

    /* static */ Tuner::NexusInformation Tuner::NexusInformation::_instance;

    // The following methods will be called before any create is called. It allows for an initialization,
    // if requires, and a deinitialization, if the Tuners will no longer be used.
    /* static */ uint32_t ITuner::Initialize(const string& configuration)
    {
        Tuner::NexusInformation::Instance().Initialize(configuration);
        return (Core::ERROR_NONE);
    }

    /* static */ uint32_t ITuner::Deinitialize()
    {
        Tuner::NexusInformation::Instance().Deinitialize();
        return (Core::ERROR_NONE);
    }

    // Accessor to create a tuner.
    /* static */ ITuner* ITuner::Create(const string& info)
    {
        return (Tuner::Create(info));
    }

} // namespace Broadcast
} // namespace WPEFramework
