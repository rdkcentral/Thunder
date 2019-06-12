#include "Module.h"

#include "ProcessContainer.h"

#include "Tracing.h"
#include <lxc/lxccontainer.h>
#include <vector>
#include <utility>

namespace WPEFramework {

class LXCContainerAdministrator : public ProcessContainers::IContainerAdministrator {
private:
    using LxcContainerType = struct lxc_container;
public:
    LXCContainerAdministrator(const LXCContainerAdministrator&) = delete;
    LXCContainerAdministrator& operator=(const LXCContainerAdministrator&) = delete;

    LXCContainerAdministrator() 
        : _lock()
        , _searchpaths() {


            printf("LXC Version:%s\n", lxc_get_version());

            lxc_log log;
            log.name = "huppel";
            log.lxcpath = "/home/marcelf/.local/share/lxc/";
            log.file = "/usr/src/huppel.log";
            log.level = "0";
            log.prefix = "huppel";
            log.quiet = false;

            int result = lxc_log_init(&log);
            printf("LXC log init: %i\n",result);

        }

    virtual ~LXCContainerAdministrator() {
            lxc_log_close();
    }

    class LCXContainer : public ProcessContainers::IContainerAdministrator::IContainer {
    public:
        LCXContainer(const LCXContainer&) = delete;
        LCXContainer& operator=(const LCXContainer&) = delete;

        LCXContainer(const string& name, LxcContainerType* lxccontainer)
            : _name(name)
            , _referenceCount(1)
            , _lxccontainer(lxccontainer) {
        }

        const string& Id() const override {
            return _name;
        }

        void Start(const string& command, ProcessContainers::IStringIterator& parameters) override;
        bool Stop(const uint32_t timeout /*ms*/, const bool kill) override;

        void AddRef() const override {
            WPEFramework::Core::InterlockedIncrement(_referenceCount);
            lxc_container_get(_lxccontainer);
        }

        uint32_t Release() const override {
            uint32_t retval = WPEFramework::Core::ERROR_NONE;
            uint32_t lxcresult = lxc_container_put(_lxccontainer);
            if (WPEFramework::Core::InterlockedDecrement(_referenceCount) == 0) {
                ASSERT(lxcresult == 1); // if 1 is returned, lxc also released the container
//                TRACE(ProcessContainers::ProcessContainerization, (_T("Container Definition with name %s released"), _name));

                delete this;
                retval = WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED;

            }
            return retval;
        }

        private:
            const string _name;
            mutable uint32_t _referenceCount;
            LxcContainerType* _lxccontainer;
    };

    void ContainerDefinitionSearchPaths(ProcessContainers::IStringIterator& searchpaths) override {
        _lock.Lock();
        searchpaths.Reset(0);
        while( searchpaths.Next() == true ) {
            _searchpaths.emplace_back(searchpaths.Current());
        }
        _lock.Unlock();
    }

    // Lifetime management
     void AddRef() const override {
    }
    uint32_t Release() const override {
        return Core::ERROR_NONE;
    }

    ProcessContainers::IContainerAdministrator::IContainer* Container(const string& name) override;

    string GetNames() const override;

private:
    using SearchPathContainer = std::vector<string>;

    mutable Core::CriticalSection _lock;
    SearchPathContainer _searchpaths;
};

ProcessContainers::IContainerAdministrator::IContainer* LXCContainerAdministrator::Container(const string& name) {

    _lock.Lock();

    _searchpaths.push_back({string("/home/marcelf/.local/share/lxc/")});  // todo remove, after configuration in place. let's first get the container to work

    ProcessContainers::IContainerAdministrator::IContainer* container { nullptr };
    SearchPathContainer::const_iterator searchpath { _searchpaths.cbegin() };

    while( ( container == nullptr ) && ( searchpath != _searchpaths.cend() ) )  {
        LxcContainerType **clist = nullptr;
        int32_t numberofcontainersfound = list_defined_containers((*searchpath).c_str(), nullptr, &clist);
        int32_t index = 0;
        while( ( container == nullptr) && ( index < numberofcontainersfound ) ) {
            LxcContainerType *c = clist[index];
            if( name == c->name ) {
//                TRACE(ProcessContainers::ProcessContainerization, (_T("Container Definition with name %s retreived from %s."), c->name, (*searchpath).c_str()));
                // huppel move config code code or remove (doesn't seem to work)
                c->set_config_item(c, "lxc.console.buffer.size", "4096");
                c->set_config_item(c, "lxc.console.logfile", "/usr/src/containerconsole.log");
                container = new LXCContainerAdministrator::LCXContainer(name, c);
            }
            else {
                lxc_container_put(c);
            }
            ++index;
        };
        if( numberofcontainersfound > 0 ) {
            free(clist);
        }
        ++searchpath;
    };

    _lock.Unlock();

    return container;
}

string LXCContainerAdministrator::GetNames() const {
    char **names;
    int32_t result = list_defined_containers("/home/marcelf/.local/share/lxc/", &names, NULL);
    string namecollection;
    for (int32_t i = 0; i < result; ++i) {
        if( i > 0 ) {
            namecollection += ", ";
        }
        namecollection += names[i];
		free(names[i]);
	}
    if( result > 0 ) {
        free(names);
    }
    return namecollection;
}

ProcessContainers::IContainerAdministrator& ProcessContainers::IContainerAdministrator::Instance()
{
    static LXCContainerAdministrator& myLXCContainerAdministrator = Core::SingletonType<LXCContainerAdministrator>::Instance();

    return myLXCContainerAdministrator;
}

void LXCContainerAdministrator::LCXContainer::Start(const string& command, ProcessContainers::IStringIterator& parameters) {
//    bool retval = _lxccontainer->stop(_lxccontainer);
//    printf("Container stop: %i\n", retval);

//    retval = _lxccontainer->shutdown(_lxccontainer, 5);
//    printf("Container shutdown: %i\n", retval);


//    bool retval = _lxccontainer->start(_lxccontainer, 0, NULL);
//    printf("Container start: %i\n", retval);

//    lxc_attach_command_t lxccommand;
//	pid_t pid;

//    char* program = "/usr/src/HuppelWIP/CurrentVersion/staging/usr/bin/WPEProcess";

/*    char* argv[] = {"/usr/src/HuppelWIP/CurrentVersion/staging/usr/bin/WPEProcess", "-a", "/usr/src/HuppelWIP/CurrentVersion/staging/usr/bin/", "-c", "WebServerImplementation", 
    "-d", "/usr/src/HuppelWIP/CurrentVersion/staging/usr/share/WPEFramework/WebServer/", "-e", "7", "-i", "102", "-l" ,"libWPEFrameworkWebServer.so", "-m" "/usr/src/HuppelWIP/CurrentVersion/staging/usr/lib/wpeframework/proxystubs/",
    "-p" ,"/root/WebServer/", "-r", "/tmp/communicator", "-s", "/usr/src/HuppelWIP/CurrentVersion/staging/usr/lib/wpeframework/plugins/", "-x 1", NULL};*/
    
/*    char* argv[] = {"/usr/src/HuppelWIP/CurrentVersion/staging/usr/bin/WPEProcess", "-a /usr/src/HuppelWIP/CurrentVersion/staging/usr/bin/", "-c WebServerImplementation", 
    "-d /usr/src/HuppelWIP/CurrentVersion/staging/usr/share/WPEFramework/WebServer/", "-e 7", "-i 102", "-l libWPEFrameworkWebServer.so", "-m /usr/src/HuppelWIP/CurrentVersion/staging/usr/lib/wpeframework/proxystubs/",
    "-p /root/WebServer/", "-r /tmp/communicator", "-s /usr/src/HuppelWIP/CurrentVersion/staging/usr/lib/wpeframework/plugins/", NULL};*/

//    lxccommand.program = (char *)program;
//    lxccommand.argv = argv;

/*&    char* params[2];
    params[0] = const_cast<char*>(parameters.c_str());
    params[1] = NULL;

    lxccommand.program = (char *)program;
    lxccommand.argv = params;
*/
//    lxc_attach_options_t options = LXC_ATTACH_OPTIONS_DEFAULT;
//    options.uid = 1000;
//    options.gid = 1000;

 //   int ret = _lxccontainer->attach(_lxccontainer, lxc_attach_run_command, &lxccommand, &options, &pid);
 //   printf("Attach to container: %i\n", ret);

    /*bool retval = _lxccontainer->startl(_lxccontainer, 1, "/usr/src/HuppelWIP/CurrentVersion/staging/usr/bin/WPEProcess", "-a", "/usr/src/HuppelWIP/CurrentVersion/staging/usr/bin/", "-c", "WebServerImplementation", 
    "-d",  "/usr/src/HuppelWIP/CurrentVersion/staging/usr/share/WPEFramework/WebServer/", "-e", "7", "-i", "102", "-l", "libWPEFrameworkWebServer.so", "-m", "/usr/src/HuppelWIP/CurrentVersion/staging/usr/lib/wpeframework/proxystubs/",
    "-p", "/root/WebServer/", "-r", "/tmp/communicator", "-s", "/usr/src/HuppelWIP/CurrentVersion/staging/usr/lib/wpeframework/plugins/", NULL);   
*/
/*    bool retval = _lxccontainer->startl(_lxccontainer, 1, "/usr/bin/WPEProcess", "-a /usr/bin/", "-c WebServerImplementation", 
    "-d /usr/share/WPEFramework/WebServer/", "-e 7", "-i 102", "-l libWPEFrameworkWebServer.so", "-m /usr/lib/wpeframework/proxystubs/",
    "-p /root/WebServer/", "-r /tmp/communicator", "-s /usr/lib/wpeframework/plugins/", NULL);    */

//     retval = _lxccontainer->startl(_lxccontainer, 1, "/usr/bin/Testapp", NULL);
//    bool retval = _lxccontainer->startl(_lxccontainer, 1, "/usr/src/HuppelWIP/CurrentVersion/staging/usr/bin/Testapp", NULL);

/*
    huppel: can only be used in single threaded app
    bool retval = _lxccontainer->want_daemonize(_lxccontainer, false);
    printf("container to daemonize: %i\n", retval);

*/

    std::vector<const char*> params(parameters.Count()+2);
    parameters.Reset(0);
    uint16_t pos = 0;
    params[pos++] = command.c_str();

    while( parameters.Next() == true ) {
        params[pos++] = parameters.Current().c_str();
    }
    params[pos++] = nullptr;
    ASSERT(pos == parameters.Count()+2);

    if(true) {
        params.insert(params.begin(), command.c_str());
        bool retval = _lxccontainer->start(_lxccontainer, 0, const_cast<char**>(params.data()));
        printf("start command in container: %i\n", retval);
    } else {
        bool retval = _lxccontainer->start(_lxccontainer, 0, NULL);
        printf("Container start: %i\n", retval);

        lxc_attach_command_t lxccommand;
        lxccommand.program = (char *)command.c_str();
        lxccommand.argv = const_cast<char**>(params.data());

        lxc_attach_options_t options = LXC_ATTACH_OPTIONS_DEFAULT;
	    pid_t pid;
        int ret = _lxccontainer->attach(_lxccontainer, lxc_attach_run_command, &lxccommand, &options, &pid);
        printf("Attach to container: %i\n", ret);
    }

    if(false) {
        ::SleepMs(2000);
        lxc_console_log log;
        uint64_t sizeread = 0; // note will not allocate memory
        log.read_max = &sizeread;
        log.read = true;
        log.clear = false;
        int retint = _lxccontainer->console_log(_lxccontainer, &log);
        printf("Get console log: %i\n", retint);
        if( retint == 0 && sizeread != 0 ) {
            printf("Get console value: %s\n", log.data);
        }
        sizeread = 0;
        log.read = false;
        log.clear = true;
        retint = _lxccontainer->console_log(_lxccontainer, &log);
        printf("Get console log: %i\n", retint);
    } 
}

bool LXCContainerAdministrator::LCXContainer::Stop(const uint32_t timeout /*ms*/, const bool kill) {
    bool result = false;
    if( kill == false ) {
        int internaltimeout = timeout/1000;
        if( timeout == Core::infinite ) {
            internaltimeout = -1;
        }
        result = _lxccontainer->shutdown(_lxccontainer, internaltimeout);
    } else {
        result = _lxccontainer->stop(_lxccontainer);
    }
    return result;
}

} //namespace WPEFramework 



