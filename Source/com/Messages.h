#ifndef __COM_MESSAGES_H
#define __COM_MESSAGES_H

#include "Module.h"

namespace WPEFramework {
namespace RPC {

    namespace Data {
        static const uint16_t IPC_BLOCK_SIZE = 512;

        class Frame : public Core::FrameType<IPC_BLOCK_SIZE> {
        private:
            Frame(Frame&);
            Frame& operator=(const Frame&);

        public:
            Frame()
            {
            }
            ~Frame()
            {
            }

        public:
            friend class Input;
            friend class Output;
            friend class ObjectInterface;

            uint16_t Serialize(const uint16_t offset, uint8_t stream[], const uint16_t maxLength) const
            {
                uint16_t copiedBytes((Size() - offset) > maxLength ? maxLength : (Size() - offset));

                ::memcpy(stream, &(operator[](offset)), copiedBytes);

                return (copiedBytes);
            }
            uint16_t Deserialize(const uint16_t offset, const uint8_t stream[], const uint16_t maxLength)
            {
                Size(offset + maxLength);

                ::memcpy(&(operator[](offset)), stream, maxLength);

                return (maxLength);
            }
        };

        class Input {
        private:
            Input(const Input&);
            Input& operator=(const Input&);

        public:
            Input()
                : _data()
            {
            }
            ~Input()
            {
            }

        public:
            inline void Clear()
            {
                _data.Clear();
            }
            void Set(void* implementation, const uint32_t interfaceId, const uint8_t methodId)
            {
                uint16_t result = _data.SetNumber<void*>(0, implementation);
                result += _data.SetNumber<uint32_t>(result, interfaceId);
                _data.SetNumber(result, methodId);
            }
            template <typename TYPENAME>
            TYPENAME* Implementation()
            {
                void* result = nullptr;

                _data.GetNumber<void*>(0, result);

                return (static_cast<TYPENAME*>(result));
            }
            uint32_t InterfaceId() const
            {
                uint32_t result = 0;

                _data.GetNumber<uint32_t>(sizeof(void*), result);

                return (result);
            }
            uint8_t MethodId() const
            {
                uint8_t result = 0;

                _data.GetNumber(sizeof(void*) + sizeof(uint32_t), result);

                return (result);
            }
            uint32_t Length() const
            {
                return (_data.Size());
            }
            inline Frame::Writer Writer()
            {
                return (Frame::Writer(_data, (sizeof(void*) + sizeof(uint32_t) + sizeof(uint8_t))));
            }
            inline const Frame::Reader Reader() const
            {
                return (Frame::Reader(_data, (sizeof(void*) + sizeof(uint32_t) + sizeof(uint8_t))));
            }
            uint16_t Serialize(uint8_t stream[], const uint16_t maxLength, const uint32_t offset) const
            {
                return (_data.Serialize(static_cast<uint16_t>(offset), stream, maxLength));
            }
            uint16_t Deserialize(const uint8_t stream[], const uint16_t maxLength, const uint32_t offset)
            {
                return (_data.Deserialize(static_cast<uint16_t>(offset), stream, maxLength));
            }

        private:
            Frame _data;
        };

        class Output {
        private:
            Output(const Output&);
            Output& operator=(const Output&);

        public:
            Output()
                : _data()
            {
            }
            ~Output()
            {
            }

        public:
            inline void Clear()
            {
                _data.Clear();
            }
            inline Frame::Writer Writer()
            {
                return (Frame::Writer(_data, 0));
            }
            inline const Frame::Reader Reader() const
            {
                return (Frame::Reader(_data, 0));
            }
            inline uint32_t Length() const
            {
                return (static_cast<uint32_t>(_data.Size()));
            }
            inline uint16_t Serialize(uint8_t stream[], const uint16_t maxLength, const uint32_t offset) const
            {
                return (_data.Serialize(static_cast<uint16_t>(offset), stream, maxLength));
            }
            inline uint16_t Deserialize(const uint8_t stream[], const uint16_t maxLength, const uint32_t offset)
            {
                return (_data.Deserialize(static_cast<uint16_t>(offset), stream, maxLength));
            }

        private:
            Frame _data;
        };

        class Init {
        private:
            Init(const Init&);
            Init& operator=(const Init&);

        public:
            Init()
                : _implementation(nullptr)
                , _interfaceId(~0)
                , _exchangeId(~0)
            {
            }
            Init(Core::IUnknown* implementation, const uint32_t interfaceId, const uint32_t exchangeId)
                : _implementation(implementation)
                , _interfaceId(interfaceId)
                , _exchangeId(exchangeId)
            {
            }
            ~Init()
            {
            }

        public:
            void Set(const uint32_t exchangeId, const uint32_t interfaceId, void* implementation)
            {
                _implementation = implementation;
                _interfaceId = interfaceId;
                _exchangeId = exchangeId;
            }
            void* Implementation()
            {
                return (_implementation);
            }
            uint32_t InterfaceId() const
            {
                return (_interfaceId);
            }
            uint32_t ExchangeId() const
            {
                return (_exchangeId);
            }

        private:
            void* _implementation;
            uint32_t _interfaceId;
            uint32_t _exchangeId;
        };

        class ObjectInterface {
        private:
            ObjectInterface(const ObjectInterface&);
            ObjectInterface& operator=(const ObjectInterface&);

        public:
            ObjectInterface()
                : _data()
            {
            }
            ~ObjectInterface()
            {
            }

        public:
            inline void Clear()
            {
                _data.Clear();
            }
            void Set(const string& className, const uint32_t versionId, const uint32_t interfaceId)
            {
                _data.SetNumber<uint32_t>(0, versionId);
                _data.SetNumber<uint32_t>(4, interfaceId);
                _data.SetText(8, className);
            }
            string ClassName() const
            {
                string className;

                _data.GetText(8, className);

                return (className);
            }
            uint32_t InterfaceId() const
            {
                uint32_t interfaceId;

                _data.GetNumber<uint32_t>(4, interfaceId);

                return (interfaceId);
            }
            uint32_t VersionId() const
            {
                uint32_t versionId;

                _data.GetNumber<uint32_t>(0, versionId);

                return (versionId);
            }
            uint32_t Length() const
            {
                return (_data.Size());
            }
            inline Frame::Writer Writer()
            {
                return (Frame::Writer(_data, (sizeof(void*) + sizeof(uint32_t) + sizeof(uint8_t))));
            }
            inline const Frame::Reader Reader() const
            {
                return (Frame::Reader(_data, (sizeof(void*) + sizeof(uint32_t) + sizeof(uint8_t))));
            }
            uint16_t Serialize(uint8_t stream[], const uint16_t maxLength, const uint32_t offset) const
            {
                return (_data.Serialize(static_cast<uint16_t>(offset), stream, maxLength));
            }
            uint16_t Deserialize(const uint8_t stream[], const uint16_t maxLength, const uint32_t offset)
            {
                return (_data.Deserialize(static_cast<uint16_t>(offset), stream, maxLength));
            }

        private:
            Frame _data;
        };
    }

    typedef Core::IPCMessageType<1, Data::Init, Core::IPC::ScalarType<string> > AnnounceMessage;
    typedef Core::IPCMessageType<2, Data::Input, Data::Output> InvokeMessage;
    typedef Core::IPCMessageType<3, Data::ObjectInterface, Core::IPC::ScalarType<void*> > ObjectMessage;
}
}

#endif // __COM_MESSAGES_H
