#ifndef __MPEGDESCRIPTORS_H
#define __MPEGDESCRIPTORS_H

// ---- Include system wide include files ----

// ---- Include local include files ----
#include "Module.h"

// ---- Referenced classes and types ----

// ---- Helper types and constants ----

// ---- Helper functions ----

// ---- Class Definition ----

namespace WPEFramework {
namespace Broadcast {
namespace MPEG {
    class EXTERNAL Descriptor {
    public:
        inline Descriptor()
            : _descriptor()
        {
        }
        inline Descriptor(const Core::DataElement& data)
            : _descriptor(data)
        {
        }
        inline Descriptor(const Descriptor& copy)
            : _descriptor(copy._descriptor)
        {
        }
        inline ~Descriptor() {}

        inline Descriptor& operator=(const Descriptor& RHS)
        {
            _descriptor = RHS._descriptor;

            return (*this);
        }

    public:
        inline uint8_t Tag() const { return (_descriptor[0]); }
        inline uint8_t Length() const { return (_descriptor[1] + 2); }

    private:
        Core::DataElement _descriptor;
    };

    class EXTERNAL DescriptorIterator {
    public:
        inline DescriptorIterator()
            : _descriptors()
            , _index(NUMBER_MAX_UNSIGNED(uint32_t))
        {
        }
        inline DescriptorIterator(const Core::DataElement& data)
            : _descriptors(data)
            , _index(NUMBER_MAX_UNSIGNED(uint32_t))
        {
        }
        inline DescriptorIterator(const DescriptorIterator& copy)
            : _descriptors(copy._descriptors)
            , _index(copy._index)
        {
        }
        inline ~DescriptorIterator() {}

        inline DescriptorIterator& operator=(const DescriptorIterator& rhs)
        {
            _descriptors = rhs._descriptors;
            _index = rhs._index;

            return (*this);
        }

    public:
        inline bool IsValid() const { return (_index < _descriptors.Size()); }
        inline void Reset() { _index = NUMBER_MAX_UNSIGNED(uint32_t); }
        bool Next()
        {
            if (_index == NUMBER_MAX_UNSIGNED(uint32_t)) {
                _index = 0;
            } else if (_index < _descriptors.Size()) {
                _index += Descriptor(Core::DataElement(_descriptors, _index)).Length();
            }

            // See if we have a valid descriptor, Does it fit the block we have ?
            if ((_index + Descriptor(Core::DataElement(_descriptors, _index)).Length()) > _descriptors.Size()) {
                // It's too big, Jump to the end..
                _index = static_cast<uint32_t>(_descriptors.Size());
            }

            return (IsValid());
        }
        inline Descriptor Current()
        {
            return (Descriptor(Core::DataElement(_descriptors, _index)));
        }
        inline const Descriptor Current() const
        {
            return (Descriptor(Core::DataElement(_descriptors, _index)));
        }

        uint32_t Count() const
        {
            uint32_t count = 0;
            uint32_t offset = 0;
            while (offset < _descriptors.Size()) {
                count++;
                offset += Descriptor(Core::DataElement(_descriptors, offset)).Length();
            }

            if (offset > _descriptors.Size()) {
                // reduce the count by one, the last one is toooooooo big
                count--;
            }

            return (count);
        }

    private:
        Core::DataElement _descriptors;
        uint32_t _index;
    };

} // namespace MPEG
} // namespace Broadcast 
} // namespace WPEFramework

#endif //__MPEGDESCRIPTORS_H
