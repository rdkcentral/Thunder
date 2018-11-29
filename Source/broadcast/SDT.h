#ifndef DVB_SDT_TABLE_H
#define DVB_SDT_TABLE_H

// ---- Include system wide include files ----

// ---- Include local include files ----
#include "MPEGDescriptor.h"
#include "MPEGSection.h"
#include "Module.h"

// ---- Referenced classes and types ----

// ---- Helper types and constants ----

// ---- Helper functions ----

// ---- Class Definition ----

namespace WPEFramework {
namespace Broadcast {
namespace DVB {

    class EXTERNAL ServiceDescriptor : public  {
    private:
        ServiceDescriptor operator= (const ServiceDescriptor& rhs) = delete;

    public:
        constexpr static uint8_t TAG = 0x48;

        enum type {
            DIGITAL_TELEVISION          = 0x01,
            DIGITAL_RADIO               = 0x02,
            TELETEXT                    = 0x03,
            NVOD_REFERENCE              = 0x04,
            NVOD_TIME_SHIFT             = 0x05,
            MOSAIC                      = 0x06,  
            ADVANCED_DIGITAL_RADIO      = 0x0A,
            ADVANCED_DIGITAL_MOSAIC     = 0x0B,
            DATA_BROADCAST_SERVICE      = 0x0C,
            ADVANCED_SD_TELEVISION      = 0x16,
            ADVANCED_SD_NVOD_TIME_SHIFT = 0x17,
            ADVANCED_SD_NVOD_REFERENCE  = 0x18,
            ADVANCED_HD_TELEVISION      = 0x19,
            ADVANCED_HD_NVOD_TIME_SHIFT = 0x1A,
            ADVANCED_HD_NVOD_REFERENCE  = 0x1B
        };

    public:
        ServiceDescriptor () : _data() {
        }
        ServiceDescriptor (const ServiceDescriptor& copy) : _data(copy) {
        }
        ServiceDescriptor (const MPEG::Descriptor& copy) : _data(copy) {
        }
        ~ServiceDescriptor() {
        }

    public:
        type Type() const {
            return (static_cast<type>(_data[0]));
        }
        string Provider() const {
            return (ToString(reinterpret_cast<const char*>(&(_data[2])), _data[1]));
        }
        string Name() const {
            uint8_t offset = 1 /* service type */ + 1 /* length */ + _data[1];
            return (ToString(reinterpret_cast<const char*>(&(_data[offset + 1])), _data[offset]));
        }

    private:
        MPEG::Descriptor _data;
    };
  
    class EXTERNAL SDT {
    public:
        static const uint16_t ACTUAL = 0x42;
        static const uint16_t OTHER  = 0x46;

    public:
        enum running {
            Undefined     = 0,
            NotRunning    = 1,
            AboutToStart  = 2,
            Pausing       = 3,
            Running       = 4
        };

    public:
        class ServiceIterator {
        public:
            ServiceIterator()
                : _info()
                , _offset(~0)
            {
            }
            ServiceIterator(const Core::DataElement& data)
                : _info(data)
                , _offset(~0)
            {
            }
            ServiceIterator(const ServiceIterator& copy)
                : _info(copy._info)
                , _offset(copy._offset)
            {
            }
            ~ServiceIterator() {}

            ServiceIterator& operator=(const ServiceIterator& RHS)
            {
                _info = RHS._info;
                _offset = RHS._offset;

                return (*this);
            }

        public:
            inline bool IsValid() const { return (_offset < _info.Size()); }
            inline void Reset() { _offset = ~0; }
            inline bool Next()
            {
                if (_offset == static_cast<uint16_t>(~0)) {
                    _offset = 0;
                } else if (_offset < _info.Size()) {
                    _offset += (DescriptorSize() + 5);
                }

                return (IsValid());
            }
            inline bool EIT_PF() const 
            { 
                return ((_info[_offset + 2] & 0x01) != 0); 
            }
            inline bool EIT_Schedule() const 
            { 
                return ((_info[_offset + 2] & 0x02) != 0); 
            }
            inline bool IsRunning() const 
            { 
                return ((_info[_offset + 2] & 0x02) != 0); 
            }
            inline bool IsFreeToAir() const 
            { 
                return ((_info[_offset + 3] & 0x10) != 0); 
            }
            inline running RunningMode() const 
            { 
                return (static_cast<running>((_info[_offset + 3] & 0xE0) >> 5)); 
            }
            inline uint16_t ServiceId() const
            {
                return ((_info[_offset + 0] << 8) | _info[_offset + 1]);
            }
            inline MPEG::DescriptorIterator Descriptors() const
            {
                return (MPEG::DescriptorIterator(
                    Core::DataElement(_info, _offset + 5, DescriptorSize())));
            }
            inline uint8_t Services() const
            {
                uint8_t count = 0;
                uint16_t offset = 0;
                while (offset < _info.Size()) {
                    offset += (((_info[offset + 3] << 8) | _info[offset + 4]) & 0x0FFF) + 5;
                    count++;
                }
                return (count);
            }

        private:
            inline uint16_t DescriptorSize() const
            {
                return ((_info[_offset + 3] << 8) | _info[_offset + 4]) & 0x0FFF;
            }

        private:
            Core::DataElement _info;
            uint16_t _offset;
        };

    public:
        SDT()
            : _data()
            , _transportStreamId(~0)
        {
        }
        SDT(const MPEG::Table& data)
            : _data(data.Data())
            , _transportStreamId(data.Extension())
        {
        }
        SDT(const uint16_t transportStreamId, const Core::DataElement& data)
            : _data(data)
            , _transportStreamId(transportStreamId)
        {
        }
        SDT(const SDT& copy)
            : _data(copy._data)
            , _transportStreamId(copy._transportStreamId)
        {
        }
        ~SDT() {}

        SDT& operator=(const SDT& rhs)
        {
            _data = rhs._data;
            _transportStreamId = rhs._transportStreamId;
            return (*this);
        }
        bool operator==(const SDT& rhs) const
        {
            return ((_transportStreamId == rhs._transportStreamId) && (_data == rhs._data));
        }
        bool operator!=(const SDT& rhs) const { return (!operator==(rhs)); }

    public:
        inline bool IsValid() const
        {
            return ((_transportStreamId != static_cast<uint16_t>(~0)) && (_data.Size() >= 2));
        }
        inline uint16_t TransportStreamId() const { return (_transportStreamId); }
        uint16_t OriginalNetworkId() const
        {
            return (_data.GetNumber<uint16_t, Core::ENDIAN_BIG>(0) & 0x1FFF);
        }
        ServiceIterator Services() const
        {
            return (ServiceIterator(Core::DataElement(_data, 3, _data.Size() - 3)));
        }

    private:
        Core::DataElement _data;
        uint16_t _transportStreamId;
    };

} // namespace DVB 
} // namespace Broadcast
} // namespace WPEFramework

#endif // DVB_SDT_TABLE_H

