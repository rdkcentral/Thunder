#include "ProcessContainer.h"
#include <stdint.h>
namespace WPEFramework {
namespace ProcessContainers {

    NetworkInterfaceIterator::NetworkInterfaceIterator() 
        : _current(UINT32_MAX)
        , _count(0)
        , _refCount(1)
    {
        
    }

    void NetworkInterfaceIterator::AddRef()
    {
        _refCount++;
    }

    void NetworkInterfaceIterator::Release()
    {
        if (--_refCount == 0) {
            delete this;
        }
    }

    bool NetworkInterfaceIterator::Next() 
    {
        if (_current == UINT32_MAX)
            _current = 0;
        else
            ++_current;

        return IsValid();
    }

    void NetworkInterfaceIterator::Reset() 
    {
        _current = UINT32_MAX;
    }

    bool NetworkInterfaceIterator::IsValid() const
    {
        return (_current < _count) && (_current != UINT32_MAX);
    }

    uint32_t NetworkInterfaceIterator::Count() const
    {
        return _count;
    }

    IContainer* IContainerAdministrator::Find(const string& id)
    {
        auto iterator = Containers();
        IContainer* result = nullptr;

        while(iterator.Next() == true) {
            if (iterator.Current()->Id() == id) {
                result = &(*iterator.Current());
                break;
            }
        }

        return result;
    }

}
}