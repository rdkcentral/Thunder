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
    virtual ~FileSystemMonitor()
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

        _adminLock.Lock();

        Files::iterator index = _files.find(filename);
        if (index != _files.end()) {
            Observers::iterator loop = _observers.find(index->second);
            ASSERT(loop != _observers.end());

            loop->second.Register(callback);
        }
        else
        {
            int fileFd = inotify_add_watch(_notifyFd, filename.c_str(), IN_CLOSE_WRITE);
            if (fileFd >= 0) {
                _files.emplace(std::piecewise_construct,
                    std::forward_as_tuple(filename),
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

        _adminLock.Lock();

        Files::iterator index = _files.find(filename);
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
                if (length > 0) {
                    const struct inotify_event *event = reinterpret_cast<const struct inotify_event *>(eventBuffer);

                    _adminLock.Lock();

                    // Check if we have this entry..
                    Observers::iterator loop = _observers.find(event->wd);
                    if (loop != _observers.end()) {
                        loop->second.Notify();
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
class FileSystemMonitor : public Core::IResource {
public:
    struct ICallback
    {
        virtual ~ICallback() = default;
        virtual void Updated() = 0;
    };

private:
    class Worker : public Core::Thread {
    public:
        Worker() = delete;
        Worker(const Worker&) = delete;
        Worker& operator= (const Worker&) = delete;
        Worker(FileSystemMonitor& parent) 
            : _parent(parent) {
            _syncEvent = ::CreateEvent(nullptr, true, false, nullptr);
        }
        ~Worker() override {
        }

    public:
        HANDLE Event() {
            return(_syncEvent);
        }
        uint32_t Worker() override {
        
            return (Core::infinite);
        }

    private:
        FileSystemMonitor& _parent; 
        HANDLE _syncEvent;
    };

    typedef std::unordered_map<int, string> Observers;
    typedef std::unordered_map<string, int> Directories;

    FileSystemMonitor()
        : _adminLock()
        , _directories()
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
    virtual ~FileSystemMonitor() = default;

public:
    bool IsValid() const
    {
        return (_notifyFd != -1);
    }
    bool Register(ICallback *callback, const string &filename)
    {
        ASSERT(callback != nullptr);

        _adminLock.Lock();

        if (Core::File(fileName).IsDirectory() == true) {
        }
        else {
        }
        _adminLock.Unlock();

        return (IsValid());
    }
    void Unregister(ICallback *callback, const string &filename)
    {
        ASSERT(_notifyFd != -1);
        ASSERT(callback != nullptr);

        _adminLock.Lock();

        Files::iterator index = _files.find(filename);
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
                    Core::ResourceMonitor::Instance().Unregister(*this);
                }
            }
        }

        _adminLock.Unlock();
    }
private:
    Observers   _observers;
    Directories _directories;
};

#endif

} // namespace Core
} // namespace WPEFramework
