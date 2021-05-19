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

#ifndef SCHEDULE_ADMINISTRATOR_H
#define SCHEDULE_ADMINISTRATOR_H

#include "Definitions.h"
#include "Descriptors.h"
#include "EIT.h"

namespace WPEFramework {

namespace Broadcast {

    class Schedules {
    private:
        Schedules(const Schedules&) = delete;
        Schedules& operator=(const Schedules&) = delete;

        class Sink : public ITuner::INotification {
        private:
            Sink() = delete;
            Sink(const Sink&) = delete;
            Sink& operator=(const Sink&) = delete;

        public:
            Sink(Schedules& parent)
                : _parent(parent)
            {
            }
            virtual ~Sink()
            {
            }

        public:
            virtual void Activated(ITuner* tuner) override
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
            Schedules& _parent;
        };

        class Parser : public ISection {
        private:
            Parser() = delete;
            Parser(const Parser&) = delete;
            Parser& operator=(const Parser&) = delete;

        public:
            Parser(Schedules& parent, ITuner* source, const bool scan)
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
                    _source->Filter(0x11, DVB::EIT::ACTUAL, this);
                    _source->Filter(0x11, DVB::EIT::OTHER, this);
                } else {
                    _source->Filter(0x11, DVB::EIT::OTHER, nullptr);
                    _source->Filter(0x11, DVB::EIT::ACTUAL, nullptr);
                }
            }

        private:
            virtual void Handle(const MPEG::Section& section) override
            {

                ASSERT(section.IsValid());

                if (section.TableId() == DVB::EIT::ACTUAL) {
                    _actual.AddSection(section);
                    if (_actual.IsValid() == true) {
                        _parent.Load(DVB::EIT(_actual));
                    }
                } else if (section.TableId() == DVB::EIT::OTHER) {
                    _others.AddSection(section);
                    if (_others.IsValid() == true) {
                        _parent.Load(DVB::EIT(_others));
                    }
                }
            }

        private:
            Schedules& _parent;
            ITuner* _source;
            MPEG::Table _actual;
            MPEG::Table _others;
        };

        typedef std::list<Parser> Scanners;

    public:
        Schedules()
            : _adminLock()
            , _scanners()
            , _sink(*this)
            , _scan(true)
        {
            ITuner::Register(&_sink);
        }
        virtual ~Schedules()
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
        void Load(const DVB::EIT& table)
        {
            _adminLock.Lock();

            _adminLock.Unlock();
        }

    private:
        mutable Core::CriticalSection _adminLock;
        Scanners _scanners;
        Sink _sink;
        bool _scan;
    };

} // namespace Broadcast
} // namespace WPEFramework

#endif // SCHEDULE_ADMINISTRATOR_H
