#ifndef PROGRAMTABLE_H
#define PROGRAMTABLE_H

#include "Definitions.h"
#include "MPEGTable.h"

namespace WPEFramework {

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
            Observer(ProgramTable* parent, const uint32_t frequency, IMonitor* callback)
                : _parent(*parent)
                , _callback(callback)
                , _frequency(frequency)
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
            uint32_t _frequency;
            MPEG::Table _table;
            ScanMap _entries;
        };

        typedef std::map<uint64_t, MPEG::PMT> Programs;
        typedef std::map<IMonitor*, Observer> Observers;

    public:
        static ProgramTable& Instance() { return (_instance); }
        virtual ~ProgramTable() {}

    public:
        inline void Reset() { _programs.clear(); }
        ISection* Register(IMonitor* callback, const uint32_t frequency)
        {

            _adminLock.Lock();

            ASSERT(_observers.find(callback) == _observers.end());

            auto index = _observers.emplace(
                std::piecewise_construct, std::forward_as_tuple(callback),
                std::forward_as_tuple(this, frequency, callback));

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
        inline bool Program(const uint32_t frequency, const uint16_t programId,
            MPEG::PMT& pmt) const
        {
            bool updated = false;

            _adminLock.Lock();

            Programs::const_iterator index(_programs.find(Key(frequency, programId)));
            if (index != _programs.end()) {
                updated = (index->second != pmt);
                pmt = index->second;
            }

            _adminLock.Unlock();

            return (updated);
        }

    private:
        inline uint64_t Key(const uint32_t frequency,
            const uint16_t programId) const
        {
            uint64_t result = frequency;
            return ((result << 16) | programId);
        }
        bool AddProgram(const uint32_t frequency, const MPEG::Table& data)
        {
            bool added = false;
            uint64_t key(Key(frequency, data.Extension()));

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

        static ProgramTable _instance;
    };

} // namespace Broadcast
} // namespace WPEFramework

#endif
