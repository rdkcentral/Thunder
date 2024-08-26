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

#ifndef __BROADCAST_DEFINITIONS_H
#define __BROADCAST_DEFINITIONS_H

#include "Module.h"

namespace Thunder {

namespace Broadcast {

    namespace MPEG {

        class Section;
    }

    struct ISection {
        virtual ~ISection() {}
        virtual void Handle(const MPEG::Section& section) = 0;
    };

    struct IMonitor {
        virtual ~IMonitor() {}
        virtual void ChangePid(const uint16_t newpid, ISection* observer) = 0;
    };

    template <typename LISTOBJECT>
    class IteratorType {
    public:
        IteratorType()
            : _position(0)
            , _index()
            , _list()
        {
        }
        IteratorType(const std::map<uint16_t, LISTOBJECT>& container)
            : _position(0)
            , _index()
            , _list()
        {
            typename std::map<uint16_t, LISTOBJECT>::const_iterator index(container.begin());
            while (index != container.end()) {
                _list.emplace_back(index->second);
                index++;
            }
        }
        IteratorType(const IteratorType<LISTOBJECT>& copy)
            : _list()
        {
            operator=(copy);
        }
        ~IteratorType()
        {
        }

        IteratorType<LISTOBJECT>& operator=(const IteratorType<LISTOBJECT>& rhs)
        {
            _position = rhs._position;
            _list.clear();
            typename std::list<LISTOBJECT>::const_iterator index(rhs._list.begin());

            while (index != rhs._list.end()) {
                _list.emplace_back(*index);
                if (_list.size() == rhs._position) {
                    _index = --_list.end();
                }
                index++;
            }
            return (*this);
        }

    public:
        bool IsValid() const
        {
            return ((_position != 0) && (_position <= _list.size()));
        }
        void Reset()
        {
            _position = 0;
        }
        bool Next()
        {
            if (_position == 0) {
                _index = _list.begin();
                _position++;
            } else if (_position <= _list.size()) {
                _index++;
                _position++;
            }

            return (_position <= _list.size());
        }
        const LISTOBJECT& Current() const
        {

            ASSERT(IsValid() == true);

            return (*_index);
        }

    private:
        uint32_t _position;
        typename std::list<LISTOBJECT>::const_iterator _index;
        typename std::list<LISTOBJECT> _list;
    };

    template <typename TYPE>
    TYPE ConvertBCD(const uint8_t buffer[], const uint8_t digits, const bool evenStart)
    {
        TYPE value = 0;
        for (uint8_t index = 0; index < digits;) {
            value *= 10;
            if (evenStart == true) {
                value += ((index & 0x01) ? (buffer[index / 2] & 0xF) : (buffer[index / 2] >> 4));
                index++;
            } else {
                index++;
                value += ((index & 0x01) ? (buffer[index / 2] & 0xF) : (buffer[index / 2] >> 4));
            }
        }
        return (value);
    }

    enum SpectralInversion {
        Auto,
        Normal,
        Inverted
    };

    enum Modulation {
        MODULATION_UNKNOWN = 0,

        // Satellite modulation types
        HORIZONTAL_QPSK = 1,
        HORIZONTAL_8PSK = 2,
        HORIZONTAL_QAM16 = 3,
        VERTICAL_QPSK = 5,
        VERTICAL_8PSK = 6,
        VERTICAL_QAM16 = 7,
        LEFT_QPSK = 9,
        LEFT_8PSK = 10,
        LEFT_QAM16 = 11,
        RIGHT_QPSK = 13,
        RIGHT_8PSK = 14,
        RIGHT_QAM16 = 15,

        // Cable/Terestrial modulation types
        QAM16 = 16,
        QAM32 = 32,
        QAM64 = 64,
        QAM128 = 128,
        QAM256 = 256,
        QAM512 = 512,
        QAM1024 = 1024,
        QAM2048 = 2048,
        QAM4096 = 4096
    };

    enum fec {
        FEC_INNER_UNKNOWN = 0,
        FEC_1_2 = 1,
        FEC_2_3 = 2,
        FEC_3_4 = 3,
        FEC_5_6 = 4,
        FEC_7_8 = 5,
        FEC_8_9 = 6,
        FEC_3_5 = 7,
        FEC_4_5 = 8,
        FEC_9_10 = 9,
        FEC_2_5 = 10,
        FEC_6_7 = 11,
        FEC_INNER_NONE = 15
    };

    enum fec_outer {
        FEC_OUTER_UNKNOWN = 0,
        FEC_OUTER_NONE = 1,
        RS = 2
    };

    enum transmission {
        TRANSMISSION_AUTO,
        TRANSMISSION_1K,
        TRANSMISSION_2K,
        TRANSMISSION_4K,
        TRANSMISSION_8K,
        TRANSMISSION_16K,
        TRANSMISSION_32K,
        TRANSMISSION_C3780,
        TRANSMISSION_C1
    };

    enum guard {
        GUARD_AUTO,
        GUARD_1_4,
        GUARD_1_8,
        GUARD_1_16,
        GUARD_1_32,
        GUARD_1_128,
        GUARD_19_128,
        GUARD_19_256,
    };

    enum hierarchy {
        NoHierarchy,
        AutoHierarchy,
        Hierarchy1,
        Hierarchy2,
        Hierarchy4,
    };

    struct EXTERNAL ITuner {

        struct INotification {
            virtual ~INotification() {}

            virtual void Activated(ITuner* tuner) = 0;
            virtual void Deactivated(ITuner* tuner) = 0;
            virtual void StateChange(ITuner* tuner) = 0;
        };

        struct ICallback {
            virtual ~ICallback() {}

            virtual void StateChange() = 0;
        };

        ITuner() : _adminLock(), _callback(nullptr) {}
        virtual ~ITuner() {}

        enum state {
            IDLE = 0x01,
            LOCKED = 0x02,
            PREPARED = 0x04,
            STREAMING = 0x08
        };

        enum DTVStandard {
            DVB = 0x1000,
            ATSC = 0x2000,
            ISDB = 0x3000,
            DAB = 0x4000
        };

        enum modus {
            Satellite = 0x1,
            Terrestrial = 0x2,
            Cable = 0x3
        };

        enum annex {
            NoAnnex = 0x000, // NoAnnex -> S/T
            A = 0x400,       // A       -> S2/T2
            B = 0x800,
            C = 0xC00
        };

        // The following methods will be called before any create is called. It allows for an initialization,
        // if requires, and a deinitialization, if the Tuners will no longer be used.
        static uint32_t Initialize(const string& configuration);
        static uint32_t Deinitialize();

        // See if the tuner supports the requested mode, or is configured for the requested mode. This method
        // only returns proper values if the Initialize has been called before.
        static bool IsSupported(const ITuner::modus mode);

        // Accessor to create a tuner.
        static ITuner* Create(const string& info);

        // Accessor to metadata on the tuners.
        static void Register(INotification* notify);
        static void Unregister(INotification* notify);

        // Offer the ability to get the proper information (with respect to hardware properties) of this tuner instance.
        virtual uint32_t Properties() const = 0;

        inline annex Annex() const {
            return (static_cast<annex>(Properties() & 0x0F00));
        }
        inline modus Modus() const {
            return (static_cast<modus>(Properties() & 0xF));
        }
        inline DTVStandard Standard() const {
            return (static_cast<DTVStandard>(Properties() & 0xF000));
        }

        // Currently locked on ID
        // This method return a unique number that will identify the locked on Transport stream. The ID will always
        // identify the uniquely locked on to Tune request. ID => 0 is reserved and means not locked on to anything.
        virtual uint16_t Id() const = 0;

        // Using these methods the state of the tuner can be viewed.
        // IDLE:      Means there is no request, or the frequency requested (with other parameters) can not be locked.
        // LOCKED:    The stream has been locked, frequency, modulation, symbolrate and spectral inversion seem to be fine.
        // PREPARED:  The program that was requetsed to prepare fore, has been found in PAT/PMT, the needed information,
        //            like PIDS is loaded. If Priming is available, this means that the priming has started!
        // STREAMING: This means that the requested program is being streamed to decoder or file, depending on implementation/inpuy.
        virtual state State() const = 0;

        // Using the next method, the allocated Frontend will try to lock the channel that is found at the given parameters.
        // Frequency is always in MHz.
        virtual uint32_t Tune(const uint16_t frequency, const Modulation, const uint32_t symbolRate, const uint16_t fec, const SpectralInversion) = 0;

        // In case the tuner needs to be tuned to s apecific programId, please list it here. Once the PID's associated to this
        // programId have been found, and set, the Tuner will reach its PREPARED state.
        virtual uint32_t Prepare(const uint16_t programId) = 0;

        // A Tuner can be used to filter PSI/SI. Using the next call a callback can be installed to receive sections associated
        // with a table. Each valid section received will be offered as a single section on the ISection interface for the user
        // to process.
        virtual uint32_t Filter(const uint16_t pid, const uint8_t tableId, ISection* callback) = 0;

        // Using the next two methods, the frontends will be hooked up to decoders or file, and be removed from a decoder or file.
        virtual uint32_t Attach(const uint8_t index) = 0;
        virtual uint32_t Detach(const uint8_t index) = 0;


        // If you have an ITuner interface, you can subscribe to state changes of this Tuner interface
        // This will only be one instance, by design, to avoid the overhead of maintining a list and
        // thus spending more resources. The idea is that the object holding the ITuner interface to
        // do the actual tune can assign a callback and receive the actual state changes. If you "just"
        // receive this interface on the INotification, you should *NOT* set a callback on this interface
        void Callback(ICallback* callback) {

            _adminLock.Lock();

            ASSERT ((_callback == nullptr) ^ (callback == nullptr));

            _callback = callback;

            _adminLock.Unlock();
        }
        void StateChange() {
            _adminLock.Lock();
            if (_callback != nullptr) {
                _callback->StateChange();
            }
            _adminLock.Unlock();
        }
        void Lock() const {
            _adminLock.Lock();
        }
        void Unlock() const {
            _adminLock.Unlock();
        }

    private:
        mutable Core::CriticalSection _adminLock;
        ICallback* _callback;
    };

} // namespace Broadcast
} // namespace Thunder

#endif // __BROADCAST_DEFINITIONS_H
