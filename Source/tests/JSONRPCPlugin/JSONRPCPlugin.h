#pragma once

#include "Data.h"
#include "Module.h"

namespace WPEFramework {

namespace Plugin {

    // This is a server for a JSONRPC communication channel.
    // For a plugin to be capable to handle JSONRPC, inherit from PluginHost::JSONRPC.
    // By inheriting from this class, the plugin realizes the interface PluginHost::IDispatcher.
    // This realization of this interface implements, by default, the following methods on this plugin
    // - exists
    // - register
    // - unregister
    // Any other methood to be handled by this plugin  can be added can be added by using the
    // templated methods Rgister on the PluginHost::JSONRPC class.
    // As the registration/unregistration of notifications is realized by the class PluginHost::JSONRPC,
    // this class exposes a public method called, Notify(), using this methods, all subscribed clients
    // will receive a JSONRPC message as a notification, in case this method is called.
    class JSONRPCPlugin : public PluginHost::IPlugin, public PluginHost::JSONRPC {
    private:
        // We do not allow this plugin to be copied !!
        JSONRPCPlugin(const JSONRPCPlugin&) = delete;
        JSONRPCPlugin& operator=(const JSONRPCPlugin&) = delete;

        // The next class is a helper class, just to trigger an a-synchronous callback every Period()
        // amount of time.
        class PeriodicSync : public Core::IDispatch {
        private:
            PeriodicSync() = delete;
            PeriodicSync(const PeriodicSync&) = delete;
            PeriodicSync& operator=(const PeriodicSync&) = delete;

        public:
            PeriodicSync(JSONRPCPlugin* parent)
                : _parent(*parent)
            {
            }
            ~PeriodicSync()
            {
            }

        public:
            void Period(const uint8_t time)
            {
                _nextSlot = (time * 1000);
            }
            // This method is called by the main process ThreadPool at the scheduled time.
            // After the parent has been called to send out a-synchronous notifications, it
            // will schedule itself again, to be triggered after the set period.
            virtual void Dispatch() override
            {
                _parent.SendTime();

                if (_nextSlot != 0) {
                    PluginHost::WorkerPool::Instance().Schedule(Core::Time::Now().Add(_nextSlot), Core::ProxyType<Core::IDispatch>(*this));
                }
            }

        private:
            uint32_t _nextSlot;
            JSONRPCPlugin& _parent;
        };

        // Define a handler for incoming JSONRPC messages. This method does not take any
        // parameters, it just returns the current time of this server, if it is called.
        uint32_t time(const Core::JSON::String& /* parameters */, Core::JSON::String& response)
        {
            response = Core::Time::Now().ToRFC1123();
            return (Core::ERROR_NONE);
        }

        // If the parameters are more complex (aggregated JSON objects) us JSON container
        // classes.
        uint32_t extended(const Data::Parameters& params, Data::Response& response)
        {
            if (params.UTC.Value() == true) {
                response.Time = Core::Time::Now().Ticks();
            } else {
                response.Time = Core::Time::Now().Add(60 * 60 * 100).Ticks();
            }
            if (params.Location.Value() == _T("BadDay")) {
                response.State = Data::Response::INACTIVE;
            } else {
                response.State = Data::Response::IDLE;
            }
            return (Core::ERROR_NONE);
        }
        uint32_t set_geometry(const Data::Geometry& window)
        {
            _window.X = window.X;
            _window.Y = window.Y;
            _window.Width = window.Width;
            _window.Height = window.Height;
            return (Core::ERROR_NONE);
        }
        uint32_t get_geometry(Data::Geometry& window) const
        {
            window.X = _window.X;
            window.Y = _window.Y;
            window.Width = _window.Width;
            window.Height = _window.Height;
            return (Core::ERROR_NONE);
        }
        uint32_t get_data(Core::JSON::String& data) const
        {
            data = _data;
            return (Core::ERROR_NONE);
        }
        uint32_t set_data(const Core::JSON::String& data)
        {
            _data = data.Value();
            return (Core::ERROR_NONE);
        }

    public:
        JSONRPCPlugin();
        virtual ~JSONRPCPlugin();

        // Build QueryInterface implementation, specifying all possible interfaces to be returned.
        BEGIN_INTERFACE_MAP(JSONRPCPlugin)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
        END_INTERFACE_MAP

    public:
        //   IPlugin methods
        // -------------------------------------------------------------------------------------------------------
        virtual const string Initialize(PluginHost::IShell* service) override;
        virtual void Deinitialize(PluginHost::IShell* service) override;
        virtual string Information() const override;

        //   Private methods specific to this class.
        // -------------------------------------------------------------------------------------------------------
        void SendTime();

    private:
        Core::ProxyType<PeriodicSync> _job;
        Data::Window _window;
        string _data;
    };

} // namespace Plugin
} // namespace WPEFramework
