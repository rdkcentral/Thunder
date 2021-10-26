#include "MessageDispatcher.h"

namespace WPEFramework {
namespace Core {
    //-----------MESSAGE DISPATCHER-----------
    MessageDispatcher::MessageDispatcher()
        : _reader(nullptr)
        , _writer(nullptr)
        , _dataBuffer(nullptr)
        , _metaDataBuffer(nullptr)
    {
    }

    MessageDispatcher::~MessageDispatcher()
    {
        auto metaDataBufferSize = _metaDataBuffer->Size();
        auto dataBufferSize = _dataBuffer->Size();

        //size reported by buffers is lower by the size of control.
        _mappedFile->SetNumber<decltype(metaDataBufferSize), ENDIAN_BIG>(0, metaDataBufferSize + sizeof(Core::CyclicBuffer::control));
        _mappedFile->SetNumber<decltype(metaDataBufferSize), ENDIAN_BIG>(sizeof(metaDataBufferSize), dataBufferSize + sizeof(Core::CyclicBuffer::control));
    }

    /**
     * @brief Creates a message dispatcher
     * 
     * @param identifier name of the dispatcher
     * @param instanceId number of the instance
     * @param totalSize total size of the buffers (data + metadata)
     * @param percentage how much of totalSize is reserved for metadat (eg. 10 = 10% of totalSize)
     * @return uint32_t Core::ERROR_NONE if buffers are valid, Core::ERROR_UNAVAILABLE otherwise
     */
    uint32_t MessageDispatcher::Create(const string& identifier, const uint32_t instanceId, uint32_t totalSize, uint8_t percentage)
    {
        ASSERT(_dataBuffer == nullptr);
        ASSERT(_metaDataBuffer == nullptr);
        ASSERT(totalSize > 2 * sizeof(Core::CyclicBuffer::control));

        string doorBellName = Core::Format("%s.doorbell", identifier.c_str());
        string fileName = Core::Format("%s.%d.buffer", identifier.c_str(), instanceId);

        uint32_t metadataSize = (totalSize * percentage) / 100;
        uint32_t dataSize = totalSize - metadataSize;

        //first bytes in the file are reserved to store information about buffer sizes
        uint32_t offset = sizeof(metadataSize) + sizeof(dataSize);

        ASSERT(metadataSize > sizeof(Core::CyclicBuffer::control));
        ASSERT(dataSize > sizeof(Core::CyclicBuffer::control));

        // clang-format off
        _mappedFile.reset(new Core::DataElementFile(fileName, Core::File::USER_READ    | 
                                                              Core::File::USER_WRITE   | 
                                                              Core::File::USER_EXECUTE | 
                                                              Core::File::GROUP_READ   |
                                                              Core::File::GROUP_WRITE  |
                                                              Core::File::OTHERS_READ  |
                                                              Core::File::OTHERS_WRITE | 
                                                              Core::File::CREATE | 
                                                              Core::File::SHAREABLE, totalSize + offset));
        // clang-format on
        ASSERT(_mappedFile->IsValid());
        _metaDataBuffer.reset(new MessageBuffer(doorBellName, *_mappedFile, true, offset, metadataSize, false));
        _dataBuffer.reset(new MessageBuffer(doorBellName, *_mappedFile, true, offset + metadataSize, dataSize, true));

        ASSERT(_dataBuffer->IsValid() && _metaDataBuffer->IsValid());
        _reader.reset(new Reader(*this, dataSize - sizeof(Core::CyclicBuffer::control)));
        _writer.reset(new Writer(*this));

        _mappedFile->SetNumber<decltype(metadataSize), ENDIAN_BIG>(0, metadataSize);
        _mappedFile->SetNumber<decltype(dataSize), ENDIAN_BIG>(sizeof(metadataSize), dataSize);

        return (_dataBuffer->IsValid() && _metaDataBuffer->IsValid() ? Core::ERROR_NONE : Core::ERROR_UNAVAILABLE);
    }

    /**
     * @brief Opens previously created message dispatcher
     * 
     * @param identifier unique name 
     * @param instanceId number of instance
     * @return uint32_t Core::ERROR_NONE if buffers are valid, Core::ERROR_UNAVAILABLE otherwise
     */
    uint32_t MessageDispatcher::Open(const string& identifier, const uint32_t instanceId)
    {

        ASSERT(_dataBuffer == nullptr);
        ASSERT(_metaDataBuffer == nullptr);
        string doorBellName = Core::Format("%s.doorbell", identifier.c_str());
        string fileName = Core::Format("%s.%d.buffer", identifier.c_str(), instanceId);

        uint32_t metadataSize = 0;
        uint32_t dataSize = 0;
        uint32_t offset = sizeof(metadataSize) + sizeof(dataSize); //first bytes in the file are reserved to store information about buffer sizes

        Core::File file(fileName);
        // clang-format off
        _mappedFile.reset(new Core::DataElementFile(file, Core::File::USER_READ    | 
                                                          Core::File::USER_WRITE   | 
                                                          Core::File::USER_EXECUTE | 
                                                          Core::File::GROUP_READ   |
                                                          Core::File::GROUP_WRITE  |
                                                          Core::File::OTHERS_READ  |
                                                          Core::File::OTHERS_WRITE | 
                                                          Core::File::SHAREABLE));
        // clang-format on
        ASSERT(_mappedFile->IsValid());

        metadataSize = _mappedFile->GetNumber<decltype(metadataSize), ENDIAN_BIG>(0);
        dataSize = _mappedFile->GetNumber<decltype(dataSize), ENDIAN_BIG>(sizeof(metadataSize));

        _metaDataBuffer.reset(new MessageBuffer(doorBellName, *_mappedFile, true, offset, metadataSize, false));
        _dataBuffer.reset(new MessageBuffer(doorBellName, *_mappedFile, true, offset + metadataSize, dataSize, true));

        ASSERT(_dataBuffer->IsValid() && _metaDataBuffer->IsValid());
        //real size of buffers is lower by sizeof control, so the read buffer should also be smaller
        _reader.reset(new Reader(*this, dataSize - sizeof(Core::CyclicBuffer::control)));
        _writer.reset(new Writer(*this));

        return (_dataBuffer->IsValid() && _metaDataBuffer->IsValid() ? Core::ERROR_NONE : Core::ERROR_UNAVAILABLE);
    }

    MessageDispatcher::Reader& MessageDispatcher::GetReader()
    {
        return *_reader;
    }

    MessageDispatcher::Writer& MessageDispatcher::GetWriter()
    {
        return *_writer;
    }

    //----------READER-----------
    MessageDispatcher::Reader::Reader(MessageDispatcher& parent, uint32_t dataBufferSize)
        : _parent(parent)
    {
        _dataBuffer.resize(dataBufferSize);
    }

    uint32_t MessageDispatcher::Reader::Metadata(uint8_t& outType, uint16_t& outLength, uint8_t* outValue)
    {
        return Core::ERROR_NONE;
    }

    uint32_t MessageDispatcher::Reader::Data(uint8_t& outType, uint16_t& outLength, uint8_t* outValue)
    {
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

    uint32_t MessageDispatcher::Writer::Metadata(const uint8_t type, const uint16_t length, const uint8_t* value)
    {
        //TODO BLOCKING CALL
        return Core::ERROR_NONE;
    }

    uint32_t MessageDispatcher::Writer::Data(const uint8_t type, const uint16_t length, const uint8_t* value)
    {
        ASSERT(length > 0);
        uint32_t result = Core::ERROR_NONE;
        const uint16_t fullLength = sizeof(type) + sizeof(length) + length; // headerLength + informationLength

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