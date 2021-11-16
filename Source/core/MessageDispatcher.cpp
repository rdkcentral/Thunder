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
        , _dataBuffer(new DataBuffer(doorBellFilename, _mappedFile, true, sizeof(dataSize), dataSize, true))
        , _metaDataBuffer(new MetaDataBuffer(metaDataFilename))
        // clang-format on
        , _reader(*this, dataSize - sizeof(dataSize) - sizeof(Core::CyclicBuffer::control))
        , _writer((*this))
        , _metaDataFilename(metaDataFilename)
    {
        ASSERT(dataSize > sizeof(Core::CyclicBuffer::control));
        ASSERT(_mappedFile.IsValid());
        ASSERT(_dataBuffer != nullptr);
        ASSERT(_dataBuffer->IsValid());

        if (_mappedFile.IsValid()) {
            if (_dataBuffer == nullptr) {
                TRACE_L1(_T("DataBuffer failed to be created"));
            } else if (!_dataBuffer->IsValid()) {
                TRACE_L1(_T("DataBuffer is invalid"));
            }

            if (dataSize > sizeof(Core::CyclicBuffer::control)) {
                _mappedFile.SetNumber<decltype(dataSize), ENDIAN_BIG>(0, dataSize);
            } else {
                TRACE_L1(_T("Invalid DataBuffer size"));
            }

        } else {
            TRACE_L1(_T("Memory mapped file not valid"));
        }
    }

    MessageDispatcher::MessageDispatcher(const string& doorBellFilename, Core::DataElementFile&& mappedFile, const string& metaDataFilename, uint32_t dataSize)
        : _mappedFile(std::move(mappedFile))
        , _dataBuffer(new DataBuffer(doorBellFilename, _mappedFile, false, sizeof(dataSize), dataSize, true))
        , _metaDataBuffer(nullptr) //do not need a server on a wrting side
        , _reader(*this, dataSize - sizeof(Core::CyclicBuffer::control))
        , _writer(*this)
        , _metaDataFilename(metaDataFilename)
    {
        ASSERT(dataSize > sizeof(Core::CyclicBuffer::control));
        ASSERT(_mappedFile.IsValid());
        ASSERT(_dataBuffer != nullptr);
        ASSERT(_dataBuffer->IsValid());

        if (_mappedFile.IsValid()) {
            if (_dataBuffer == nullptr) {
                TRACE_L1(_T("DataBuffer failed to be created"));
            } else if (!_dataBuffer->IsValid()) {
                TRACE_L1(_T("DataBuffer is invalid"));
            }

            if (dataSize < sizeof(Core::CyclicBuffer::control)) {
                TRACE_L1(_T("Invalid DataBuffer size, is the data buffer file deleted?"));
            }

        } else {
            TRACE_L1(_T("Memory mapped file not valid"));
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
                Packet::Deserialize(_dataBuffer.data(), outType, outLength, outValue);

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
    uint32_t MessageDispatcher::Writer::Metadata(const uint8_t type, const uint16_t length, const uint8_t* value)
    {
        ASSERT(length > 0);
        uint32_t result = Core::ERROR_GENERAL;

        Core::IPCChannelClientType<Core::Void, false, true> channel(Core::NodeId(_parent._metaDataFilename.c_str()), Core::MessageDispatcher::MetaDataBuffer::MetaDataBufferSize);
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
    uint32_t MessageDispatcher::Writer::Data(const uint8_t type, const uint16_t length, const uint8_t* value)
    {
        ASSERT(length > 0);
        uint32_t result = Core::ERROR_WRITE_ERROR;
        const uint16_t fullLength = sizeof(type) + sizeof(length) + length; // headerLength + informationLength

        const uint16_t reservedLength = _parent._dataBuffer->Reserve(fullLength);

        if (reservedLength >= fullLength) {
            //no need to serialize because we can write to CyclicBuffer step by step
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
}
}