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
    MessageDispatcher MessageDispatcher::Create(const string& identifier, const uint32_t instanceId, const uint32_t dataSize)
    {
        string doorBellFilename = Core::Format("%s.doorbell", identifier.c_str());
        string dataFilename = Core::Format("%s.%d.data", identifier.c_str(), instanceId);
        string metaDataFilename = Core::Format("%s.%d.metadata", identifier.c_str(), instanceId);

        return { doorBellFilename, dataFilename, metaDataFilename, dataSize };
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
        string dataFilename = Core::Format("%s.%d.data", identifier.c_str(), instanceId);
        string metaDataFilename = Core::Format("%s.%d.metadata", identifier.c_str(), instanceId);

        Core::File file(dataFilename);
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

        uint32_t dataSize = mappedFile.GetNumber<decltype(dataSize), ENDIAN_BIG>(0);

        ASSERT(dataSize > sizeof(Core::CyclicBuffer::control));

        return { doorBellFilename, std::move(mappedFile), metaDataFilename, dataSize };
    }

    MessageDispatcher::MessageDispatcher(const string& doorBellFilename, const string& dataFileName, const string& metaDataFilename, uint32_t dataSize)
        // clang-format off
        :  _mappedFile(dataFileName, Core::File::CREATE       |
                                     Core::File::USER_READ    | 
                                     Core::File::USER_WRITE   | 
                                     Core::File::USER_EXECUTE | 
                                     Core::File::GROUP_READ   |
                                     Core::File::GROUP_WRITE  |
                                     Core::File::OTHERS_READ  |
                                     Core::File::OTHERS_WRITE | 
                                     Core::File::SHAREABLE, sizeof(dataSize) + dataSize )
        , _dataBuffer(new DataBuffer(doorBellFilename, _mappedFile, true, sizeof(dataSize), dataSize, true))
        , _metaDataBuffer(new MetaDataBuffer(metaDataFilename))
        // clang-format on
        , _reader(*this, dataSize - sizeof(dataSize) - sizeof(Core::CyclicBuffer::control))
        , _writer((*this))
        , _metaDataFilename(metaDataFilename)
    {

        ASSERT(dataSize > sizeof(Core::CyclicBuffer::control));

        ASSERT(_mappedFile.IsValid());

        ASSERT(_dataBuffer->IsValid());

        _mappedFile.SetNumber<decltype(dataSize), ENDIAN_BIG>(0, dataSize);
    }

    MessageDispatcher::MessageDispatcher(const string& doorBellFilename, Core::DataElementFile&& mappedFile, const string& metaDataFilename, uint32_t dataSize)
        : _mappedFile(std::move(mappedFile))
        , _dataBuffer(new DataBuffer(doorBellFilename, _mappedFile, false, sizeof(dataSize), dataSize, true))
        , _metaDataBuffer(nullptr) //do not need a server on a wrting side
        , _reader(*this, dataSize - sizeof(Core::CyclicBuffer::control))
        , _writer(*this)
        , _metaDataFilename(metaDataFilename)
    {
        ASSERT(_mappedFile.IsValid());

        ASSERT(_dataBuffer != nullptr);
        ASSERT(_dataBuffer->IsValid());
    }

    /**
     * @brief On destroy, write sizes of each buffer, so a next Open will know the needed size for each buffer
     * 
     */
    MessageDispatcher::~MessageDispatcher()
    {
        auto dataBufferSize = _dataBuffer->Size();

        //size reported by buffers is lower by the size of control.
        _mappedFile.SetNumber<decltype(dataBufferSize), ENDIAN_BIG>(0, dataBufferSize);
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

    uint32_t MessageDispatcher::Reader::Data(uint8_t& outType, uint16_t& outLength, uint8_t* outValue)
    {
        ASSERT(_parent._mappedFile.IsValid());
        uint32_t result = Core::ERROR_READ_ERROR;
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

            } else {
                uint32_t offset = 0;

                ::memcpy(&outLength, &(_dataBuffer[offset]), sizeof(outLength));
                offset += sizeof(outLength);

                ::memcpy(&outType, &(_dataBuffer[offset]), sizeof(outType));
                offset += sizeof(outType);

                outLength -= offset; //fullLength - ( length of type + length of message)
                ::memcpy(outValue, &(_dataBuffer[offset]), outLength);

                result = Core::ERROR_NONE;
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

    MessageDispatcher::Writer::Writer(Writer&& other)
        : _parent(other._parent)

    {
    }

    uint32_t MessageDispatcher::Writer::Metadata(const uint8_t type, const uint16_t length, const uint8_t* value)
    {
        std::cerr << "METADATA" << std::endl;
        Core::IPCChannelClientType<Core::Void, false, true> channel(Core::NodeId(_parent._metaDataFilename.c_str()), Core::MessageDispatcher::MetaDataBuffer::MetaDataBufferSize);
        auto metaDataFrame = Core::ProxyType<Core::MessageDispatcher::MetaDataBuffer::MetaDataFrame>::Create();

        if (channel.Open(1000) == Core::ERROR_NONE) {

            std::cerr << "OPENED" << std::endl;

            Packet packet(type, length, value);
            auto serialized = packet.Serialize();
            metaDataFrame->Parameters().Set(serialized.size(), serialized.data());

            Core::ProxyType<Core::IIPC> message(metaDataFrame);

            auto result = channel.Invoke(message, Core::infinite);

            if (result == Core::ERROR_NONE) {
                auto response = metaDataFrame->Response();
                std::cerr << "RESPONSE: " << response.Value() << std::endl;
            } else {
                std::cerr << "UNABLE TO INVOKE IN GIVEN TIME" << std::endl;
            }

            std::cerr << "ABOUT TO CLOSE" << std::endl;
            channel.Close(1000);
        }

        std::cerr << "METADATA END" << std::endl;
        return Core::ERROR_NONE;
    }

    uint32_t MessageDispatcher::Writer::Data(const uint8_t type, const uint16_t length, const uint8_t* value)
    {
        ASSERT(length > 0);
        uint32_t result = Core::ERROR_WRITE_ERROR;
        const uint16_t fullLength = sizeof(type) + sizeof(length) + length; // headerLength + informationLength

        // Tell the buffer how much we are going to write.
        const uint16_t reservedLength = _parent._dataBuffer->Reserve(fullLength);

        if (reservedLength >= fullLength) {
            _parent._dataBuffer->Write(reinterpret_cast<const uint8_t*>(&fullLength), sizeof(fullLength)); //fullLength
            _parent._dataBuffer->Write(reinterpret_cast<const uint8_t*>(&type), sizeof(type)); //type
            _parent._dataBuffer->Write(value, length); //value
            result = Core::ERROR_NONE;

        } else {
            TRACE_L1("Buffer to small to fit message!\n");
        }

        return result;
    }

    void MessageDispatcher::Writer::Ring()
    {
        _parent._dataBuffer->Ring();
    }

    //------------MESSAGE BUFFER-----------
    MessageDispatcher::DataBuffer::DataBuffer(const string& doorBell,
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

    void MessageDispatcher::DataBuffer::Ring()
    {
        _doorBell.Ring();
    }

    uint32_t MessageDispatcher::DataBuffer::Wait(const uint32_t waitTime)
    {
        auto result = _doorBell.Wait(waitTime);
        if (result != Core::ERROR_TIMEDOUT) {
            _doorBell.Acknowledge();
        }
        return result;
    }

    void MessageDispatcher::DataBuffer::Relinquish()
    {
        return (_doorBell.Relinquish());
    }

    uint32_t MessageDispatcher::DataBuffer::GetOverwriteSize(Cursor& cursor)
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

    uint32_t MessageDispatcher::DataBuffer::GetReadSize(Cursor& cursor)
    {
        // Just read one entry.
        uint16_t entrySize = 0;
        cursor.Peek(entrySize);
        return entrySize;
    }
}
}