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

#ifndef NETWORKS_ADMINISTRATOR_H
#define NETWORKS_ADMINISTRATOR_H

#include "Definitions.h"
#include "Descriptors.h"
#include "NIT.h"
#include "ProgramTable.h"

namespace Thunder {

namespace Broadcast {

    class Networks {
    private:
        Networks(const Networks&) = delete;
        Networks& operator=(const Networks&) = delete;

        class Sink : public ITuner::INotification {
        private:
            Sink() = delete;
            Sink(const Sink&) = delete;
            Sink& operator=(const Sink&) = delete;

        public:
            Sink(Networks& parent)
                : _parent(parent)
            {
            }
            virtual ~Sink()
            {
            }

        public:
            virtual void Activated(ITuner* /* tuner */) override
            {
            }
            virtual void Deactivated(ITuner* tuner) override
            {
                _parent.Deactivated(tuner);
            }
            virtual void StateChange(ITuner* tuner) override
            {
                _parent.StateChange(tuner);
            }

        private:
            Networks& _parent;
        };

        class Parser : public ISection {
        private:
            Parser() = delete;
            Parser(const Parser&) = delete;
            Parser& operator=(const Parser&) = delete;

        public:
            Parser(Networks& parent, ITuner* source, const bool scan, const uint16_t pid)
                : _parent(parent)
                , _source(source)
                , _actual(Core::ProxyType<Core::DataStore>::Create(512))
                , _others(Core::ProxyType<Core::DataStore>::Create(512))
                , _pid(pid)
            {
                if (scan == true) {
                    Scan(true);
                }
            }
            virtual ~Parser()
            {
                Scan(false);
            }
            inline bool operator==(const ITuner* rhs) const
            {
                return (_source == rhs);
            }
            inline bool operator!=(const ITuner* rhs) const
            {
                return (!operator==(rhs));
            }

        public:
            void Scan(const bool scan)
            {
                if (scan == true) {
                    // Start loading the SDT info
                    _source->Filter(_pid, DVB::NIT::ACTUAL, this);
                    _source->Filter(_pid, DVB::NIT::OTHER, this);
                } else {
                    _source->Filter(_pid, DVB::NIT::OTHER, nullptr);
                    _source->Filter(_pid, DVB::NIT::ACTUAL, nullptr);
                }
            }

        private:
            virtual void Handle(const MPEG::Section& section) override
            {

                ASSERT(section.IsValid());

                if (section.TableId() == DVB::NIT::ACTUAL) {
                    _actual.AddSection(section);
                    if (_actual.IsValid() == true) {
                        _parent.Load(DVB::NIT(_actual));
                    }
                } else if (section.TableId() == DVB::NIT::OTHER) {
                    _others.AddSection(section);
                    if (_others.IsValid() == true) {
                        _parent.Load(DVB::NIT(_others));
                    }
                }
            }

        private:
            Networks& _parent;
            ITuner* _source;
            MPEG::Table _actual;
            MPEG::Table _others;
            uint16_t _pid;
        };

        typedef std::list<Parser> Scanners;

    public:
        class Network {
        public:
            Network()
                : _originalNetworkId(~0)
                , _transportStreamId(~0)
                , _frequency(0)
                , _modulation(0)
                , _symbolRate(0)
                , _name()
            {
            }
            Network(const DVB::NIT::NetworkIterator& info)
                : _originalNetworkId(info.OriginalNetworkId())
                , _transportStreamId(info.TransportStreamId())
                , _frequency(0)
                , _modulation(0)
                , _symbolRate(0)
                , _name()
            {
                MPEG::DescriptorIterator index(info.Descriptors());

                if (index.Tag(DVB::Descriptors::NetworkName::TAG) == true) {
                    DVB::Descriptors::NetworkName data(index.Current());
                    _name = data.Name();
                }

                // Start searching from the beginning again !
                index.Reset();

                if (index.Tag(DVB::Descriptors::SatelliteDeliverySystem::TAG) == true) {
                    DVB::Descriptors::SatelliteDeliverySystem data(index.Current());
                    _frequency = data.Frequency();
                    _symbolRate = data.SymbolRate();
                    _modulation = data.Modulation();
                } else {
                    // Start searching from the beginning again !
                    index.Reset();

                    if (index.Tag(DVB::Descriptors::CableDeliverySystem::TAG) == true) {
                        DVB::Descriptors::CableDeliverySystem data(index.Current());
                        _frequency = data.Frequency();
                        _symbolRate = data.SymbolRate();
                        _modulation = data.Modulation();
                    }
                }
            }
            Network(const Network& copy)
                : _originalNetworkId(copy._originalNetworkId)
                , _transportStreamId(copy._transportStreamId)
                , _frequency(copy._frequency)
                , _modulation(copy._modulation)
                , _symbolRate(copy._symbolRate)
                , _name(copy._name)
            {
            }
            ~Network()
            {
            }

            Network& operator=(const Network& rhs)
            {
                _originalNetworkId = rhs._originalNetworkId;
                _transportStreamId = rhs._transportStreamId;
                _frequency = rhs._frequency;
                _modulation = rhs._modulation;
                _symbolRate = rhs._symbolRate;
                _name = rhs._name;

                return (*this);
            }

        public:
            bool IsValid() const
            {
                return (_frequency != 0);
            }
            inline uint16_t OriginalNetworkId() const
            {
                return (_originalNetworkId);
            }
            inline uint16_t TransportStreamId() const
            {
                return (_transportStreamId);
            }
            inline uint16_t Frequency() const
            {
                return (_frequency);
            }
            inline uint16_t Modulation() const
            {
                return (_modulation);
            }
            inline uint16_t SymbolRate() const
            {
                return (_symbolRate);
            }

        private:
            uint16_t _originalNetworkId;
            uint16_t _transportStreamId;
            uint16_t _frequency;
            uint16_t _modulation;
            uint16_t _symbolRate;
            string _name;
        };

        typedef IteratorType<Network> Iterator;

    private:
        typedef std::map<uint16_t, Network> NetworkMap;

    public:
        Networks()
            : _adminLock()
            , _scanners()
            , _sink(*this)
            , _scan(true)
            , _networks()
        {
            ITuner::Register(&_sink);
        }
        virtual ~Networks()
        {
            ITuner::Unregister(&_sink);
        }

    public:
        void Scan(const bool scan)
        {
            _adminLock.Lock();

            if (_scan != scan) {
                _scan = scan;
                Scanners::iterator index(_scanners.begin());
                while (index != _scanners.end()) {
                    index->Scan(_scan);
                    index++;
                }
            }
            _adminLock.Unlock();
        }
        Network Id(const uint16_t id) const
        {
            Network result;

            _adminLock.Lock();
            NetworkMap::const_iterator index(_networks.find(id));
            if (index != _networks.end()) {
                result = index->second;
            }
            _adminLock.Unlock();

            return (result);
        }
        Iterator List() const
        {

            _adminLock.Lock();
            Iterator value(_networks);
            _adminLock.Unlock();

            return (value);
        }

    private:
        void Deactivated(ITuner* tuner)
        {
            _adminLock.Lock();

            Scanners::iterator index = std::find(_scanners.begin(), _scanners.end(), tuner);

            if (index != _scanners.end()) {
                _scanners.erase(index);
            }

            _adminLock.Unlock();
        }
        void StateChange(ITuner* tuner)
        {

            _adminLock.Lock();

            Scanners::iterator index = std::find(_scanners.begin(), _scanners.end(), tuner);

            if (index != _scanners.end()) {
                if (tuner->State() == ITuner::IDLE) {
                    _scanners.erase(index);
                }
            } else {
                if (tuner->State() != ITuner::IDLE) {
                    uint16_t pid = ProgramTable::Instance().NITPid(tuner->Id());
                    if (pid != static_cast<uint16_t>(~0)) {
                        _scanners.emplace_back(*this, tuner, _scan, pid);
                    }
                }
            }

            _adminLock.Unlock();
        }
        void Load(const DVB::NIT& table)
        {
            _adminLock.Lock();

            DVB::NIT::NetworkIterator index = table.Networks();

            while (index.Next() == true) {
                _networks[index.TransportStreamId()] = Network(index);
            }

            _adminLock.Unlock();
        }

    private:
        mutable Core::CriticalSection _adminLock;
        Scanners _scanners;
        Sink _sink;
        bool _scan;
        NetworkMap _networks;
    };

} // namespace Broadcast
} // namespace Thunder

#endif // NETWORKS_ADMINISTRATOR_H
