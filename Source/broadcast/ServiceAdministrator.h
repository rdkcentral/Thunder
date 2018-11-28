#ifndef SERVICE_ADMINISTRATOR_H
#define SERVICE_ADMINISTRATOR_H

#include "Definitions.h"
#include "MPEGTable.h"
#include "SDT.h"
#include "TunerAdministrator.h"

namespace WPEFramework {

namespace Broadcast {

    class ServiceAdministrator {
    private:
        ServiceAdministrator(const ServiceAdministrator&) = delete;
        ServiceAdministrator& operator=(const ServiceAdministrator&) = delete;

        class Sink : public TunerAdministrator::INotification {
        private:
            Sink() = delete;
            Sink(const Sink&) = delete;
            Sink& operator= (const Sink&) = delete;

        public:
            Sink(ServiceAdministrator& parent) : _parent (parent) {
            }
            virtual ~Sink() {
            }

        public:
            virtual void Activated(ITuner* tuner) override {
                _parent.Activated(tuner);
            }
            virtual void Deactivated(ITuner* tuner) override {
                _parent.Deactivated(tuner);
            }
            virtual void StateChange(ITuner* tuner) override {
                _parent.StateChange(tuner);
            }

        private:
            ServiceAdministrator& _parent;
        };

        class Parser : public ISection {
        public:
            enum state {
                IDLE,
                NIT,
                SDT,
                EIT
            };

        private:
            Parser() = delete;
            Parser(const Parser&) = delete;
            Parser& operator= (const Parser&) = delete;

        public:
            Parser(ServiceAdministrator& parent, ITuner* source) 
                : _parent(parent)
                , _source(source)
                , _state(IDLE)
                , _pid(~0)
                , _table(~0) {
            }
            virtual ~Parser() {
            }

            inline bool operator== (const Parser& rhs) const {
                return(_source == rhs._source);
            }
            inline bool operator!= (const Parser& rhs) const {
                return(!operator==(rhs));
            }
            inline bool operator== (const ITuner* rhs) const {
                return(_source == rhs);
            }
            inline bool operator!= (const ITuner* rhs) const {
                return(!operator==(rhs));
            }

        public:
            void Open(const state newState, const uint16_t pid) {
                Close();
                switch(newState) {
                case IDLE:
                     _state = newState;
                     break;
                case NIT:
                     _state = newState;
                     break;
                case SDT:
                     _state = newState;
                     break;
                case EIT:
                     _state = newState;
                     break;
                default:
                     ASSERT(false);
                     break;
                }
            }
            void Close() {
                if (_pid != static_cast<uint16_t>(~0)) {
                    _source->Filter(_pid, _table, nullptr);
                }
            }

        private:
            virtual void Handle(const MPEG::Section& section) override;

        private:
            ServiceAdministrator& _parent;
            ITuner* _source;
            state _state;
            uint16_t _pid;
            uint8_t _table;
        };

        ServiceAdministrator()
            : _adminLock()
            , _scanners()
            , _sink(*this) {
            TunerAdministrator::Instance().Register(&_sink);
        }

        typedef std::list<Parser> Scanners;

    public:
        static ServiceAdministrator& Instance();
        virtual ~ServiceAdministrator() {
            // This will *NOT* work as this instantiated the
            // static tuner, it will be destructed before the
            // ServiceAdministrator is destructed !!!!
            // TunerAdministrator::Instance().Unregister(&_sink);
        }

    public:

    private:
        void Activated(ITuner* tuner);
        void Deactivated(ITuner* tuner);
        void StateChange(ITuner* tuner);

    private:
        Core::CriticalSection _adminLock;
        Scanners _scanners;
        Sink _sink;
    };

} // namespace Broadcast
} // namespace WPEFramework

#endif // SERVICE_ADMINISTRATOR_H
