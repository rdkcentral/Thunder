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

#include "../Module.h"
#include "../IAudioCodec.h"
#include "../DataRecord.h"

namespace Thunder {

namespace Bluetooth {

namespace A2DP {

    class EXTERNAL SBC : public IAudioCodec {
    public:
        static constexpr uint8_t CODEC_TYPE = 0x00; // SBC

        static constexpr uint8_t MIN_BITPOOL = 2;
        static constexpr uint8_t MAX_BITPOOL = 250;

    public:
        enum preset {
            COMPATIBLE,
            LQ,
            MQ,
            HQ,
            XQ
        };

        class Config : public Core::JSON::Container {
        public:
            enum channelmode {
                MONO,
                STEREO,
                JOINT_STEREO,
                DUAL_CHANNEL
            };

        public:
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;
            Config()
                : Core::JSON::Container()
                , Preset(COMPATIBLE)
                , ChannelMode(JOINT_STEREO)
                , Bitpool(MIN_BITPOOL)
            {
                Add(_T("preset"), &Preset);
                Add(_T("channelmode"), &ChannelMode);
                Add(_T("bitpool"), &Bitpool);
            }
            ~Config() = default;

        public:
            Core::JSON::EnumType<preset> Preset;
            Core::JSON::EnumType<channelmode> ChannelMode;
            Core::JSON::DecUInt32 Bitpool;
        }; // class Config

        class Format {
        public:
            enum samplingfrequency : uint8_t {
                SF_INVALID      = 0,
                SF_48000_HZ     = 1, // mandatory for sink
                SF_44100_HZ     = 2, // mandatory for sink
                SF_32000_HZ     = 4,
                SF_16000_HZ     = 8
            };

            enum channelmode : uint8_t  {
                CM_INVALID      = 0,
                CM_JOINT_STEREO = 1, // all mandatory for sink
                CM_STEREO       = 2,
                CM_DUAL_CHANNEL = 4,
                CM_MONO         = 8
            };

            enum blocklength : uint8_t  {
                BL_INVALID      = 0,
                BL_16           = 1,  // all mandatory for sink
                BL_12           = 2,
                BL_8            = 4,
                BL_4            = 8,
            };

            enum subbands : uint8_t  {
                SB_INVALID      = 0,
                SB_8            = 1, // all mandatory for sink
                SB_4            = 2,
            };

            enum allocationmethod : uint8_t {
                AM_INVALID      = 0,
                AM_LOUDNESS     = 1, // all mandatory for sink
                AM_SNR          = 2,
            };

        public:
            Format()
                : _samplingFrequency(SF_44100_HZ)
                , _channelMode(CM_JOINT_STEREO)
                , _blockLength(BL_16)
                , _subBands(SB_8)
                , _allocationMethod(AM_LOUDNESS)
                , _minBitpool(MIN_BITPOOL)
                , _maxBitpool(MIN_BITPOOL) // not an error
            {
            }
            Format(const uint8_t stream[], const uint16_t length)
                : _samplingFrequency(SF_44100_HZ)
                , _channelMode(CM_JOINT_STEREO)
                , _blockLength(BL_16)
                , _subBands(SB_8)
                , _allocationMethod(AM_LOUDNESS)
                , _minBitpool(MIN_BITPOOL)
                , _maxBitpool(MIN_BITPOOL) // not an error
            {
                Deserialize(stream, length);
            }
            Format(const uint16_t maxBitpool, const uint16_t minBitpool)
                : _samplingFrequency(SF_16000_HZ | SF_32000_HZ | SF_44100_HZ | SF_48000_HZ)
                , _channelMode(CM_MONO | CM_DUAL_CHANNEL | CM_STEREO | CM_JOINT_STEREO)
                , _blockLength(BL_4 | BL_8 | BL_12 | BL_16)
                , _subBands(SB_4 | SB_8)
                , _allocationMethod(AM_LOUDNESS | AM_SNR)
                , _minBitpool(minBitpool)
                , _maxBitpool(maxBitpool)
            {
            }
            ~Format() = default;
            Format(const Format&) = default;
            Format& operator=(const Format&) = default;

        public:
            uint16_t Serialize(uint8_t stream[], const uint16_t length) const
            {
                ASSERT(length >= 6);

                uint8_t octet;
                Bluetooth::DataRecord data(stream, length, 0);

                data.Push(IAudioCodec::MEDIA_TYPE);
                data.Push(CODEC_TYPE);

                octet = ((static_cast<uint8_t>(_samplingFrequency) << 4) | static_cast<uint8_t>(_channelMode));
                data.Push(octet);

                octet = ((static_cast<uint8_t>(_blockLength) << 4) | (static_cast<uint8_t>(_subBands) << 2) | static_cast<uint8_t>(_allocationMethod));
                data.Push(octet);

                data.Push(_minBitpool);
                data.Push(_maxBitpool);

                return (data.Length());
            }
            uint16_t Deserialize(const uint8_t stream[], const uint16_t length)
            {
                ASSERT(length >= 6);

                Bluetooth::DataRecord data(stream, length);

                uint8_t octet{};

                data.Pop(octet);
                ASSERT(octet == IAudioCodec::MEDIA_TYPE);

                data.Pop(octet);
                ASSERT(octet == CODEC_TYPE);

                data.Pop(octet);
                _samplingFrequency = (octet >> 4);
                _channelMode = (octet & 0xF);

                data.Pop(octet);
                _blockLength = (octet >> 4);
                _subBands = ((octet >> 2) & 0x3);
                _allocationMethod = (octet & 0x3);

                data.Pop(_minBitpool);
                data.Pop(_maxBitpool);

                return (6);
            }

        public:
            uint8_t SamplingFrequency() const {
                return (_samplingFrequency);
            }
            uint8_t ChannelMode() const {
                return (_channelMode);
            }
            uint8_t BlockLength() const {
                return (_blockLength);
            }
            uint8_t SubBands() const {
                return (_subBands);
            }
            uint8_t AllocationMethod() const {
                return (_allocationMethod);
            }
            uint8_t MinBitpool() const {
                return (_minBitpool);
            }
            uint8_t MaxBitpool() const {
                return (_maxBitpool);
            }

        public:
            void SamplingFrequency(const samplingfrequency sf)
            {
                _samplingFrequency = sf;
            }
            void ChannelMode(const channelmode cm)
            {
                _channelMode = cm;
            }
            void BlockLength(const blocklength bl)
            {
                _blockLength = bl;
            }
            void SubBands(const subbands sb)
            {
                _subBands = sb;
            }
            void AllocationMethod(const allocationmethod am)
            {
                _allocationMethod = am;
            }
            void MinBitpool(const uint8_t value)
            {
                _minBitpool = (value < MIN_BITPOOL? MIN_BITPOOL : value);
            }
            void MaxBitpool(const uint8_t value)
            {
                _maxBitpool = (value > MAX_BITPOOL? MAX_BITPOOL : value);
            }

        private:
            uint8_t _samplingFrequency;
            uint8_t _channelMode;
            uint8_t _blockLength;
            uint8_t _subBands;
            uint8_t _allocationMethod;
            uint8_t _minBitpool;
            uint8_t _maxBitpool;
        }; // class Format

    public:
        SBC(const uint8_t maxBitpool, const uint8_t minBitpool = 2)
            : _lock()
            , _supported(maxBitpool, minBitpool)
            , _actuals()
            , _preset(COMPATIBLE)
            , _sbcHandle(nullptr)
            , _preferredBitpool(0)
            , _bitpool(0)
            , _bitRate(0)
            , _sampleRate(0)
            , _channels(0)
            , _rawFrameSize(0)
            , _encodedFrameSize(0)
            , _frameDuration(0)
        {
            SBCInitialize();
        }
        SBC(const Bluetooth::Buffer& config)
            : _lock()
            , _supported(config.data(), config.length())
            , _actuals()
            , _preset(COMPATIBLE)
            , _sbcHandle(nullptr)
            , _preferredBitpool(0)
            , _bitpool(0)
            , _bitRate(0)
            , _sampleRate(0)
            , _channels(0)
            , _rawFrameSize(0)
            , _encodedFrameSize(0)
            , _frameDuration(0)
        {
            SBCInitialize();
        }
        ~SBC() override
        {
            SBCDeinitialize();
        }

    public:
        IAudioCodec::codectype Type() const override {
            return (IAudioCodec::codectype::LC_SBC);
        }
        uint32_t BitRate() const override {
            return (_bitRate);
        }
        uint16_t RawFrameSize() const override {
            return (_rawFrameSize);
        }
        uint16_t EncodedFrameSize() const override {
            return (_encodedFrameSize);
        }

        uint32_t Configure(const uint8_t stream[], const uint16_t length) override;
        uint32_t Configure(const StreamFormat& format, const string& settings) override;

        void Configuration(StreamFormat& format, string& settings) const override;

        uint32_t QOS(const int8_t policy);

        uint16_t Encode(const uint16_t inBufferSize, const uint8_t inBuffer[],
                        uint16_t& outBufferSize, uint8_t outBuffer[]) const override;

        uint16_t Decode(const uint16_t inBufferSize, const uint8_t inBuffer[],
                        uint16_t& outBufferSize, uint8_t outBuffer[]) const override;

        uint16_t Serialize(const bool capabilities, uint8_t stream[], const uint16_t length) const override;

    private:
        void Bitpool(uint8_t value);

    private:
#ifdef __DEBUG__
        void DumpConfiguration() const;
        void DumpBitrateConfiguration() const;
#endif

    private:
        void SBCInitialize();
        void SBCDeinitialize();
        void SBCConfigure();

    private:
        mutable Core::CriticalSection _lock;
        Format _supported;
        Format _actuals;
        preset _preset;
        void* _sbcHandle;
        uint8_t _preferredBitpool;
        uint8_t _bitpool;
        uint32_t _bitRate;
        uint32_t _sampleRate;
        uint8_t _channels;
        uint16_t _rawFrameSize;
        uint16_t _encodedFrameSize;
        uint32_t _frameDuration;

    private:
        struct SBCHeader {
            uint8_t frameCount;
        } __attribute__((packed));

    }; // class SBC

} // namespace A2DP

} // namespace Bluetooth

}
