#include "MessageDispatcher.h"

namespace WPEFramework {
namespace Core {
    namespace {
        //only if multiple if power of 2
        int RoundUp(int numToRound, int multiple)
        {
            return (numToRound + multiple - 1) & -multiple;
        }
    }

    //-----------PACKET-----------

    /**
     * @brief Construct a new Message Dispatcher:: Packet:: Packet object
     * 
     * @param type type of message
     * @param length length of value buffer
     * @param value buffer
     */
    MessageDispatcher::Packet::Packet(const uint8_t type, const uint16_t length, const uint8_t* value)
        : _type(type)
    {
        _buffer.resize(length);
        std::copy(value, value + length, _buffer.begin());
    }

    /**
     * @brief Construct a new Message Dispatcher:: Packet:: Packet object.
     *        Passed buffer  will be deserialized and written to member variables.
     *        Serialized buffer should be following pattern as specified in @ref Serialize.
     * 
     * @param fullLength buffer length
     * @param buffer buffer (serialized)
     */
    MessageDispatcher::Packet::Packet(const uint16_t fullLength, const uint8_t* buffer)
    {
        //create a buffer to store deserialized value (reusing _buffer here)
        _buffer.resize(fullLength);
        uint16_t actualLength = 0;

        //fill in type, length and value
        Deserialize(buffer, _type, actualLength, _buffer.data());

        //after _buffer is filled with value, resize it to the size of actualLength which is the size of deserialized value
        _buffer.resize(actualLength);
        _buffer.shrink_to_fit();
    }

    /**
     * @brief Write member variables into buffer. Pattern is:
     *       - uint16_t - full length of message
     *       - uint8_t - type of message
     *       - uint8_t* - buffer
     * 
     * @return std::vector<uint8_t> serialized buffer
     */
    std::vector<uint8_t> MessageDispatcher::Packet::Serialize()
    {
        std::vector<uint8_t> result;
        uint16_t bufferLength = _buffer.size();
        uint16_t fullLength = sizeof(bufferLength) + sizeof(_type) + _buffer.size();
        result.resize(fullLength);

        uint32_t offset = 0;
        memcpy(result.data(), &fullLength, sizeof(fullLength));
        offset += sizeof(fullLength);

        memcpy(result.data() + offset, &_type, sizeof(_type));
        offset += sizeof(_type);

        memcpy(result.data() + offset, _buffer.data(), bufferLength);
        offset += bufferLength;

        //should be fast enough due to NRVO
        return result;
    }

    /**
     * @brief Deserialize buffer and get its contents. Serialized buffer should be following pattern as specified in @ref Serialize.
     * 
     * @param buffer serialized buffer
     * @param outType type of message
     * @param outLength length of message
     * @param outValue message buffer
     */
    void MessageDispatcher::Packet::Deserialize(const uint8_t* buffer, uint8_t& outType, uint16_t& outLength, uint8_t* outValue)
    {
        uint32_t offset = 0;

        ::memcpy(&outLength, &(buffer[offset]), sizeof(outLength));
        offset += sizeof(outLength);

        ::memcpy(&outType, &(buffer[offset]), sizeof(_type));
        offset += sizeof(_type);

        outLength -= offset; //fullLength - ( length of type + length of message)
        ::memcpy(outValue, &(buffer[offset]), outLength);
    }

    uint8_t MessageDispatcher::Packet::Type() const
    {
        return _type;
    }

    uint32_t MessageDispatcher::Packet::Length() const
    {
        return _buffer.size();
    }

    const uint8_t* MessageDispatcher::Packet::Value() const
    {
        return _buffer.data();
    }

    //-----------DATA BUFFER-----------
    MessageDispatcher::DataBuffer::DataBuffer(const string& doorBell,
        Core::DataElementFile& buffer,
        const bool initiator,
        const uint32_t offset,
        const uint32_t bufferSize,
        const bool overwrite)
        : Core::CyclicBuffer(buffer, initiator, offset, bufferSize, overwrite)
        , _doorBell(doorBell.c_str())
    {
        ASSERT(IsValid());
    }

    /**
     * @brief Signal that data is available
     * 
     */
    void MessageDispatcher::DataBuffer::Ring()
    {
        _doorBell.Ring();
    }

    /**
     * @brief Wait for the doorbell and acknowledge if rang in given time
     * 
     * @param waitTime how much should we wait for the doorbell
     * @return uint32_t ERROR_UNAVAILABLE: doorbell is not connected to its counterpart
     *                  ERROR_TIMEDOUT: ring not rang in given time
     *                  ERROR_NONE: OK
     */
    uint32_t MessageDispatcher::DataBuffer::Wait(const uint32_t waitTime)
    {
        auto result = _doorBell.Wait(waitTime);
        if (result != Core::ERROR_TIMEDOUT) {
            _doorBell.Acknowledge();
        }
        return result;
    }

    /**
     * @brief Unbind doorbell from its counterpart
     * 
     */
    void MessageDispatcher::DataBuffer::Relinquish()
    {
        return (_doorBell.Relinquish());
    }

    //CyclicBuffer specific overrides
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

    //-----------META DATA BUFFER-----------

    MessageDispatcher::MetaDataBuffer::MetaDataBuffer(const std::string& binding)
        : BaseClass(Core::NodeId(binding.c_str()), MetaDataBufferSize)
    {
        CreateFactory<MetaDataFrame>(1);
        Register(MetaDataFrame::Id(), Core::ProxyType<Core::IIPCServer>(Core::ProxyType<MetaDataFrameHandler>::Create(this)));
        Open(Core::infinite);
    }

    MessageDispatcher::MetaDataBuffer::~MetaDataBuffer()
    {
        Close(Core::infinite);
        Unregister(MetaDataFrame::Id());
        DestroyFactory<MetaDataFrame>();
    }

    void MessageDispatcher::MetaDataBuffer::RegisterMetaDataCallback(MetaDataCallback notification)
    {
        _notification = notification;
    }
    void MessageDispatcher::MetaDataBuffer::UnregisterMetaDataCallback()
    {
        _notification = nullptr;
    }

    MessageDispatcher::MetaDataBuffer::MetaDataFrameHandler::MetaDataFrameHandler(MetaDataBuffer* parent)
        : _parent(*parent)
    {
    }

    void MessageDispatcher::MetaDataBuffer::MetaDataFrameHandler::Procedure(Core::IPCChannel& source, Core::ProxyType<Core::IIPC>& data)
    {
        auto message = Core::ProxyType<MetaDataFrame>(data);

        auto length = message->Parameters().Length();
        auto value = message->Parameters().Value();

        Packet packet(length, value);
        if (_parent._notification != nullptr) {
            _parent._notification(packet.Type(), packet.Length(), packet.Value());
            message->Response() = Core::ERROR_NONE;

        } else {
            message->Response() = Core::ERROR_UNAVAILABLE;
        }

        source.ReportResponse(data);
    }

    //-----------MESSAGE DISPATCHER-----------    
    /**
     * @brief Creates a message dispatcher
     * 
     * @param identifier name of the dispatcher
     * @param instanceId number of the instance
     * @param dataSize size of data buffer
     * @return MessageDispatcher 
     */
    MessageDispatcher MessageDispatcher::Create(const string& identifier, const uint32_t instanceId, const uint32_t dataSize)
    {
        string directory = _T("/tmp/MessageDispatcher");
        auto filenames = PrepareFilenames(directory, identifier, instanceId);

        if (Core::File(directory).IsDirectory()) {
            //if directory exists remove it to clear data (eg. sockets) that can remain after previous creation
            Core::Directory(directory.c_str()).Destroy(false);
        }

        if (!Core::Directory(directory.c_str()).CreatePath()) {
            TRACE_L1(_T("Unable to create MessageDispatcher directory"));
        }

        return { std::get<0>(filenames), std::get<1>(filenames), std::get<2>(filenames), dataSize };
    }

    /**
     * @brief Prepare filenames for MessageDispatcher filenames
     * 
     * @param baseDirectory where are those filed stored
     * @param identifier identifer of the instance
     * @param instanceId number of instance
     * @return std::tuple<string, string, string> 
     *         0 - doorBellFilename
     *         1 - dataFileName
     *         2 - metaDataFilename
     */
    std::tuple<string, string, string> MessageDispatcher::PrepareFilenames(const string& baseDirectory, const string& identifier, const uint32_t instanceId)
    {

        string doorBellFilename = Core::Format("%s/%s.doorbell", baseDirectory.c_str(), identifier.c_str());
        string dataFilename = Core::Format("%s/%s.%d.data", baseDirectory.c_str(), identifier.c_str(), instanceId);
        string metaDataFilename = Core::Format("%s/%s.%d.metadata", baseDirectory.c_str(), identifier.c_str(), instanceId);

        return std::make_tuple(doorBellFilename, dataFilename, metaDataFilename);
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
        string directory = _T("/tmp/MessageDispatcher");
        auto filenames = PrepareFilenames(directory, identifier, instanceId);

        Core::File file(std::get<1>(filenames));
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

        uint32_t dataSize = mappedFile.GetNumber<decltype(dataSize), ENDIAN_BIG>(0);

        return { std::get<0>(filenames), std::move(mappedFile), std::get<2>(filenames), dataSize };
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
        , _dataBuffer(new DataBuffer(doorBellFilename, _mappedFile, true, sizeof(dataSize), 0, true))
        , _metaDataBuffer(new MetaDataBuffer(metaDataFilename))
        // clang-format on
        , _metaDataFilename(metaDataFilename)
    {
        ASSERT(dataSize > sizeof(Core::CyclicBuffer::control));
        ASSERT(_mappedFile.IsValid());
        ASSERT(_dataBuffer != nullptr);
        ASSERT(_dataBuffer->IsValid());

        if (!IsValid()) {
            TRACE_L1("MessageDispatcher is not valid!");
        } else {
            _dataReadBuffer.resize(dataSize - RoundUp(sizeof(dataSize), sizeof(void*)) - sizeof(Core::CyclicBuffer::control));
            _mappedFile.SetNumber<decltype(dataSize), ENDIAN_BIG>(0, dataSize);
        }
    }

    MessageDispatcher::MessageDispatcher(const string& doorBellFilename, Core::DataElementFile&& mappedFile, const string& metaDataFilename, uint32_t dataSize)
        : _mappedFile(std::move(mappedFile))
        , _dataBuffer(new DataBuffer(doorBellFilename, _mappedFile, false, sizeof(dataSize), 0, true))
        , _metaDataBuffer(nullptr) //do not need a server on a wrting side
        , _metaDataFilename(metaDataFilename)
    {
        ASSERT(dataSize > sizeof(Core::CyclicBuffer::control));
        ASSERT(_mappedFile.IsValid());
        ASSERT(_dataBuffer != nullptr);
        ASSERT(_dataBuffer->IsValid());

        if (!IsValid()) {
            TRACE_L1("MessageDispatcher is not valid!");
        } else {
            //actual read buffer is: data - aligned offset - sizeof administration
            _dataReadBuffer.resize(dataSize - RoundUp(sizeof(dataSize), sizeof(void*)) - sizeof(Core::CyclicBuffer::control));
        }
    }

    /**
     * @brief On destroy, write sizes of each buffer, so a next Open will know the needed size for data buffer
     * 
     */
    MessageDispatcher::~MessageDispatcher()
    {
        _dataBuffer->Relinquish();

        auto dataBufferSize = _dataBuffer->Size();
        _mappedFile.SetNumber<decltype(dataBufferSize), ENDIAN_BIG>(0, dataBufferSize);
    }

    MessageDispatcher::MessageDispatcher(MessageDispatcher&& other)
        : _dataLock()
        , _metaDataLock()
        , _mappedFile(std::move(other._mappedFile))
        , _dataBuffer(std::move(other._dataBuffer))
        , _metaDataBuffer(std::move(other._metaDataBuffer))
    {
    }

    uint32_t MessageDispatcher::PopData(uint8_t& outType, uint16_t& outLength, uint8_t* outValue)
    {
        _dataLock.Lock();

        ASSERT(_mappedFile.IsValid());
        uint32_t result = Core::ERROR_READ_ERROR;

        bool available = _dataBuffer->IsValid();

        if (available == false) {
            available = _dataBuffer->Validate();
        }

        if (available) {
            uint32_t length = _dataBuffer->Read(_dataReadBuffer.data(), _dataReadBuffer.size());
            if (length < 3) {
                //did not receive type and length, this is not valid message
                TRACE_L1("Inconsistent message\n");
                _dataBuffer->Flush();

            } else {
                Packet::Deserialize(_dataReadBuffer.data(), outType, outLength, outValue);

                result = Core::ERROR_NONE;
            }
        }

        _dataLock.Unlock();

        return result;
    }

    /**
     * @brief Writes metadata. Reader needs to register for notifications to recevie this message
     * 
     * @param type type of message
     * @param length length of message
     * @param value vbuffer
     * @return uint32_t ERROR_GENERAL: unable to open communication channel
     *                  ERROR_WRITE_ERROR: unable to write
     *                  ERROR_UNAVAILABLE: message was sent but not reported (missing Register call on the other side)
     *                                     caller should send this message again
     *                  ERROR_NONE: OK
    
     */
    uint32_t MessageDispatcher::PushMetadata(const uint8_t type, const uint16_t length, const uint8_t* value)
    {
        _metaDataLock.Lock();

        ASSERT(length > 0);

        uint32_t result = Core::ERROR_GENERAL;

        Core::IPCChannelClientType<Core::Void, false, true> channel(Core::NodeId(_metaDataFilename.c_str()), Core::MessageDispatcher::MetaDataBuffer::MetaDataBufferSize);
        auto metaDataFrame = Core::ProxyType<Core::MessageDispatcher::MetaDataBuffer::MetaDataFrame>::Create();

        if (channel.Open(Core::infinite) == Core::ERROR_NONE) {
            Packet packet(type, length, value);
            auto serialized = packet.Serialize();
            metaDataFrame->Parameters().Set(serialized.size(), serialized.data());

            if (channel.Invoke(metaDataFrame, Core::infinite) == Core::ERROR_NONE) {
                result = metaDataFrame->Response();
            } else {
                result = Core::ERROR_WRITE_ERROR;
            }

            channel.Close(Core::infinite);
        }

        _metaDataLock.Unlock();

        return result;
    }

    /**
     * @brief Writes data into cyclic buffer. If it does not fit the data already in the cyclic buffer will be flushed.
     *        After writing everything, this side should call Ring() to notify other side.
     *        To receive this data other side needs to wait for the doorbel ring and then use Reader::Data
     *
     * @param type type of message
     * @param length length of message
     * @param value buffer 
     * @return uint32_t ERROR_WRITE_ERROR: failed to reserve enough space - eg, value size is exceeding max cyclic buffer size
     *                  ERROR_NONE: OK
     */
    uint32_t MessageDispatcher::PushData(const uint8_t type, const uint16_t length, const uint8_t* value)
    {
        _dataLock.Lock();

        ASSERT(length > 0);
        uint32_t result = Core::ERROR_WRITE_ERROR;
        const uint16_t fullLength = sizeof(type) + sizeof(length) + length; // headerLength + informationLength

        const uint16_t reservedLength = _dataBuffer->Reserve(fullLength);

        if (reservedLength >= fullLength) {
            //no need to serialize because we can write to CyclicBuffer step by step
            _dataBuffer->Write(reinterpret_cast<const uint8_t*>(&fullLength), sizeof(fullLength)); //fullLength
            _dataBuffer->Write(reinterpret_cast<const uint8_t*>(&type), sizeof(type)); //type
            _dataBuffer->Write(value, length); //value
            result = Core::ERROR_NONE;

        } else {
            TRACE_L1("Buffer to small to fit message!\n");
        }

        _dataLock.Unlock();

        return result;
    }

    void MessageDispatcher::Ring()
    {
        _dataBuffer->Ring();
    }

    uint32_t MessageDispatcher::Wait(const uint32_t waitTime)
    {
        return _dataBuffer->Wait(waitTime);
    }

    void MessageDispatcher::RegisterDataAvailable(MetaDataCallback notification)
    {
        if (_metaDataBuffer->IsOpen()) {
            _metaDataBuffer->RegisterMetaDataCallback(notification);
        }
    }
    void MessageDispatcher::UnregisterDataAvailable()
    {
        _metaDataBuffer->UnregisterMetaDataCallback();
    }

    bool MessageDispatcher::IsValid() const
    {
        bool result = _mappedFile.IsValid();

        if (result) {

            if (_dataBuffer == nullptr) {
                result = false;
            } else {

                if (!_dataBuffer->IsValid()) {
                    result = false;
                }
                if (_metaDataBuffer != nullptr) {
                    if (!_metaDataBuffer->IsOpen()) {
                        result = false;
                    }
                }
            }
        }
        return result;
    }

    uint32_t MessageDispatcher::DataSize() const
    {
        return _dataReadBuffer.size();
    }

    uint32_t MessageDispatcher::MetaDataSize() const
    {
        return MessageDispatcher::MetaDataBuffer::MetaDataBufferSize;
    }

}
}