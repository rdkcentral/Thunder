#ifndef __LOCKABLECONTAINER_H
#define __LOCKABLECONTAINER_H

#include "Module.h"

#include "ReadWriteLock.h"

namespace WPEFramework {
namespace Core {

    template <typename CONTAINER>
    class EXTERNAL LockableContainerType : public CONTAINER {
    private:
        enum ContainerState {
            READERS,
            WRITER,
            IDLE
        };

    public:
        LockableContainerType()
            : CONTAINER()
            , m_RWLock()
        {
        }
        LockableContainerType(const LockableContainerType<CONTAINER>& copy)
            : CONTAINER(copy)
            , m_RWLock()
        {
        }
        ~LockableContainerType()
        {
        }

        LockableContainerType<CONTAINER> operator=(const LockableContainerType<CONTAINER>& rhs)
        {
            // Make sure we can write in this container!!!
            m_RWLock.WriteLock();

            CONTAINER::operator=(rhs);

            // Make sure we can write in this container!!!
            m_RWLock.WriteUnlock();

            return (*this);
        }

    public:
        bool ReadLock(const uint32_t waitTime = Core::infinite) const
        {
            return (m_RWLock.ReadLock(waitTime));
        }
        void ReadUnlock() const
        {
            m_RWLock.ReadUnlock();
        }
        bool WriteLock(const uint32_t waitTime = Core::infinite) const
        {
            return (m_RWLock.WriteLock(waitTime));
        }
        inline void WriteUnlock() const
        {
            m_RWLock.WriteUnlock();
        }

    private:
        mutable ReadWriteLock m_RWLock;
    };
}
} // namespace Core

#endif // __LOCKABLECONTAINER_H
