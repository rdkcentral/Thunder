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

#ifndef PROGRAMTABLE_H
#define PROGRAMTABLE_H

#include "Definitions.h"
#include "MPEGTable.h"

namespace Thunder {

namespace Broadcast {

    class ProgramTable {
    private:
        ProgramTable()
            : _adminLock()
            , _observers()
            , _programs()
        {
        }
        ProgramTable(const ProgramTable&) = delete;
        ProgramTable& operator=(const ProgramTable&) = delete;

        static Core::ProxyPoolType<Core::DataStore> _storeFactory;

        class Observer : public ISection {
        private:
            Observer() = delete;
            Observer(const Observer&) = delete;
            Observer& operator=(const Observer&) = delete;

            typedef std::list<uint32_t> ScanMap;

        public:
            Observer(ProgramTable* parent, const uint16_t keyId, IMonitor* callback)
                : _parent(*parent)
                , _callback(callback)
                , _keyId(keyId)
                , _table(_storeFactory.Element())
                , _entries()
            {
            }
            virtual ~Observer() {}

        public:
            virtual void Handle(const MPEG::Section& section) override;

        private:
            ProgramTable& _parent;
            IMonitor* _callback;
            uint16_t _keyId;
            MPEG::Table _table;
            ScanMap _entries;
        };

        typedef std::map<uint32_t, MPEG::PMT> Programs;
        typedef std::map<IMonitor*, Observer> Observers;
        typedef std::map<uint32_t, uint16_t> NITPids;

    public:
        static ProgramTable& Instance();
        virtual ~ProgramTable() {}

    public:
        inline void Reset() { _programs.clear(); }
        ISection* Register(IMonitor* callback, const uint16_t keyId)
        {
            _adminLock.Lock();

            ASSERT(_observers.find(callback) == _observers.end());

            auto index = _observers.emplace(
                std::piecewise_construct, std::forward_as_tuple(callback),
                std::forward_as_tuple(this, keyId, callback));

            _adminLock.Unlock();

            return &(index.first->second);
        }
        void Unregister(IMonitor* callback)
        {

            _adminLock.Lock();

            Observers::iterator index(_observers.find(callback));

            if (index != _observers.end()) {
                _observers.erase(index);
            }

            _adminLock.Unlock();
        }
        inline bool Program(const uint16_t keyId, const uint16_t programId, MPEG::PMT& pmt) const
        {
            bool updated = false;

            _adminLock.Lock();

            Programs::const_iterator index(_programs.find(Key(keyId, programId)));
            if (index != _programs.end()) {
                updated = (index->second != pmt);
                pmt = index->second;
            }

            _adminLock.Unlock();

            return (updated);
        }
        uint16_t NITPid(const uint16_t keyId) const
        {
            uint16_t result(~0);

            NITPids::const_iterator index(_nitPids.find(keyId));

            if (index != _nitPids.end()) {
                result = index->second;
            }

            return (result);
        }

    private:
        inline uint32_t Key(const uint16_t keyId, const uint16_t programId) const
        {
            uint64_t result = keyId;
            return ((result << 16) | programId);
        }
        bool AddProgram(const uint16_t keyId, const MPEG::Table& data)
        {
            bool added = false;
            uint64_t key(Key(keyId, data.Extension()));

            _adminLock.Lock();

            Programs::iterator index(_programs.find(key));
            if (index != _programs.end()) {
                MPEG::PMT entry(data);
                if (index->second != entry) {
                    added = true;
                    index->second = entry;
                }
            } else {
                _programs.emplace(std::piecewise_construct, std::forward_as_tuple(key),
                    std::forward_as_tuple(data.Extension(), data.Data()));
                added = true;
            }

            _adminLock.Unlock();

            return (added);
        }

    private:
        mutable Core::CriticalSection _adminLock;
        Observers _observers;
        Programs _programs;
        NITPids _nitPids;
    };

} // namespace Broadcast
} // namespace Thunder

#endif
