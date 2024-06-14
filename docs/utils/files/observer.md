The file observer can be used to monitor files/directories and invoke code when a change occurs. On linux systems, it uses inotify to receive filesystem events.

## Example

To get started with a simple FileObserver implementation, create a class that inherits from `Core::FileSystemMonitor::ICallback`. The class registers with the `FileSystemMonitor` instance upon construction, and unregisters upon destruction.

This class should implement the `Updated()` method from the `ICallback` interface which is called whenever the observed file is changed.

```c++
#include <core/core.h>

using namespace Thunder;

class FileChangeMonitor : Core::FileSystemMonitor::ICallback {
public:
    FileChangeMonitor(string filePath)
        : _filePath(std::move(filePath))
        , _registered(false)
    {
        // Check the path is valid
        Core::File file(_filePath);
        if (!file.Exists()) {
            printf("Error: Path %s does not exist - cannot add monitor\n", _filePath.c_str());
        } else {
            // Register the file path with the filesystem observer
            if (Core::FileSystemMonitor::Instance().Register(this, _filePath)) {
                _registered = true;
                printf("Successfully installed file monitor for %s\n", _filePath.c_str());
            } else {
                printf("Failed to install filesystem monitor\n");
            }
        }
    }

    ~FileChangeMonitor() override
    {
        if (_registered) {
            Core::FileSystemMonitor::Instance().Unregister(this, _filePath);
        }
    }

    // Delete copy/move ctors
    FileChangeMonitor(FileChangeMonitor&&) = delete;
    FileChangeMonitor(const FileChangeMonitor&) = delete;
    FileChangeMonitor& operator=(const FileChangeMonitor&) = delete;

    void Updated() override
    {
        printf("This file %s has changed!\n", _filePath.c_str());
    }

private:
    const string _filePath;
    bool _registered;
};
```

We can now construct an instance of `FileChangeMonitor` to observe a file or directory of our choosing

!!! warning
	If watching a directory, be aware the file observer is not recursive, so will only monitor the top-level for changes. Add seperate observers for child directories if required

```c++
// Monitor a specific file (note this is based on inodes so cannot withstand the file being deleted & recreated)
FileChangeMonitor fileMonitor("/tmp/test.txt");

// Monitor an entire directory
FileChangeMonitor fileMonitor("/tmp");
```

Below is a `main()` method that installs the file monitor and sleeps for 20 seconds. 

Whilst the app is running, modifying the `/tmp/test.txt` file will cause our callback to fire and the message to be printed to the console

```c++
int main(int argc, char const* argv[])
{
    {
        FileChangeMonitor fileMonitor("/tmp/test.txt");
        SleepS(20);
    }

    // Always remember to call Core::Singleton::Dispose() at the end of the
    // application after destructing any created objects
    Core::Singleton::Dispose();
    return 0;
}

/* Output:
Successfully installed file monitor for /tmp/test.txt
This file /tmp/test.txt has changed!
*/
```

