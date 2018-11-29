#ifndef SERVICE_ADMINISTRATOR_H
#define SERVICE_ADMINISTRATOR_H

#include "Definitions.h"
#include "MPEGTable.h"
#include "NIT.h"
#include "SDT.h"
#include "TDT.h"
#include "ProgramTable.h"
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
                METADATA,
                SCHEDULE
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
                , _NIT(Core::ProxyType<Core::DataStore>::Create(512))
                , _SDT(Core::ProxyType<Core::DataStore>::Create(512)) {
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
            state State() const {
                return (_state);
            }
            void Open() {
                if (_state == IDLE) {
                    _pid = ProgramTable::Instance().NITPid(_source->Id());
                    if (_pid != static_cast<uint16_t>(~0)) {
                        _state = METADATA;

                        // Start loading the NIT info
                        _source->Filter(_pid, DVB::NIT::ACTUAL, this);

                        // Start loading the SDT info
                        _source->Filter(0x11, DVB::SDT::ACTUAL, this);

                        // Start loading the TDT info
                        _source->Filter(0x14, DVB::TDT::ID, this);
                    }
                }
            }
            void Close() {
                if (_state == METADATA) {
                    _source->Filter(0x14, DVB::TDT::ID, nullptr);
                    _source->Filter(0x11, DVB::SDT::ACTUAL, nullptr);
                    _source->Filter(_pid, DVB::NIT::ACTUAL, nullptr);
                    _state = IDLE;
                }
                else if (_state == SCHEDULE) {
                }
            }

        private:
            virtual void Handle(const MPEG::Section& section) override;

        private:
            ServiceAdministrator& _parent;
            ITuner* _source;
            state _state;
            uint16_t _pid;
            MPEG::Table _NIT;
            MPEG::Table _SDT;
        };

        class EXTERNAL ServiceInfo {
        private:
            ServiceInfo() = delete;
            ServiceInfo(const ServiceInfo&) = delete;
            ServiceInfo& operator= (const ServiceInfo&) = delete;

        public:
            ServiceInfo (
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
        const Service& Program

    private:
        void Activated(ITuner* tuner);
        void Deactivated(ITuner* tuner);
        void StateChange(ITuner* tuner);
        void Load(const DVB::NIT& table);
        void Load(const DVB::SDT& table);
        void Load(const DVB::TDT& table);

    private:
        Core::CriticalSection _adminLock;
        Scanners _scanners;
        Sink _sink;
        std::map<uint16_t, Service> _services;
    };

} // namespace Broadcast
} // namespace WPEFramework

#endif // SERVICE_ADMINISTRATOR_H
