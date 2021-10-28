#include "MessageDispatcher.h"

namespace WPEFramework {
namespace Core {
    //-----------MESSAGE DISPATCHER-----------
    
    /**
     * @brief Creates a message dispatcher
     * 
     * @param identifier name of the dispatcher
     * @param instanceId number of the instance
     * @param totalSize total size of the buffers (data + metadata)
     * @param percentage how much of totalSize is reserved for metadat (eg. 10 = 10% of totalSize)
     * @return MessageDispatcher
     */
    MessageDispatcher MessageDispatcher::Create(const string& identifier, const uint32_t instanceId, uint32_t totalSize, uint8_t percentage)
    {
        string doorBellFilename = Core::Format("%s.doorbell", identifier.c_str());
        string fileName = Core::Format("%s.%d.buffer", identifier.c_str(), instanceId);

        uint32_t metaDataSize = (totalSize * percentage) / 100;
        uint32_t dataSize = totalSize - metaDataSize;

        //first bytes in the file are reserved to store information about buffer sizes
        uint32_t offset = sizeof(metaDataSize) + sizeof(dataSize);

        ASSERT(metaDataSize > sizeof(Core::CyclicBuffer::control));
        ASSERT(dataSize > sizeof(Core::CyclicBuffer::control));

        return { doorBellFilename, fileName, metaDataSize, dataSize, offset };
    }

    /**
     * @brief Opens previously created message dispatcher
     * 
     * @param identifier unique name 
     * @param instanceId number of instance
     * @return MessageDispatcher
     */
    MessageDispatcher MessageDispatcher::Open(const string& identifier, const uint32_t instanceId)
    {
        string doorBellFilename = Core::Format("%s.doorbell", identifier.c_str());
        string fileName = Core::Format("%s.%d.buffer", identifier.c_str(), instanceId);

        Core::File file(fileName);
        // clang-format off
        Core::DataElementFile mappedFile = Core::DataElementFile(file, Core::File::USER_READ    | 
                                                                       Core::File::USER_WRITE   | 
                                                                       Core::File::USER_EXECUTE | 
                                                                       Core::File::GROUP_READ   |
                                                                       Core::File::GROUP_WRITE  |
                                                                       Core::File::OTHERS_READ  |
                                                                       Core::File::OTHERS_WRITE | 
                                                                       Core::File::SHAREABLE);
        // clang-format on
        ASSERT(mappedFile.IsValid());

        uint32_t metaDataSize = mappedFile.GetNumber<decltype(metaDataSize), ENDIAN_BIG>(0);
        uint32_t dataSize = mappedFile.GetNumber<decltype(dataSize), ENDIAN_BIG>(sizeof(metaDataSize));
        ASSERT(metaDataSize > sizeof(Core::CyclicBuffer::control));
        ASSERT(dataSize > sizeof(Core::CyclicBuffer::control));

        uint32_t offset = sizeof(metaDataSize) + sizeof(dataSize);

        return { doorBellFilename, std::move(mappedFile), metaDataSize, dataSize, offset };
    }

    MessageDispatcher::MessageDispatcher(const string& doorBellFilename, const string& fileName, uint32_t metaDataSize, uint32_t dataSize, uint32_t offset)
        // clang-format off
        :  _mappedFile(fileName, Core::File::CREATE       |
                                 Core::File::USER_READ    | 
                                 Core::File::USER_WRITE   | 
                                 Core::File::USER_EXECUTE | 
                                 Core::File::GROUP_READ   |
                                 Core::File::GROUP_WRITE  |
                                 Core::File::OTHERS_READ  |
                                 Core::File::OTHERS_WRITE | 
                                 Core::File::SHAREABLE, offset + metaDataSize + dataSize)
        // clang-format on
        , _metaDataBuffer(new MessageBuffer(doorBellFilename, _mappedFile, true, offset, metaDataSize, false))
        , _dataBuffer(new MessageBuffer(doorBellFilename, _mappedFile, true, offset + metaDataSize, dataSize, true))
        , _reader(*this, dataSize - sizeof(Core::CyclicBuffer::control))
        , _writer((*this))
    {

        ASSERT(metaDataSize > sizeof(Core::CyclicBuffer::control));
        ASSERT(dataSize > sizeof(Core::CyclicBuffer::control));

        ASSERT(_mappedFile.IsValid());

        ASSERT(_dataBuffer->IsValid());
        ASSERT(_metaDataBuffer->IsValid());

        _mappedFile.SetNumber<decltype(metaDataSize), ENDIAN_BIG>(0, metaDataSize);
        _mappedFile.SetNumber<decltype(dataSize), ENDIAN_BIG>(sizeof(metaDataSize), dataSize);
    }

    MessageDispatcher::MessageDispatcher(const string& doorBellFilename, Core::DataElementFile&& mappedFile, uint32_t metaDataSize, uint32_t dataSize, uint32_t offset)
        : _mappedFile(std::move(mappedFile))
        , _metaDataBuffer(new MessageBuffer(doorBellFilename, _mappedFile, false, offset, metaDataSize, false))
        , _dataBuffer(new MessageBuffer(doorBellFilename, _mappedFile, false, offset + metaDataSize, dataSize, true))
        , _reader(*this, dataSize - sizeof(Core::CyclicBuffer::control))
        , _writer(*this)
    {
        ASSERT(_dataBuffer != nullptr);
        ASSERT(_metaDataBuffer != nullptr);

        ASSERT(metaDataSize > sizeof(Core::CyclicBuffer::control));
        ASSERT(dataSize > sizeof(Core::CyclicBuffer::control));

        ASSERT(_mappedFile.IsValid());

        ASSERT(_dataBuffer->IsValid());
        ASSERT(_metaDataBuffer->IsValid());
    }

    /**
     * @brief On destroy, write sizes of each buffer, so a next Open will know the needed size for each buffer
     * 
     */
    MessageDispatcher::~MessageDispatcher()
    {
        auto metaDataBufferSize = _metaDataBuffer->Size();
        auto dataBufferSize = _dataBuffer->Size();

        //size reported by buffers is lower by the size of control.
        _mappedFile.SetNumber<decltype(metaDataBufferSize), ENDIAN_BIG>(0, metaDataBufferSize + sizeof(Core::CyclicBuffer::control));
        _mappedFile.SetNumber<decltype(metaDataBufferSize), ENDIAN_BIG>(sizeof(metaDataBufferSize), dataBufferSize + sizeof(Core::CyclicBuffer::control));
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
    MessageDispatcher::Reader::Reader(MessageDispatcher& parent, uint32_t dataBufferSize)
        : _parent(parent)
    {
        _dataBuffer.resize(dataBufferSize);
    }

    MessageDispatcher::Reader::Reader(Reader&& other)
        : _parent(other._parent)
        , _dataBuffer(other._dataBuffer)

    {
    }

    uint32_t MessageDispatcher::Reader::Metadata(uint8_t& outType, uint16_t& outLength, uint8_t* outValue)
    {
        return Core::ERROR_NONE;
    }

    uint32_t MessageDispatcher::Reader::Data(uint8_t& outType, uint16_t& outLength, uint8_t* outValue)
    {
        _parent._lock.Lock();

        ASSERT(_parent._mappedFile.IsValid());
        uint32_t result = Core::ERROR_NONE;
        bool available = _parent._dataBuffer->IsValid();

        if (available == false) {
            available = _parent._dataBuffer->Validate();
        }

        if (available) {
            uint32_t length = _parent._dataBuffer->Read(_dataBuffer.data(), _dataBuffer.size());
            if (length < 3) {
                //did not receive type and length, this is not valid message
                TRACE_L1("Inconsistent message\n");
                _parent._dataBuffer->Flush();
                result = Core::ERROR_READ_ERROR;

            } else {
                uint32_t offset = 0;

                ::memcpy(&outLength, &(_dataBuffer[offset]), sizeof(outLength));
                offset += sizeof(outLength);

                ::memcpy(&outType, &(_dataBuffer[offset]), sizeof(outType));
                offset += sizeof(outType);

                outLength -= offset; //fullLength - ( length of type + length of message)
                ::memcpy(outValue, &(_dataBuffer[offset]), sizeof(uint8_t) * outLength);
            }
        }


        _parent._lock.Unlock();

        return result;
    }

    bool MessageDispatcher::Reader::IsEmpty() const
    {
        return _parent._dataBuffer->Used();
    }

    uint32_t MessageDispatcher::Reader::Wait(const uint32_t waitTime)
    {
        return _parent._dataBuffer->Wait(waitTime);
    }

    //------------WRITER-----------
    MessageDispatcher::Writer::Writer(MessageDispatcher& parent)
        : _parent(parent)
    {
    }

    MessageDispatcher::Writer::Writer(Writer&& other)
        : _parent(other._parent)

    {
    }

    uint32_t MessageDispatcher::Writer::Metadata(const uint8_t type, const uint16_t length, const uint8_t* value)
    {
        //TODO BLOCKING CALL
        return Core::ERROR_NONE;
    }

    uint32_t MessageDispatcher::Writer::Data(const uint8_t type, const uint16_t length, const uint8_t* value)
    {
        _parent._lock.Lock();

        ASSERT(length > 0);
        uint32_t result = Core::ERROR_NONE;
        const uint16_t fullLength = sizeof(type) + sizeof(length) +  length; // headerLength + informationLength

        // Tell the buffer how much we are going to write.
        const uint16_t reservedLength = _parent._dataBuffer->Reserve(fullLength);

        if (reservedLength >= fullLength) {
            _parent._dataBuffer->Write(reinterpret_cast<const uint8_t*>(&fullLength), sizeof(fullLength)); //fullLength
            _parent._dataBuffer->Write(reinterpret_cast<const uint8_t*>(&type), sizeof(type)); //type
            _parent._dataBuffer->Write(value, length); //value

        } else {
            result = Core::ERROR_WRITE_ERROR;
            TRACE_L1("Buffer to small to fit message!\n");
        }

        _parent._lock.Unlock();

        return result;
    }

    void MessageDispatcher::Writer::Ring()
    {
        _parent._dataBuffer->Ring();
    }

    //------------MESSAGE BUFFER-----------
    MessageDispatcher::MessageBuffer::MessageBuffer(const string& doorBell,
        Core::DataElementFile& buffer,
        const bool initiator,
        const uint32_t offset,
        const uint32_t bufferSize,
        const bool overwrite)
        : Core::CyclicBuffer(buffer, initiator, offset, bufferSize, overwrite)
        , _doorBell(doorBell.c_str())
    {
        ASSERT(IsValid() == true);
    }

    void MessageDispatcher::MessageBuffer::Ring()
    {
        _doorBell.Ring();
    }

    uint32_t MessageDispatcher::MessageBuffer::Wait(const uint32_t waitTime)
    {
        auto result = _doorBell.Wait(waitTime);
        if (result != Core::ERROR_TIMEDOUT) {
            _doorBell.Acknowledge();
        }
        return result;
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

            TRACE_L1("Flushing buffer data !!! %d", __LINE__);

            cursor.Forward(chunkSize);
        }

        return cursor.Offset();
    }

    uint32_t MessageDispatcher::MessageBuffer::GetReadSize(Cursor& cursor)
    {
        // Just read one entry.
        uint16_t entrySize = 0;
        cursor.Peek(entrySize);
        return entrySize;
    }
}
}