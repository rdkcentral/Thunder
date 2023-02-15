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

#pragma once

#include "Module.h"
#include "Trace.h"
#include "Sync.h"
#include "Thread.h"
#include "FileSystem.h"

namespace WPEFramework {
namespace Core {

#ifdef __LINUX__
#include <sys/inotify.h>

class FileSystemMonitor : public Core::IResource {
public:
    struct ICallback
    {
        virtual ~ICallback() = default;
        virtual void Updated() = 0;
    };

private:
    class Observer {
    public:
        Observer(ICallback *callback)
            : _callbacks()
        {
            _callbacks.emplace_back(callback);
        }
        ~Observer()
        {
        }

    public:
        bool HasCallbacks() const
        {
            return (_callbacks.size() > 0);
        }
        void Register(ICallback *callback)
        {
            ASSERT(std::find(_callbacks.begin(),_callbacks.end(), callback) == _callbacks.end());
            _callbacks.emplace_back(callback);
        }
        void Unregister(ICallback *callback)
        {
            std::list<ICallback *>::iterator index = std::find(_callbacks.begin(), _callbacks.end(), callback);
            ASSERT(index != _callbacks.end());

            if (index != _callbacks.end()) {
                _callbacks.erase(index);
            }
        }
        void Notify()
        {
            std::list<ICallback *>::iterator index(_callbacks.begin());
            while (index != _callbacks.end()) {
                (*index)->Updated();
                index++;
            }
        }
    private:
        std::list<ICallback *> _callbacks;
    };

    typedef std::unordered_map<int, Observer> Observers;
    typedef std::unordered_map<string, int> Files;

    FileSystemMonitor()
        : _adminLock()
        , _notifyFd(inotify_init1(IN_NONBLOCK|IN_CLOEXEC))
        , _files()
        , _observers()
    {
    }

public:
    FileSystemMonitor(const FileSystemMonitor &) = delete;
    FileSystemMonitor &operator=(const FileSystemMonitor &) = delete;

    static FileSystemMonitor &Instance()
    {
        static FileSystemMonitor _singleton;
        return (_singleton);
    }
    ~FileSystemMonitor()
    {
        if (_notifyFd != -1) {
            ::close(_notifyFd);
        }
    }

public:
    bool IsValid() const
    {
        return (_notifyFd != -1);
    }
    bool Register(ICallback *callback, const string &filename)
    {
        ASSERT(_notifyFd != -1);
        ASSERT(callback != nullptr);

        const string path = ((Core::File(filename).IsDirectory() == true)? Core::Directory::Normalize(filename) : filename);

        _adminLock.Lock();

        Files::iterator index = _files.find(path);
        if (index != _files.end()) {
            Observers::iterator loop = _observers.find(index->second);
            ASSERT(loop != _observers.end());

            loop->second.Register(callback);
        }
        else
        {
            const uint32_t mask = Core::File(path).IsDirectory()? (IN_CREATE | IN_CLOSE_WRITE | IN_DELETE | IN_MOVE | IN_DELETE_SELF)
                                            : (IN_MODIFY | IN_CLOSE_WRITE | IN_DELETE_SELF | IN_MOVE_SELF);

            int fileFd = inotify_add_watch(_notifyFd, path.c_str(), mask);
            if (fileFd >= 0) {
                _files.emplace(std::piecewise_construct,
                    std::forward_as_tuple(path),
                    std::forward_as_tuple(fileFd));
                _observers.emplace(std::piecewise_construct,
                    std::forward_as_tuple(fileFd),
                    std::forward_as_tuple(callback));

                if (_files.size() == 1) {
                    // This is the first entry, lets start monitoring
                    Core::ResourceMonitor::Instance().Register(*this);
                }
            }
        }

        _adminLock.Unlock();

        return (IsValid());
    }
    void Unregister(ICallback *callback, const string &filename)
    {
        ASSERT(_notifyFd != -1);
        ASSERT(callback != nullptr);

        const string path = ((Core::File(filename).IsDirectory() == true)? Core::Directory::Normalize(filename) : filename);

        _adminLock.Lock();

        Files::iterator index = _files.find(path);
        ASSERT(index != _files.end());

        if (index != _files.end()) {
            Observers::iterator loop = _observers.find(index->second);
            ASSERT(loop != _observers.end());

            loop->second.Unregister(callback);
            if (loop->second.HasCallbacks() == false) {
                if (inotify_rm_watch(_notifyFd, index->second) < 0) {
                    TRACE_L1(_T("Invoke of inotify_rm_watch failed"));
                }
                // Clear this index, we are no longer observing
                _files.erase(index);
                _observers.erase(loop);
                if (_files.size() == 0) {
                    // This is the first entry, lets start monitoring
                    _adminLock.Unlock();
                    Core::ResourceMonitor::Instance().Unregister(*this);
                }
                else {
                    _adminLock.Unlock();
                }
            }
            else {
                _adminLock.Unlock();
            }
        }
        else
        {
            _adminLock.Unlock();
        }

    }

private:
    Core::IResource::handle Descriptor() const override
    {
        return (_notifyFd);
    }
    uint16_t Events() override
    {
        return (POLLIN);
    }
    void Handle(const uint16_t events) override
    {
        if ((events & POLLIN) != 0) {
            uint8_t eventBuffer[(sizeof(struct inotify_event) + NAME_MAX + 1)];
            int length;
            do
            {
                length = ::read(_notifyFd, eventBuffer, sizeof(eventBuffer));

                if (length >= static_cast<int>(sizeof(struct inotify_event))) {
                    const struct inotify_event *event = reinterpret_cast<const struct inotify_event *>(eventBuffer);

                    auto Evaluate = [this](const struct inotify_event* event) -> bool {
                        // In case of IN_CREATE notify only if the created file is a link
                        if ((event->mask & (IN_CREATE | IN_ISDIR)) == IN_CREATE) {
                            ASSERT(event->len != 0);
                            const int& wd = event->wd;
                            auto const it = std::find_if(_files.cbegin(), _files.cend(), [wd](const std::pair<string, int>& elem) {
                                return (elem.second == wd);
                            });

                            ASSERT(it != _files.cend());
                            return (Core::File((*it).first + Core::ToString(event->name)).IsLink());
                        }
                        return (true);
                    };

                    _adminLock.Lock();

                    if (Evaluate(event) == true) {
                        // Check if we have this entry..
                        Observers::iterator loop = _observers.find(event->wd);
                        if (loop != _observers.end()) {
                            loop->second.Notify();
                        }
                    }

                    _adminLock.Unlock();
                }
            } while (length > 0);
        }
    }

private:
    Core::CriticalSection _adminLock;
    int _notifyFd;
    Files _files;
    Observers _observers;
};

#endif

#ifdef __WINDOWS__

// --------------------------------------------------------------------------------------------
// https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-readdirectorychangesw
// --------------------------------------------------------------------------------------------
class FileSystemMonitor {
public:
    struct ICallback
    {
        virtual ~ICallback() = default;
        virtual void Updated() = 0;
    };

private:
    // The code for windows is derived from the example given here:
    // https://gist.github.com/nickav/a57009d4fcc3b527ed0f5c9cf30618f8
    // If you need to determine the event that really triggered the
    // callback see this example for possible retrieval information (in
    // the overlapped call)
    class Context {
    private:
        using ClientList = std::list<ICallback*>;
    public:
        Context() = delete;
        Context(const Context&) = delete;
        Context& operator= (const Context&) = delete;

        Context(const string& pathName)
            : _adminLock()
            , _filename(pathName)
            , _file(INVALID_HANDLE_VALUE)
            , _clients() {
            _file = CreateFile(pathName.c_str(),
                FILE_LIST_DIRECTORY,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                NULL,
                OPEN_EXISTING,
                FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
                NULL);

            if (_file != INVALID_HANDLE_VALUE) {
                _overlapped.hEvent = CreateEvent(NULL, FALSE, 0, NULL);

                if (ReadDirectoryChangesW(
                    _file, _buffer, sizeof(_buffer), FALSE,
                    FILE_NOTIFY_CHANGE_FILE_NAME |
                    FILE_NOTIFY_CHANGE_DIR_NAME,
                    NULL, &_overlapped, NULL) == FALSE) {
                    TRACE_L1("Could not start observing: %s", _filename.c_str());
                }
            }
        }
        ~Context() {
            if (_file != INVALID_HANDLE_VALUE) {
                ::CloseHandle(_file);
                if (_overlapped.hEvent != INVALID_HANDLE_VALUE) {
                    ::CloseHandle(_overlapped.hEvent);
                }
            }
        }

    public:
        // No need to lock here. This is an internal private class and can only be accessed through the
        // outerclass FileSystemMonitor. This class will take care of the locking on:
        // 1) Register
        // 2) Unregister
        // 3) Handle
        // 4) IsEmpty
        // 5) Notify
        void Register(ICallback* client) {
            ASSERT(std::find(_clients.begin(), _clients.end(), client) == _clients.end());

            _clients.push_back(client);
        }
        void Unregister(ICallback* client) {
            ClientList::iterator index (std::find(_clients.begin(), _clients.end(), client));

            ASSERT (index != _clients.end());

            if (index != _clients.end()) {
                _clients.erase(index);
            }
        }
        HANDLE Handle() {
            return (_overlapped.hEvent);
        }
        bool IsEmpty() const {
            return (_clients.empty());
        }
        void Notify() {
            DWORD bytes_transferred;
            if (GetOverlappedResult(_file, &_overlapped, &bytes_transferred, FALSE) != FALSE) {
                // See: https://gist.github.com/nickav/a57009d4fcc3b527ed0f5c9cf30618f8 for
                // data in the retrieved _buffer with size of &bytes_transferred.
                if (ReadDirectoryChangesW(
                    _file, _buffer, sizeof(_buffer), FALSE,
                    FILE_NOTIFY_CHANGE_FILE_NAME |
                    FILE_NOTIFY_CHANGE_DIR_NAME,
                    NULL, &_overlapped, NULL) == FALSE) {

                    TRACE_L1("Could not start observing: %s", _filename.c_str());
                }

                for (auto& client : _clients) {
                    client->Updated();
                }
            }
        }

    private:
        Core::CriticalSection _adminLock;
        string _filename;
        HANDLE _file;
        std::list<ICallback*> _clients;
        OVERLAPPED _overlapped;
        uint8_t _buffer[64];
    };

    using ObserveMap = std::unordered_map<string, Context>;

    class Dispatcher : public Core::Thread {
    public:
        Dispatcher() = delete;
        Dispatcher(const Dispatcher&) = delete;
        Dispatcher& operator= (const Dispatcher&) = delete;
        Dispatcher(FileSystemMonitor& parent)
            : _parent(parent) {
        }
        ~Dispatcher() override {
        }

    public:
        uint32_t Worker() override {
            _parent.Process();
            return (0);
        }

    private:
        FileSystemMonitor& _parent;
    };

    friend class SingletonType<FileSystemMonitor>;
    FileSystemMonitor()
        : _adminLock()
        , _dispatcher(*this)
        , _observers()
        , _trigger(::CreateEvent(NULL, FALSE, FALSE, NULL)) {
    }

public:
    FileSystemMonitor(const FileSystemMonitor &) = delete;
    FileSystemMonitor &operator=(const FileSystemMonitor &) = delete;

    static FileSystemMonitor& Instance()
    {
        return (SingletonType<FileSystemMonitor>::Instance());
    }
    ~FileSystemMonitor() {
        // Make sure all is stopped before we destruct..
        _dispatcher.Stop();
        Trigger();
        _dispatcher.Wait(Core::Thread::BLOCKED | Core::Thread::STOPPED, Core::infinite);
        ::CloseHandle(_trigger);
    }

public:
    // All access to ObserveMap (_observers) is protected against concurrency from
    // this outer class!!
    bool IsValid() const
    {
        return (_trigger != INVALID_HANDLE_VALUE);
    }
    bool Register(ICallback *callback, const string &filename)
    {
        bool subscribed = false;

        ASSERT(callback != nullptr);

        _adminLock.Lock();

        if (Core::File(filename).IsDirectory() == true) {
            ObserveMap::iterator index = _observers.find(filename);
            if (index == _observers.end()) {
                index = _observers.emplace(std::piecewise_construct,
                    std::forward_as_tuple(filename),
                    std::forward_as_tuple(filename)).first;

                // Are we starting the first observer ?
                if (_observers.size() == 1) {
                    _dispatcher.Run();
                }
                else {
                    // Make sure the wait, if pending, gets triggered..
                    Trigger();
                }
            }

            ASSERT(index != _observers.end());

            index->second.Register(callback);
            subscribed = true;
        }
        _adminLock.Unlock();

        return (subscribed);
    }
    void Unregister(ICallback *callback, const string &filename)
    {
        ASSERT(callback != nullptr);

        _adminLock.Lock();

        ObserveMap::iterator index = _observers.find(filename);

        ASSERT(index != _observers.end());

        if (index != _observers.end()) {
            index->second.Unregister(callback);
            if (index->second.IsEmpty() == true) {
                _observers.erase(index);

                if (_observers.empty() == true) {
                    _dispatcher.Block();
                }

                // Make sure the wait, if pending, gets triggered..
                Trigger();
            }
        }

        _adminLock.Unlock();
    }

private:
    void Trigger() {
        ::SetEvent(_trigger);
    }
    void Process() {

        int count = 1;

        _adminLock.Lock();
        HANDLE* syncEvent = reinterpret_cast<HANDLE*>(ALLOCA((_observers.size() + 1) * sizeof(HANDLE)));
        for (auto& observer : _observers) {
            syncEvent[count++] = observer.second.Handle();
        }
        _adminLock.Unlock();

        syncEvent[0] = _trigger;

        ::WaitForMultipleObjects(count, syncEvent, FALSE, Core::infinite);
        ::ResetEvent(_trigger);

        _adminLock.Lock();
        for (auto& observer : _observers) {
            observer.second.Notify();
        }
        _adminLock.Unlock();
    }

private:
    Core::CriticalSection _adminLock;
    Dispatcher _dispatcher;
    ObserveMap _observers;
    HANDLE _trigger;
};

#endif

} // namespace Core
} // namespace WPEFramework
