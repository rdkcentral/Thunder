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

        enum state {
            IDLE,
            NIT,
            SDT,
            EIT
        };

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

        ServiceAdministrator()
            : _adminLock()
            , _scanners()
            , _sink(*this) {
            TunerAdministrator::Instance().Register(&_sink);
        }

        typedef std::pair<ITuner*, state> ScanState;
        typedef std::list<ScanState> Scanners;

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
