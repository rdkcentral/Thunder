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

#ifndef SERVICE_ADMINISTRATOR_H
#define SERVICE_ADMINISTRATOR_H

#include "Definitions.h"
#include "Descriptors.h"
#include "SDT.h"

namespace Thunder {

namespace Broadcast {

    class Services {
    private:
        Services(const Services&) = delete;
        Services& operator=(const Services&) = delete;

        class Sink : public ITuner::INotification {
        private:
            Sink() = delete;
            Sink(const Sink&) = delete;
            Sink& operator=(const Sink&) = delete;

        public:
            Sink(Services& parent)
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
            Services& _parent;
        };

        class Parser : public ISection {
        private:
            Parser() = delete;
            Parser(const Parser&) = delete;
            Parser& operator=(const Parser&) = delete;

        public:
            Parser(Services& parent, ITuner* source, const bool scan)
                : _parent(parent)
                , _source(source)
                , _actual(Core::ProxyType<Core::DataStore>::Create(512))
                , _others(Core::ProxyType<Core::DataStore>::Create(512))
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
                    _source->Filter(0x11, DVB::SDT::ACTUAL, this);
                    _source->Filter(0x11, DVB::SDT::OTHER, this);
                } else {
                    _source->Filter(0x11, DVB::SDT::OTHER, nullptr);
                    _source->Filter(0x11, DVB::SDT::ACTUAL, nullptr);
                }
            }

        private:
            virtual void Handle(const MPEG::Section& section) override
            {

                ASSERT(section.IsValid());

                if (section.TableId() == DVB::SDT::ACTUAL) {
                    _actual.AddSection(section);
                    if (_actual.IsValid() == true) {
                        _parent.Load(DVB::SDT(_actual));
                    }
                } else if (section.TableId() == DVB::SDT::OTHER) {
                    _others.AddSection(section);
                    if (_others.IsValid() == true) {
                        _parent.Load(DVB::SDT(_others));
                    }
                }
            }

        private:
            Services& _parent;
            ITuner* _source;
            MPEG::Table _actual;
            MPEG::Table _others;
        };

        typedef std::list<Parser> Scanners;

    public:
        class Service {
        public:
            Service()
                : _serviceId(~0)
                , _info(~0)
                , _name()
            {
            }
            Service(const DVB::SDT::ServiceIterator& info)
                : _serviceId(info.ServiceId())
                , _info((info.EIT_PF() ? 0x10 : 0x00) | (info.EIT_Schedule() ? 0x20 : 0x00) | (info.IsFreeToAir() ? 0x40 : 0x00) | info.RunningMode())
                , _name(_T("Unknown"))
            {
                MPEG::DescriptorIterator index(info.Descriptors());

                if (index.Tag(DVB::Descriptors::Service::TAG) == true) {
                    DVB::Descriptors::Service nameDescriptor(index.Current());
                    _name = nameDescriptor.Name();
                }
            }
            Service(const Service& copy)
                : _serviceId(copy._serviceId)
                , _info(copy._info)
                , _name(copy._name)
            {
            }
            ~Service()
            {
            }

            Service& operator=(const Service& rhs)
            {
                _serviceId = rhs._serviceId;
                _info = rhs._info;
                _name = rhs._name;

                return (*this);
            }

        public:
            bool IsValid() const
            {
                return (_info != static_cast<uint8_t>(~0));
            }
            const string& Name() const
            {
                return (_name);
            }
            inline uint16_t ServiceId() const
            {
                return (_serviceId);
            }
            inline bool EIT_PF() const
            {
                return ((_info & 0x10) != 0);
            }
            inline bool EIT_Schedule() const
            {
                return ((_info & 0x20) != 0);
            }
            inline bool IsFreeToAir() const
            {
                return ((_info & 0x40) != 0);
            }
            inline DVB::SDT::running RunningMode() const
            {
                return (static_cast<DVB::SDT::running>(_info & 0x07));
            }

        private:
            uint16_t _serviceId;
            uint8_t _info;
            string _name;
        };

        typedef IteratorType<Service> Iterator;

    private:
        typedef std::map<uint16_t, Service> ServiceMap;

    public:
        Services()
            : _adminLock()
            , _scanners()
            , _sink(*this)
            , _scan(true)
            , _services()
        {
            ITuner::Register(&_sink);
        }
        virtual ~Services()
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
        Service Id(const uint16_t id) const
        {
            Service result;

            _adminLock.Lock();
            ServiceMap::const_iterator index(_services.find(id));
            if (index != _services.end()) {
                result = index->second;
            }
            _adminLock.Unlock();

            return (result);
        }
        Iterator List() const
        {

            _adminLock.Lock();
            Iterator value(_services);
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
                    _scanners.emplace_back(*this, tuner, _scan);
                }
            }

            _adminLock.Unlock();
        }
        void Load(const DVB::SDT& table)
        {
            _adminLock.Lock();

            DVB::SDT::ServiceIterator index = table.Services();

            while (index.Next() == true) {
                _services[index.ServiceId()] = Service(index);
            }

            _adminLock.Unlock();
        }

    private:
        mutable Core::CriticalSection _adminLock;
        Scanners _scanners;
        Sink _sink;
        bool _scan;
        ServiceMap _services;
    };

} // namespace Broadcast
} // namespace Thunder

#endif // SERVICE_ADMINISTRATOR_H
