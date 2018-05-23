#ifndef __WEBBRIDGESUPPORT_SERVICE__
#define __WEBBRIDGESUPPORT_SERVICE__

#include "Module.h"
#include "IPlugin.h"
#include "IShell.h"
#include "Channel.h"
#include "Configuration.h"
#include "MetaData.h"

namespace WPEFramework {
namespace PluginHost {

    class EXTERNAL Factories {
    private:
        Factories();
        Factories(const Factories&) = delete;
        Factories& operator=(const Factories&) = delete;

    public:
        static Factories& Instance();
        ~Factories();

    public:
        inline Core::ProxyType<Web::Request> Request()
        {
            return (_requestFactory.Element());
        }
        inline Core::ProxyType<Web::Response> Response()
        {
            return (_responseFactory.Element());
        }
        inline Core::ProxyType<Web::FileBody> FileBody()
        {
            return (_fileBodyFactory.Element());
        }

    private:
	friend class Core::SingletonType<Factories>;

	Core::ProxyPoolType<Web::Request> _requestFactory;
        Core::ProxyPoolType<Web::Response> _responseFactory;
        Core::ProxyPoolType<Web::FileBody> _fileBodyFactory;
    };

    class EXTERNAL WorkerPool {
    private:
        class TimedJob
        {
        public:
            TimedJob ()
                : _job()
            {
            }
            TimedJob (const Core::ProxyType<Core::IDispatchType<void> >& job)
                : _job(job)
            {
            }
            TimedJob (const TimedJob& copy)
                : _job(copy._job)
            {
            }
            ~TimedJob ()
            {
            }

            TimedJob& operator= (const TimedJob& RHS)
            {
                _job = RHS._job;
                return (*this);
            }
            bool operator== (const TimedJob& RHS) const 
            {
                return (_job == RHS._job);
            }
            bool operator!= (const TimedJob& RHS) const 
            {
                return (_job != RHS._job);
            }

        public:
            uint64_t Timed (const uint64_t /* scheduledTime */)
            {
                WorkerPool::Instance().Submit(_job);
                _job.Release();

                // No need to reschedule, just drop it..
                return (0);
            }

        private:
            Core::ProxyType<Core::IDispatchType<void> > _job;
        };

        typedef Core::ThreadPoolType<Core::Job, 6> ThreadPool;

    private:
        WorkerPool() = delete;
        WorkerPool(const WorkerPool&) = delete;
        WorkerPool& operator=(const WorkerPool&) = delete;

        WorkerPool(const uint32_t stackSize);

    public:
        static WorkerPool& Instance(const uint32_t stackSize = 0);
        ~WorkerPool();

    public:
        // A-synchronous calls. If the method returns, the workers are accepting and handling work.
        inline void Run()
        {
            _workers.Run();
        }
        // A-synchronous calls. If the method returns, the workers are all blocked, no new work will
        // be accepted. Work in progress will be completed. Use the WaitState to wait for the actual block.
        inline void Block()
        {
            _workers.Block();
        }
        inline void Wait(const uint32_t waitState, const uint32_t time)
        {
            _workers.Wait(waitState, time);
        }
        inline void Submit(const Core::ProxyType<Core::IDispatch>& job)
        {
            _workers.Submit(Core::Job(job), Core::infinite);
        }
        inline void Schedule(const Core::Time& time, const Core::ProxyType<Core::IDispatch >& job)
        {
            _timer.Schedule(time, TimedJob(job));
        }
        inline uint32_t Revoke(const Core::ProxyType<Core::IDispatch >& job, const uint32_t waitTime = Core::infinite)
        {
            // First check the timer if it can be removed from there.
            _timer.Revoke(TimedJob(job));

            // Also make sure it is taken of the WorkerPool, if applicable.
            return (_workers.Revoke(Core::Job(job), waitTime));
        }
        inline void GetMetaData(MetaData::Server& metaData) const
        {
            metaData.PendingRequests = _workers.Pending();
            metaData.PoolOccupation = _workers.Active();

            for (uint8_t teller = 0; teller < _workers.Count(); teller++) {
                // Example of why copy-constructor and assignment constructor should be equal...
                Core::JSON::DecUInt32 newElement;
                newElement = _workers[teller].Runs();
                metaData.ThreadPoolRuns.Add(newElement);
            }
        }

    private:
        ThreadPool _workers;
        Core::TimerType<TimedJob> _timer;

        friend class Core::SingletonType<WorkerPool>;
    };

    class EXTERNAL Service : public IShell {
        // This object is created by the instance that instantiates the plugins. As the lifetime
        // of this object is controlled by the server, instantiating this object, do not allow
        // this obnject to be copied or created by any other instance.
    private:
        Service() = delete;
        Service(const Service&) = delete;
        Service& operator=(const Service&) = delete;

        class EXTERNAL Config {
        private:
            Config() = delete;
            Config(const Config&) = delete;
            Config& operator=(const Config&) = delete;

        public:
            Config(const PluginHost::Config& server, const Plugin::Config& plugin)
                : _baseConfig(server)
            {
                Update(plugin);
            }
            ~Config()
            {
            }

        public:
            inline void Configuration(const string& value)
            {
                _config.Configuration = value;
            }
            inline void AutoStart(const bool value)
            {
                _config.AutoStart = value;
            }
            inline const string& Accessor() const
            {
                return (_accessor);
            }
            inline const Plugin::Config& Configuration() const
            {
                return (_config);
            }
            inline const PluginHost::Config& Information() const
            {
                return (_baseConfig);
            }
            // WebPrefix is the Fully qualified name, indicating the endpoint for this plugin.
            inline const string& WebPrefix() const
            {
                return (_webPrefix);
            }

            // PersistentPath is a path to a location where the plugin instance can store data needed
            // by the plugin instance, hence why the callSign is included. .
            // This path is build up from: PersistentPath / callSign /
            inline const string& PersistentPath() const
            {
                return (_persistentPath);
            }

            // VolatilePath is a path to a location where the plugin instance can store data needed
            // by the plugin instance, hence why the callSign is included. .
            // This path is build up from: PersistentPath / callSign /
            inline const string& VolatilePath() const
            {
                return (_volatilePath);
            }
 
            // DataPath is a path, to a location (read-only to be used to store
            // This path is build up from: DataPath / className /
            inline const string& DataPath() const
            {
                return (_dataPath);
            }

            inline void Update(const Plugin::Config& config)
            {
                const string& callSign(config.Callsign.Value());

                _config = config;
                _webPrefix = _baseConfig.WebPrefix() + '/' + callSign;
                _persistentPath = _baseConfig.PersistentPath() + callSign + '/';
                _dataPath = _baseConfig.DataPath() + config.ClassName.Value() + '/';
		_volatilePath = _baseConfig.VolatilePath() + config.ClassName.Value() + '/';

		// Volatile means that the path could not have been created, create it for now.
                Core::Directory(_volatilePath.c_str()).CreatePath();

                if (_baseConfig.Accessor().PortNumber() == 80) {
                    _accessor = string(_T("http://")) + _baseConfig.Accessor().HostAddress() + _webPrefix;
                }
                else {
                    _accessor = string(_T("http://")) + _baseConfig.Accessor().HostAddress() + ':' + Core::NumberType<uint16_t>(_baseConfig.Accessor().PortNumber()).Text() + _webPrefix;
                }
            }

        private:
            const PluginHost::Config& _baseConfig;
            Plugin::Config _config;

            string _webPrefix;
            string _persistentPath;
            string _volatilePath;
            string _dataPath;
            string _accessor;
        };

    public:
        Service(const PluginHost::Config& server, const Plugin::Config& plugin)
            :
#ifdef RUNTIME_STATISTICS
            _processedRequests(0)
            , _processedObjects(0)
            ,
#endif
            _state(DEACTIVATED)
            , _config(server, plugin)
            , _notifiers()
        {
        }
        ~Service()
        {
        }

    public:
        bool IsWebServerRequest(const string& segment) const;
        void Notification(const string& message);

        inline string ClassName() const
        {
            return (_config.Configuration().ClassName.Value());
        }
        virtual string Version () const
        {
            return (_config.Information().Version());
        }
        virtual string Model () const
        {
            return (_config.Information().Model());
        }
        virtual bool Background() const
        {
            return (_config.Information().Background());
        }
        virtual string Locator() const
        {
            return (_config.Configuration().Locator.Value());
        }
        virtual string Callsign() const
        {
            return (_config.Configuration().Callsign.Value());
        }
        virtual string WebPrefix() const
        {
            return (_config.WebPrefix());
        }
        virtual string Accessor() const
        {
            return (_config.Accessor());
        }
        virtual string ConfigLine() const
        {
            return (_config.Configuration().Configuration.Value());
        }
        virtual string PersistentPath() const
        {
            return (_config.PersistentPath());
        }
        virtual string VolatilePath() const
        {
            return (_config.VolatilePath());
        }
        virtual string DataPath() const
        {
            return (_config.DataPath());
        }
        virtual string HashKey() const
        {
            return (_config.Information().HashKey());
        }
        virtual state State() const
        {
            return (_state);
        }
        virtual bool AutoStart() const
        {
            return (_config.Configuration().AutoStart.Value());
        }
        inline const Plugin::Config& Configuration() const
        {
            return (_config.Configuration());
        }
        inline const PluginHost::Config& Information() const
        {
            return (_config.Information());
        }
        inline bool IsActive() const
        {
            return (_state == ACTIVATED);
        }
        inline bool HasError() const
        {
            return (_errorMessage.empty() == false);
        }
        inline const string& ErrorMessage() const
        {
            return (_errorMessage);
        }
        inline void GetMetaData(MetaData::Service& metaData) const
        {
            metaData = _config.Configuration();
			metaData.Observers = _notifiers.size();
            metaData.JSONState = this;

#ifdef RUNTIME_STATISTICS
            metaData.ProcessedRequests = _processedRequests;
            metaData.ProcessedObjects = _processedObjects;
#endif
        }
 
        // As a service, the plugin could act like a WebService. The Webservice hosts files from a location over the
        // HTTP protocol. This service is hosting files at:
        // http://<bridge host ip>:<bridge port>/Service/<Callsign>/<PostFixURL>/....
        // Root directory of the files to be services by the URL are in case the passed in value is empty:
        // <DataPath>/<PostFixYRL>
        virtual void EnableWebServer(const string& postFixURL, const string& fileRootPath)
        {
            // The postFixURL should *NOT* contain a starting or trailing slash!!!
            ASSERT((postFixURL.length() > 0) && (postFixURL[0] != '/') && (postFixURL[postFixURL.length() - 1] != '/'));

            // The postFixURL should *NOT* contain a starting or trailing slash!!!
            // We signal the request to service web files via a non-empty _webServerFilePath.
            _webURLPath = postFixURL;

            if (fileRootPath.empty() == true) {
                _webServerFilePath = _config.DataPath() + postFixURL + '/';
            }
            else {
                // File path needs to end in a slash to indicate it is a directory and not a file.
                ASSERT(fileRootPath[fileRootPath.length() - 1] == '/');

                _webServerFilePath = fileRootPath;
            }
        }
        virtual void DisableWebServer()
        {
            // We signal the request to ervice web files via a non-empty _webServerFilePath.
            _webServerFilePath.clear();
            _webURLPath.clear();
        }
        virtual Core::ProxyType<Core::JSON::IElement> Inbound(const string& identifier) = 0;

        virtual void Notify(const string& message) = 0;

        virtual void* QueryInterface(const uint32_t id) = 0;
        virtual void* QueryInterfaceByCallsign(const uint32_t id, const string& name) = 0;
        virtual void Register(IPlugin::INotification* sink) = 0;
        virtual void Unregister(IPlugin::INotification* sink) = 0;

        // Use the base framework (webbridge) to start/stop processes and the service in side of the given binary.
        virtual void* Instantiate(const uint32_t waitTime, const string& className, const uint32_t interfaceId, const uint32_t version, uint32_t& pid, const string& locator) = 0;
		virtual void Register(RPC::IRemoteProcess::INotification* sink) = 0;
		virtual void Unregister(RPC::IRemoteProcess::INotification* sink) = 0;
		virtual RPC::IRemoteProcess* RemoteProcess(const uint32_t pid) = 0;

        // Methods to Activate and Deactivate the aggregated Plugin to this shell.
        // These are Blocking calls!!!!!
        virtual uint32_t Activate(const PluginHost::IShell::reason) = 0;
        virtual uint32_t Deactivate(const PluginHost::IShell::reason) = 0;

        inline uint32_t ConfigLine(const string& newConfiguration)
        {
            uint32_t result = Core::ERROR_ILLEGAL_STATE;

            Lock();

            if (State() == PluginHost::IShell::DEACTIVATED) {

                // Time to update the config line...
                _config.Configuration(newConfiguration);

                result = Core::ERROR_NONE;
            }

            Unlock();

            return (result);
        }
        inline uint32_t AutoStart(const bool autoStart)
        {
            uint32_t result = Core::ERROR_ILLEGAL_STATE;

            Lock();

            if (State() == PluginHost::IShell::DEACTIVATED) {

                // Time to update the config line...
                _config.AutoStart(autoStart);

                result = Core::ERROR_NONE;
            }

            Unlock();

            return (result);
        }


    protected:
        inline void Lock() const
        {
            _adminLock.Lock();
        }
        inline void Unlock() const
        {
            _adminLock.Unlock();
        }
        inline void State(const state value)
        {
            _state = value;
        }
        inline void ErrorMessage(const string& message)
        {
            _errorMessage = message;
        }
        inline bool Subscribe(Channel& channel)
        {
            _notifierLock.Lock();

            bool result = std::find(_notifiers.begin(), _notifiers.end(), &channel) == _notifiers.end();

            if (result == true) {
                if (channel.IsNotified() == true) {
                    _notifiers.push_back(&channel);
                }
            }

			_notifierLock.Unlock();

            return (result);
        }
        inline void Unsubscribe(Channel& channel)
        {

			_notifierLock.Lock();

            std::list<Channel*>::iterator index(std::find(_notifiers.begin(), _notifiers.end(), &channel));

            if (index != _notifiers.end()) {
                _notifiers.erase(index);
            }

			_notifierLock.Unlock();
        }

#ifdef RUNTIME_STATISTICS
        inline void IncrementProcessedRequests()
        {
            _processedRequests++;
        }
        inline void IncrementProcessedObjects()
        {
            _processedObjects++;
        }
#endif
        void FileToServe(const string& webServiceRequest, Web::Response& response);

    private:
        mutable Core::CriticalSection _adminLock;
		Core::CriticalSection _notifierLock;

#ifdef RUNTIME_STATISTICS
        uint32_t _processedRequests;
        uint32_t _processedObjects;
#endif

        state _state;
        Config _config;
        string _errorMessage;

        // In case the Service also has WebPage availability, these variables
        // contain the URL path and the start of the path location on disk.
        string _webURLPath;
        string _webServerFilePath;

        // Keep track of people who want to be notified of changes.
        std::list<Channel*> _notifiers;
    };
}
}

#endif // __WEBBRIDGESUPPORT_SERVICE__
