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

#include "../Module.h"

#include "SBC.h"

#include <sbc/sbc.h>

namespace Thunder {

ENUM_CONVERSION_BEGIN(Bluetooth::A2DP::SBC::preset)
    { Bluetooth::A2DP::SBC::COMPATIBLE, _TXT("Compatible") },
    { Bluetooth::A2DP::SBC::LQ, _TXT("LQ") },
    { Bluetooth::A2DP::SBC::MQ, _TXT("MQ") },
    { Bluetooth::A2DP::SBC::HQ, _TXT("HQ") },
    { Bluetooth::A2DP::SBC::XQ, _TXT("XQ") },
ENUM_CONVERSION_END(Bluetooth::A2DP::SBC::preset)

ENUM_CONVERSION_BEGIN(Bluetooth::A2DP::SBC::Config::channelmode)
    { Bluetooth::A2DP::SBC::Config::MONO, _TXT("Mono") },
    { Bluetooth::A2DP::SBC::Config::STEREO, _TXT("Stereo") },
    { Bluetooth::A2DP::SBC::Config::JOINT_STEREO, _TXT("JointSstereo") },
    { Bluetooth::A2DP::SBC::Config::DUAL_CHANNEL, _TXT("DualChannel") },
ENUM_CONVERSION_END(Bluetooth::A2DP::SBC::Config::channelmode)

namespace Bluetooth {

namespace A2DP {

    /* virtual */ uint32_t SBC::Configure(const StreamFormat& format, const string& settings)
    {
        uint32_t result = Core::ERROR_NONE;

        Core::JSON::String Data;
        Core::JSON::Container container;
        container.Add("LC-SBC", &Data);
        container.FromString(settings);

        Config config;
        config.FromString(Data.Value());

        _lock.Lock();

        preset preferredPreset = HQ;
        Format::samplingfrequency frequency = Format::SF_INVALID;
        Format::channelmode channelMode = Format::CM_INVALID;
        uint8_t maxBitpool = _supported.MinBitpool();

        if (config.Preset.IsSet() == true) {
            // Preset requested in config, this will now be the target quality.
            if (config.Preset.Value() != COMPATIBLE) {
                preferredPreset = config.Preset.Value();
            }
            else {
                preferredPreset = LQ;
            }
        }

        switch (format.SampleRate) {
        case 16000:
            frequency = Format::SF_16000_HZ;
            break;
        case 32000:
            frequency = Format::SF_32000_HZ;
            break;
        case 44100:
            frequency = Format::SF_44100_HZ;
            break;
        case 48000:
            frequency = Format::SF_48000_HZ;
            break;
        default:
            break;
        }

        frequency = static_cast<Format::samplingfrequency>(frequency & _supported.SamplingFrequency());

        switch (format.Channels) {
        case 1:
            channelMode = Format::CM_MONO;
            break;
        case 2:
            if (config.ChannelMode.IsSet() == true) {

                switch (config.ChannelMode) {
                case Config::STEREO:
                    channelMode = Format::CM_STEREO;
                    break;
                case Config::DUAL_CHANNEL:
                    channelMode = Format::CM_DUAL_CHANNEL;
                    break;
                default:
                    break;
                }
            }

            if (channelMode == Format::CM_INVALID) {
                // Joint-Stereo compresses better than regular stereo when the signal is concentrated
                // in the middle of the stereo image, what is quite typical.
                channelMode = Format::CM_JOINT_STEREO;
            }

            break;
        }

        channelMode = static_cast<Format::channelmode>(channelMode & _supported.ChannelMode());

        if ((channelMode != Format::CM_INVALID) && (frequency != Format::SF_INVALID) && (format.Resolution == 16)) {
            bool stereo = (channelMode != Format::CM_MONO);

            // Select bitpool and channel mode based on preferred format...
            // (Note that bitpools for sample rates of 16 and 32 kHz are not specified, hence will receive same values as 44,1 kHz.)
            if (preferredPreset == XQ) {
                if (_supported.MaxBitpool() >= 38) {
                    maxBitpool = 38;

                    if (stereo == true) {
                        if (_supported.MaxBitpool() >= 76) {
                            maxBitpool = 76;
                        }
                        else {
                            // Max supported bitpool is too low for XQ joint-stereo, so try 38 on dual channel instead
                            // - that should give similar quality/bitrate.
                            channelMode = Format::CM_DUAL_CHANNEL;
                        }
                    }
                }
                else {
                    // XQ not supported, drop to the next best one...
                    preferredPreset = HQ;
                }
            }

            if (preferredPreset == HQ) {
                if (frequency == Format::SF_48000_HZ) {
                    maxBitpool = (stereo? 51 : 29);
                }
                else {
                    maxBitpool = (stereo? 53 : 31);
                }

                if (maxBitpool > _supported.MaxBitpool()) {
                    preferredPreset = MQ;
                }
            }

            if (preferredPreset == MQ) {
                if (frequency == Format::SF_48000_HZ) {
                    maxBitpool = (stereo? 33 : 18);
                }
                else {
                    maxBitpool = (stereo? 35 : 19);
                }

                if (maxBitpool > _supported.MaxBitpool()) {
                    preferredPreset = LQ;
                }
            }

            if (preferredPreset == LQ) {
                maxBitpool = (stereo? 29 : 15);

                if (maxBitpool > _supported.MaxBitpool()) {
                    preferredPreset = COMPATIBLE;
                }
            }

            if (preferredPreset == COMPATIBLE) {
                // Use whatever is the maximum supported bitpool.
                maxBitpool = _supported.MaxBitpool();
            }

            ASSERT(maxBitpool <= MAX_BITPOOL);

            _actuals.SamplingFrequency(frequency);
            _actuals.ChannelMode(channelMode);
            _actuals.MinBitpool(_supported.MinBitpool());
            _actuals.MaxBitpool(maxBitpool);
            _preferredBitpool = maxBitpool;
            _preset = preferredPreset;

#ifdef __DEBUG__
            DumpConfiguration();
#endif

            Bitpool(maxBitpool);
        }
        else {
            result = Core::ERROR_NOT_SUPPORTED;
            TRACE(Trace::Error, (_T("Unsuppored SBC paramters requested")));
        }

        _lock.Unlock();

        return (result);
    }

    /* virtual */ uint32_t SBC::Configure(const uint8_t stream[], const uint16_t length)
    {
        uint32_t result = Core::ERROR_NONE;

        _lock.Lock();

        _actuals.Deserialize(stream, length);

        _preset = COMPATIBLE;

        _preferredBitpool = _actuals.MaxBitpool();

        Bitpool(_actuals.MaxBitpool());

        _lock.Unlock();

        return (result);
    }

    /* virtual */ void SBC::Configuration(StreamFormat& format, string& settings) const
    {
        Config config;

        _lock.Lock();

        format.FrameRate = 0;

        format.Resolution = 16; // Always 16-bit samples

        switch (_actuals.SamplingFrequency()) {
        case Format::SF_48000_HZ:
            format.SampleRate = 48000;
            break;
        case Format::SF_44100_HZ:
            format.SampleRate = 44100;
            break;
        case Format::SF_32000_HZ:
            format.SampleRate = 32000;
            break;
        case Format::SF_16000_HZ:
            format.SampleRate = 16000;
            break;
        default:
            ASSERT(false && "Invalid sampling frequency configured");
            break;
        }

        switch (_actuals.ChannelMode()) {
        case Format::CM_MONO:
            config.ChannelMode = Config::MONO;
            format.Channels = 1;
            break;
        case Format::CM_DUAL_CHANNEL:
            config.ChannelMode = Config::DUAL_CHANNEL;
            format.Channels = 2;
            break;
        case Format::CM_STEREO:
            config.ChannelMode = Config::STEREO;
            format.Channels = 2;
            break;
        case Format::CM_JOINT_STEREO:
            config.ChannelMode = Config::JOINT_STEREO;
            format.Channels = 2;
            break;
        default:
            ASSERT(false && "Invalid channel mode configured");
            break;
        }

        config.Preset = _preset;
        config.Bitpool = _bitpool;

        _lock.Unlock();

        config.ToString(settings);
    }

    /* virtual */ uint16_t SBC::Serialize(const bool capabilities, uint8_t stream[], const uint16_t length) const
    {
        _lock.Lock();

        const uint16_t result = (capabilities? _supported.Serialize(stream, length) : _actuals.Serialize(stream, length));

        _lock.Unlock();

        return (result);
    }

    /* virtual */ uint16_t SBC::Encode(const uint16_t inBufferSize, const uint8_t inBuffer[],
                                       uint16_t& outSize, uint8_t outBuffer[]) const
    {
        ASSERT(_rawFrameSize != 0);
        ASSERT(_encodedFrameSize != 0);

        ASSERT(inBuffer != nullptr);
        ASSERT(outBuffer != nullptr);

        uint16_t consumed = 0;
        uint16_t produced = sizeof(SBCHeader);
        uint16_t count = 0;

        _lock.Lock();

        if ((_rawFrameSize != 0) && (_encodedFrameSize != 0)) {

            const uint8_t MAX_FRAMES = 15; // only a four bit number holds the number of frames in a packet

            uint16_t frames = (inBufferSize / _rawFrameSize);
            uint16_t available = (outSize - produced);

            ASSERT(outSize >= sizeof(SBCHeader));

            if (frames > MAX_FRAMES) {
                frames = MAX_FRAMES;
            }

            while ((frames-- > 0)
                    && (inBufferSize >= (consumed + _rawFrameSize))
                    && (available >= _encodedFrameSize)) {

                ssize_t written = 0;
                ssize_t read = ::sbc_encode(static_cast<::sbc_t*>(_sbcHandle),
                                            (inBuffer + consumed), _rawFrameSize,
                                            (outBuffer + produced), available,
                                            &written);

                if (read < 0) {
                    TRACE_L1("Failed to encode an SBC frame!");
                    break;
                }
                else {
                    consumed += read;
                    available -= written;
                    produced += written;
                    count++;
                }
            }
        }

        _lock.Unlock();

        if (count > 0) {
            SBCHeader* header = reinterpret_cast<SBCHeader*>(outBuffer);
            header->frameCount = (count & 0xF);
            outSize = produced;
        }
        else {
            outSize = 0;
        }

        return (consumed);
    }

    /* virtual */ uint16_t SBC::Decode(const uint16_t inBufferSize, const uint8_t inBuffer[],
                                       uint16_t& outSize, uint8_t outBuffer[]) const
    {
        ASSERT(_rawFrameSize != 0);
        ASSERT(_encodedFrameSize != 0);

        ASSERT(inBuffer != nullptr);
        ASSERT(outBuffer != nullptr);

        uint16_t consumed = 0;
        uint16_t produced = 0;
        uint16_t available = outSize;

        _lock.Lock();

        if ((_rawFrameSize != 0) && (_encodedFrameSize != 0)) {

            ASSERT(outSize >= sizeof(SBCHeader));
            const SBCHeader* header = reinterpret_cast<const SBCHeader*>(inBuffer);

            uint8_t frames =  header->frameCount;
            consumed = sizeof(SBCHeader);

            while ((frames != 0)
                    && (inBufferSize >= (consumed + _encodedFrameSize))
                    && (available >= _rawFrameSize)) {

                size_t written = 0;
                ssize_t read = ::sbc_decode(static_cast<::sbc_t*>(_sbcHandle),
                                            (inBuffer + consumed), _encodedFrameSize,
                                            (outBuffer + produced), available,
                                            &written);


                if (read < 0) {
                    TRACE_L1("Failed to decode an SBC frame!");
                    break;
                }
                else {
                    available -= written;
                    produced += written;
                    consumed += read;
                }

                frames--;
            }

            ASSERT(frames == 0);
        }

        _lock.Unlock();

        outSize = produced;

        return (consumed);
    }

    /* virtual */ uint32_t SBC::QOS(const int8_t policy)
    {
        uint32_t result = Core::ERROR_NONE;

        ASSERT(_preferredBitpool != 0);

        const uint8_t STEP = (_preferredBitpool / 10);

        _lock.Lock();

        uint8_t newBitpool = _bitpool;

        if (policy == 0) {
            // reset quality
            newBitpool = _preferredBitpool;
        }
        else if (policy < 0) {
            // decrease quality
            if (newBitpool == _supported.MinBitpool()) {
                result = Core::ERROR_UNAVAILABLE;
            }
            else if ((newBitpool - STEP) < _supported.MinBitpool()) {
                newBitpool = _supported.MinBitpool();
            }
            else {
                newBitpool -= STEP;
            }
        }
        else {
            // increase quality
            if (_bitpool == _preferredBitpool) {
                newBitpool = Core::ERROR_UNAVAILABLE;
            }
            else if ((_bitpool + STEP) >= _preferredBitpool) {
                newBitpool = _preferredBitpool;
            }
            else {
                newBitpool += STEP;
            }
        }

        if (result == Core::ERROR_NONE) {
            Bitpool(newBitpool);
        }

        _lock.Unlock();

        return (result);
    }

    void SBC::Bitpool(uint8_t value)
    {
        ASSERT(value <= MAX_BITPOOL);
        ASSERT(value >= MIN_BITPOOL);

        _bitpool = value;

        TRACE(Trace::Information, (_T("New bitpool value for SBC: %d"), _bitpool));

        SBCConfigure();
        #ifdef __DEBUG__
            DumpBitrateConfiguration();
        #endif
    }

    void SBC::SBCInitialize()
    {
        _lock.Lock();

        ASSERT(_sbcHandle == nullptr);

        _sbcHandle = static_cast<void*>(new ::sbc_t);
        ASSERT(_sbcHandle != nullptr);

        ::sbc_init(static_cast<::sbc_t*>(_sbcHandle), 0L);

        _lock.Unlock();
    }

    void SBC::SBCDeinitialize()
    {
        _lock.Lock();

        if (_sbcHandle != nullptr) {
            ::sbc_finish(static_cast<::sbc_t*>(_sbcHandle));

            delete static_cast<::sbc_t*>(_sbcHandle);

            _sbcHandle = nullptr;
        }

        _lock.Unlock();
    }

    void SBC::SBCConfigure()
    {
        _lock.Lock();

        ASSERT(_sbcHandle != nullptr);
        ::sbc_reinit(static_cast<::sbc_t*>(_sbcHandle), 0L);

        uint32_t rate;
        uint32_t blocks;
        uint32_t bands;

        ::sbc_t* sbc = static_cast<::sbc_t*>(_sbcHandle);

        sbc->bitpool = _bitpool;
        sbc->allocation = (_actuals.AllocationMethod() == Format::AM_LOUDNESS? SBC_AM_LOUDNESS : SBC_AM_SNR);

        switch (_actuals.SubBands()) {
        default:
        case Format::SB_8:
            sbc->subbands = SBC_SB_8;
            bands = 8;
            break;
        case Format::SB_4:
            sbc->subbands = SBC_SB_4;
            bands = 4;
            break;
        }

        switch (_actuals.SamplingFrequency()) {
        case Format::SF_48000_HZ:
            sbc->frequency = SBC_FREQ_48000;
            rate = 48000;
            break;
        default:
        case Format::SF_44100_HZ:
            sbc->frequency = SBC_FREQ_44100;
            rate = 44100;
            break;
        case Format::SF_32000_HZ:
            sbc->frequency = SBC_FREQ_32000;
            rate = 32000;
            break;
        case Format::SF_16000_HZ:
            sbc->frequency = SBC_FREQ_16000;
            rate = 16000;
            break;
        }

        switch (_actuals.BlockLength()) {
        default:
        case Format::BL_16:
            sbc->blocks = SBC_BLK_16;
            blocks = 16;
            break;
        case Format::BL_12:
            sbc->blocks = SBC_BLK_12;
            blocks = 12;
            break;
        case Format::BL_8:
            sbc->blocks = SBC_BLK_8;
            blocks = 8;
            break;
        case Format::BL_4:
            sbc->blocks = SBC_BLK_4;
            blocks = 4;
            break;
        }

        switch (_actuals.ChannelMode()) {
        default:
        case Format::CM_JOINT_STEREO:
            sbc->mode = SBC_MODE_JOINT_STEREO;
            break;
        case Format::CM_STEREO:
            sbc->mode = SBC_MODE_STEREO;
            break;
        case Format::CM_DUAL_CHANNEL:
            sbc->mode = SBC_MODE_DUAL_CHANNEL;
            break;
        case Format::CM_MONO:
            sbc->mode = SBC_MODE_MONO;
            break;
        }

        _frameDuration = ::sbc_get_frame_duration(sbc); /* microseconds */
        _rawFrameSize = ::sbc_get_codesize(sbc); /* bytes */
        _encodedFrameSize = ::sbc_get_frame_length(sbc); /* bytes */

        _bitRate = ((8L * _encodedFrameSize * rate) / (bands * blocks)); /* bits per second */
        _channels = (sbc->mode == SBC_MODE_MONO? 1 : 2);
        _sampleRate = rate;

        _lock.Unlock();
    }

#ifdef __DEBUG__
    void SBC::DumpConfiguration() const
    {
        #define ELEM(name, val, prop) (_T("  [  %d] " name " [  %d]"), !!(_supported.val() & Format::prop), !!(_actuals.val() & Format::prop))
        TRACE(Trace::Information, (_T("SBC configuration:")));
        TRACE(Trace::Information, ELEM("Sampling frequency - 16 kHz     ", SamplingFrequency, SF_16000_HZ));
        TRACE(Trace::Information, ELEM("Sampling frequency - 32 kHz     ", SamplingFrequency, SF_32000_HZ));
        TRACE(Trace::Information, ELEM("Sampling frequency - 44.1 kHz   ", SamplingFrequency, SF_44100_HZ));
        TRACE(Trace::Information, ELEM("Sampling frequency - 48 kHz     ", SamplingFrequency, SF_48000_HZ));
        TRACE(Trace::Information, ELEM("Channel mode - Mono             ", ChannelMode, CM_MONO));
        TRACE(Trace::Information, ELEM("Channel mode - Stereo           ", ChannelMode, CM_STEREO));
        TRACE(Trace::Information, ELEM("Channel mode - Dual Channel     ", ChannelMode, CM_DUAL_CHANNEL));
        TRACE(Trace::Information, ELEM("Channel mode - Joint Stereo     ", ChannelMode, CM_JOINT_STEREO));
        TRACE(Trace::Information, ELEM("Block length - 4                ", BlockLength, BL_4));
        TRACE(Trace::Information, ELEM("Block length - 8                ", BlockLength, BL_8));
        TRACE(Trace::Information, ELEM("Block length - 12               ", BlockLength, BL_12));
        TRACE(Trace::Information, ELEM("Block length - 16               ", BlockLength, BL_16));
        TRACE(Trace::Information, ELEM("Frequency sub-bands - 4         ", SubBands, SB_4));
        TRACE(Trace::Information, ELEM("Frequency sub-bands - 8         ", SubBands, SB_8));
        TRACE(Trace::Information, ELEM("Bit allocation method - SNR     ", AllocationMethod, AM_SNR));
        TRACE(Trace::Information, ELEM("Bit allocation method - Loudness", AllocationMethod, AM_LOUDNESS));
        TRACE(Trace::Information, (_T("  [%3d] Minimal bitpool value            [%3d]"), _supported.MinBitpool(), _actuals.MinBitpool()));
        TRACE(Trace::Information, (_T("  [%3d] Maximal bitpool value            [%3d]"), _supported.MaxBitpool(), _actuals.MaxBitpool()));
        #undef ELEM
    }

    void SBC::DumpBitrateConfiguration() const
    {
        Core::EnumerateType<preset> preset(_preset);
        TRACE(Trace::Information, (_T("Quality preset: %s"), (preset.IsSet() == true? preset.Data() : "(custom)")));
        TRACE(Trace::Information, (_T("Bitpool value: %d"), _bitpool));
        TRACE(Trace::Information, (_T("Bitrate: %d bps"), _bitRate));
        TRACE(Trace::Information, (_T("Frame size: raw %d bytes, encoded %d bytes (%d us)"), _rawFrameSize, _encodedFrameSize, _frameDuration));
    }
#endif // __DEBUG__

} // namespace A2DP

} // namespace Bluetooth

}
