#include "MessageDispatcher.h"

namespace WPEFramework {
namespace Core {
    //-----------MESSAGE DISPATCHER-----------
    MessageDispatcher::MessageDispatcher()
        : _reader(*this)
        , _writer(*this)
        , _outputChannel(nullptr)
    {
    }

    uint32_t MessageDispatcher::Open(const string& doorBell, const string& fileName)
    {
        ASSERT(_outputChannel == nullptr);

        _outputChannel.reset(new MessageBuffer(doorBell, fileName));

        ASSERT(_outputChannel->IsValid() == true);

        return (_outputChannel->IsValid() ? Core::ERROR_NONE : Core::ERROR_UNAVAILABLE);
    }

    uint32_t MessageDispatcher::Close()
    {
        Core::SafeSyncType<Core::CriticalSection> _guard(_lock);

        _outputChannel.reset(nullptr);

        return (Core::ERROR_NONE);
    }

    MessageDispatcher::Reader& MessageDispatcher::GetReader()
    {
        return _reader;
    }

    MessageDispatcher::Writer& MessageDispatcher::GetWriter()
    {
        return _writer;
    }

    //----------READER-----------
    MessageDispatcher::Reader::Reader(MessageDispatcher& parent)
        : _parent(parent)
    {
    }

    uint32_t MessageDispatcher::Reader::Metadata(uint8_t& outType, uint16_t& outLength, uint8_t* outValue)
    {
        _parent._outputChannel->Acknowledge();
        return Core::ERROR_NONE;
    }

    uint32_t MessageDispatcher::Reader::Data(uint8_t& outType, uint16_t& outLength, uint8_t* outValue)
    {
        //TODO - error codes
        uint32_t length = 0;
        bool available = _parent._outputChannel->IsValid();

        if (available == false) {
            available = _parent._outputChannel->Validate();
        }

        if (available) {
            std::cerr << "ADDR: " << _parent._outputChannel.get() << std::endl;
            std::cerr << "BEFORE: " << _parent._outputChannel->Used() << std::endl;
            length = _parent._outputChannel->Read(_buffer, sizeof(_buffer) - 8);
            std::cerr << "AFTER: " << _parent._outputChannel->Used() << std::endl;
            std::cerr << "LENGTH: " << length << std::endl;

            if (length < 3) {
                //did not receive type and length, this is not valid message
                TRACE_L1("Inconsistent message\n");
            } else {
                uint32_t offset = 0;

                ::memcpy(&outType, &(_buffer[offset]), sizeof(outType));
                offset += sizeof(outType);

                ::memcpy(&outLength, &(_buffer[offset]), sizeof(outLength));
                offset += sizeof(outLength);

                ::memcpy(outValue, &(_buffer[offset]), sizeof(uint8_t) * outLength);
            }
        }
        _parent._outputChannel->Acknowledge();

        return Core::ERROR_NONE;
    }

    bool MessageDispatcher::Reader::IsEmpty() const
    {
        return _parent._outputChannel->Used() == 0;
    }

    uint32_t MessageDispatcher::Reader::Wait(const uint32_t waitTime)
    {
        return _parent._outputChannel->Wait(waitTime);
    }

    //------------WRITER-----------
    MessageDispatcher::Writer::Writer(MessageDispatcher& parent)
        : _parent(parent)
    {
    }

    uint32_t MessageDispatcher::Writer::Metadata(const uint8_t type, const uint16_t length, const uint8_t* value)
    {
        //TODO BLOCKING CALL
        _parent._outputChannel->Ring();
        return Core::ERROR_NONE;
    }

    uint32_t MessageDispatcher::Writer::Data(const uint8_t type, const uint16_t length, const uint8_t* value)
    {
        std::cerr << "ADDR: " << _parent._outputChannel.get() << std::endl;

        ASSERT(length > 0);
        const uint16_t fullLength = sizeof(type) + sizeof(length) + length; // headerLength + informationLength

        // Tell the buffer how much we are going to write.
        const uint16_t reservedLength = _parent._outputChannel->Reserve(fullLength);

        if (reservedLength >= fullLength) {
            _parent._outputChannel->Write(reinterpret_cast<const uint8_t*>(&type), sizeof(type)); //type
            _parent._outputChannel->Write(reinterpret_cast<const uint8_t*>(&length), sizeof(length)); //length
            _parent._outputChannel->Write(value, length); //value

        } else {
            TRACE_L1("Buffer to small to fit message!\n");
        }

        _parent._outputChannel->Ring();
        return Core::ERROR_NONE;
    }

    //------------MESSAGE BUFFER-----------
    // clang-format off
    MessageDispatcher::MessageBuffer::MessageBuffer(const string& doorBell, const string& name)
      : Core::CyclicBuffer(name, 
                                Core::File::USER_READ    | 
                                Core::File::USER_WRITE   | 
                                Core::File::USER_EXECUTE | 
                                Core::File::GROUP_READ   |
                                Core::File::GROUP_WRITE  |
                                Core::File::OTHERS_READ  |
                                Core::File::OTHERS_WRITE | 
                                Core::File::SHAREABLE,
                             CyclicBufferSize, true)
        , _doorBell(doorBell.c_str())
    {
        ASSERT (IsValid() == true);
    }
    // clang-format on

    void MessageDispatcher::MessageBuffer::Ring()
    {
        _doorBell.Ring();
    }

    void MessageDispatcher::MessageBuffer::Acknowledge()
    {
        _doorBell.Acknowledge();
    }

    uint32_t MessageDispatcher::MessageBuffer::Wait(const uint32_t waitTime)
    {
        return (_doorBell.Wait(waitTime));
    }

    void MessageDispatcher::MessageBuffer::Relinquish()
    {
        return (_doorBell.Relinquish());
    }

    uint32_t MessageDispatcher::MessageBuffer::GetOverwriteSize(Cursor& cursor)
    {
        //if not on metadata, can flush,
        while (cursor.Offset() < cursor.Size()) {
            uint16_t chunkSize = 0;
            cursor.Peek(chunkSize);

            TRACE_L1("Flushing TRACE data !!! %d", __LINE__);

            cursor.Forward(chunkSize);
        }

        return cursor.Offset();
    }

    void MessageDispatcher::MessageBuffer::DataAvailable()
    {
        _doorBell.Ring();
    }
}
}