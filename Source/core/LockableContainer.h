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
 
#ifndef __LOCKABLECONTAINER_H
#define __LOCKABLECONTAINER_H

#include "Module.h"

#include "ReadWriteLock.h"

namespace Thunder {
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
        LockableContainerType(LockableContainerType<CONTAINER>&& move)
            : CONTAINER(move)
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

        LockableContainerType<CONTAINER> operator=(LockableContainerType<CONTAINER>&& move)
        {
            if (this != &move) {
                // Make sure we can write in this container!!!
                m_RWLock.WriteLock();

                CONTAINER::operator=(move);

                // Make sure we can write in this container!!!
                m_RWLock.WriteUnlock();
            }
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
