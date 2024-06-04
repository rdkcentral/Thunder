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
 
#ifndef __STREAMTEXT_
#define __STREAMTEXT_

#include "Module.h"
#include "Portability.h"

namespace Thunder {
namespace Core {
    class TerminatorNull {
    public:
        TerminatorNull(TerminatorNull&) = delete;
        TerminatorNull(TerminatorNull&&) = delete;
        TerminatorNull& operator=(TerminatorNull&&) = delete;
        TerminatorNull& operator=(const TerminatorNull&) = delete;

    public:
        inline TerminatorNull()
        {
        }
        inline ~TerminatorNull()
        {
        }

    public:
        // 0x80 -> Termination complete. Text Terminated
        // 0x40 -> Termination sequence in Progress.
        // 0x03 -> Number of characters to delete
        inline uint8_t IsTerminated(const TCHAR character) const
        {
            return (character == '\0' ? 0x80 | 0x00 : 0x00);
        }
        inline uint8_t SizeOf() const
        {
            return (1);
        }
        inline const TCHAR* Marker() const
        {
            return (_T("\0"));
        }
    };

    class TerminatorCarriageReturn {
    public:
        TerminatorCarriageReturn(TerminatorCarriageReturn&) = delete;
        TerminatorCarriageReturn(TerminatorCarriageReturn&&) = delete;
        TerminatorCarriageReturn& operator=(TerminatorCarriageReturn&&) = delete;
        TerminatorCarriageReturn& operator=(const TerminatorCarriageReturn&) = delete;

    public:
        inline TerminatorCarriageReturn()
        {
        }
        inline ~TerminatorCarriageReturn()
        {
        }

    public:
        // 0x80 -> Termination complete. Text Terminated
        // 0x40 -> Termination sequence in Progress.
        // 0x03 -> Number of characters to delete
        inline uint8_t IsTerminated(const TCHAR character) const
        {
            return (character == '\n' ? 0x80 | 0x00 : 0x00);
        }
        inline uint8_t SizeOf() const
        {
            return (1);
        }
        inline const TCHAR* Marker() const
        {
            return (_T("\n"));
        }
    };

    class TerminatorCarriageReturnLineFeed {
    public:
        TerminatorCarriageReturnLineFeed(TerminatorCarriageReturnLineFeed&) = delete;
        TerminatorCarriageReturnLineFeed(TerminatorCarriageReturnLineFeed&&) = delete;
        TerminatorCarriageReturnLineFeed& operator=(TerminatorCarriageReturnLineFeed&&) = delete;
        TerminatorCarriageReturnLineFeed& operator=(const TerminatorCarriageReturnLineFeed&) = delete;

    public:
        inline TerminatorCarriageReturnLineFeed()
            : _triggered(0)
        {
        }
        inline ~TerminatorCarriageReturnLineFeed()
        {
        }

    public:
        // 0x80 -> Termination complete. Text Terminated
        // 0x40 -> Termination sequence in Progress.
        // 0x03 -> Number of characters to delete
        inline uint8_t IsTerminated(const TCHAR character) const
        {
            // After the /n we are expecting a /r. If the character after
            // the /n is not a /r we still assume a clossure and thus a -1.
            if ((_triggered & 0x03) == 0x00) {
                _triggered = ((character == '\n' ? 0x01 : 0x00) | (character == '\r' ? 0x02 : 0x00));

                return ((_triggered & 0x03) != 0 ? 0x40 | 0x00 : 0x00);
            }

            _triggered |= ((character == '\n' ? 0x04 : 0x00) | (character == '\r' ? 0x08 : 0x00));

            if ((_triggered == 0x09) || (_triggered == 0x06)) {
                _triggered = 0;
                return (0x80 | 0x00 | 0x01);
            }
            if ((_triggered == 0x10) || (_triggered == 0x20)) {
                _triggered = 0;
                return (0x00);
            }

            _triggered = (_triggered & 0x03);
            return (0x00 | 0x40 | 0x00);
        }
        inline uint8_t SizeOf() const
        {
            return (2);
        }
        inline const TCHAR* Marker() const
        {
            return (_T("\n\r"));
        }

    private:
        mutable uint8_t _triggered;
    };

    template <typename SOURCE, typename TEXTTERMINATOR>
    class StreamTextType {
    private:
        template <typename PARENTCLASS, typename ACTUALSOURCE>
        class HandlerType : public ACTUALSOURCE {
        public:
            HandlerType() = delete;
            HandlerType(HandlerType<PARENTCLASS, ACTUALSOURCE>&&) = delete;
            HandlerType(const HandlerType<PARENTCLASS, ACTUALSOURCE>&) = delete;
            HandlerType<PARENTCLASS, ACTUALSOURCE>& operator=(HandlerType<PARENTCLASS, ACTUALSOURCE>&&) = delete;
            HandlerType<PARENTCLASS, ACTUALSOURCE>& operator=(const HandlerType<PARENTCLASS, ACTUALSOURCE>&) = delete;

        public:
            HandlerType(PARENTCLASS& parent)
                : ACTUALSOURCE()
                , _parent(parent)
            {
            }
            template <typename Arg1>
            HandlerType(PARENTCLASS& parent, Arg1 arg1)
                : ACTUALSOURCE(arg1)
                , _parent(parent)
            {
            }
            template <typename Arg1, typename Arg2>
            HandlerType(PARENTCLASS& parent, Arg1 arg1, Arg2 arg2)
                : ACTUALSOURCE(arg1, arg2)
                , _parent(parent)
            {
            }
            template <typename Arg1, typename Arg2, typename Arg3>
            HandlerType(PARENTCLASS& parent, Arg1 arg1, Arg2 arg2, Arg3 arg3)
                : ACTUALSOURCE(arg1, arg2, arg3)
                , _parent(parent)
            {
            }
            template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
            HandlerType(PARENTCLASS& parent, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4)
                : ACTUALSOURCE(arg1, arg2, arg3, arg4)
                , _parent(parent)
            {
            }
            template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
            HandlerType(PARENTCLASS& parent, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5)
                : ACTUALSOURCE(arg1, arg2, arg3, arg4, arg5)
                , _parent(parent)
            {
            }
            template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6>
            HandlerType(PARENTCLASS& parent, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6)
                : ACTUALSOURCE(arg1, arg2, arg3, arg4, arg5, arg6)
                , _parent(parent)
            {
            }
            template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7>
            HandlerType(PARENTCLASS& parent, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6, Arg7 arg7)
                : ACTUALSOURCE(arg1, arg2, arg3, arg4, arg5, arg6, arg7)
                , _parent(parent)
            {
            }
            template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7, typename Arg8>
            HandlerType(PARENTCLASS& parent, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6, Arg7 arg7, Arg8 arg8)
                : ACTUALSOURCE(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)
                , _parent(parent)
            {
            }
            template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7, typename Arg8, typename Arg9>
            HandlerType(PARENTCLASS& parent, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6, Arg7 arg7, Arg8 arg8, Arg9 arg9)
                : ACTUALSOURCE(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9)
                , _parent(parent)
            {
            }
            template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7, typename Arg8, typename Arg9, typename Arg10>
            HandlerType(PARENTCLASS& parent, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6, Arg7 arg7, Arg8 arg8, Arg9 arg9, Arg10 arg10)
                : ACTUALSOURCE(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10)
                , _parent(parent)
            {
            }
            template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7, typename Arg8, typename Arg9, typename Arg10, typename Arg11>
            HandlerType(PARENTCLASS& parent, Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6, Arg7 arg7, Arg8 arg8, Arg9 arg9, Arg10 arg10, Arg11 arg11)
                : ACTUALSOURCE(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11)
                , _parent(parent)
            {
            }

            ~HandlerType()
            {
            }

        public:
            // Methods to extract and insert data into the socket buffers
            virtual uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize)
            {
                return (_parent.SendData(dataFrame, maxSendSize));
            }

            virtual uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize)
            {
                return (_parent.ReceiveData(dataFrame, receivedSize));
            }

            // Signal a state change, Opened, Closed or Accepted
            virtual void StateChange()
            {
                _parent.StateChange();
            }
            virtual bool IsIdle() const
            {
                return (_parent.IsIdle());
            }

        private:
            PARENTCLASS& _parent;
        };

        typedef StreamTextType<SOURCE, TEXTTERMINATOR> BaseClass;

    public:
        StreamTextType(StreamTextType<SOURCE, TEXTTERMINATOR>&&) = delete;
        StreamTextType(const StreamTextType<SOURCE, TEXTTERMINATOR>&) = delete;
        StreamTextType<SOURCE, TEXTTERMINATOR>& operator=(StreamTextType<SOURCE, TEXTTERMINATOR>&&) = delete;
        StreamTextType<SOURCE, TEXTTERMINATOR>& operator=(const StreamTextType<SOURCE, TEXTTERMINATOR>&) = delete;

    public:
PUSH_WARNING(DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST)
        StreamTextType()
            : _channel(*this)
            , _adminLock()
            , _sendQueue()
            , _offset(0)
            , _receiver()
            , _terminator()
        {
        }
        template <typename Arg1>
        StreamTextType(Arg1 arg1)
            : _channel(*this, arg1)
            , _adminLock()
            , _sendQueue()
            , _offset(0)
            , _receiver()
            , _terminator()
        {
        }
        template <typename Arg1, typename Arg2>
        StreamTextType(Arg1 arg1, Arg2 arg2)
            : _channel(*this, arg1, arg2)
            , _adminLock()
            , _sendQueue()
            , _offset(0)
            , _receiver()
            , _terminator()
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3>
        StreamTextType(Arg1 arg1, Arg2 arg2, Arg3 arg3)
            : _channel(*this, arg1, arg2, arg3)
            , _adminLock()
            , _sendQueue()
            , _offset(0)
            , _receiver()
            , _terminator()
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
        StreamTextType(Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4)
            : _channel(*this, arg1, arg2, arg3, arg4)
            , _adminLock()
            , _sendQueue()
            , _offset(0)
            , _receiver()
            , _terminator()
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
        StreamTextType(Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5)
            : _channel(*this, arg1, arg2, arg3, arg4, arg5)
            , _adminLock()
            , _sendQueue()
            , _offset(0)
            , _receiver()
            , _terminator()
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6>
        StreamTextType(Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6)
            : _channel(*this, arg1, arg2, arg3, arg4, arg5, arg6)
            , _adminLock()
            , _sendQueue()
            , _offset(0)
            , _receiver()
            , _terminator()
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7>
        StreamTextType(Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6, Arg7 arg7)
            : _channel(*this, arg1, arg2, arg3, arg4, arg5, arg6, arg7)
            , _adminLock()
            , _sendQueue()
            , _offset(0)
            , _receiver()
            , _terminator()
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7, typename Arg8>
        StreamTextType(Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6, Arg7 arg7, Arg8 arg8)
            : _channel(*this, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)
            , _adminLock()
            , _sendQueue()
            , _offset(0)
            , _receiver()
            , _terminator()
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7, typename Arg8, typename Arg9>
        StreamTextType(Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6, Arg7 arg7, Arg8 arg8, Arg9 arg9)
            : _channel(*this, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9)
            , _adminLock()
            , _sendQueue()
            , _offset(0)
            , _receiver()
            , _terminator()
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7, typename Arg8, typename Arg9, typename Arg10>
        StreamTextType(Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6, Arg7 arg7, Arg8 arg8, Arg9 arg9, Arg10 arg10)
            : _channel(*this, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10)
            , _adminLock()
            , _sendQueue()
            , _offset(0)
            , _receiver()
            , _terminator()
        {
        }
        template <typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7, typename Arg8, typename Arg9, typename Arg10, typename Arg11>
        StreamTextType(Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6, Arg7 arg7, Arg8 arg8, Arg9 arg9, Arg10 arg10, Arg11 arg11)
            : _channel(*this, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11)
            , _adminLock()
            , _sendQueue()
            , _offset(0)
            , _receiver()
            , _terminator()
        {
        }

POP_WARNING()
        virtual ~StreamTextType()
        {
            _channel.Close(Core::infinite);
        }

    public:
        virtual void Received(string& text) = 0;
        virtual void Send(const string& text) = 0;
        virtual void StateChange() = 0;

        void Submit(const string& element)
        {
            _adminLock.Lock();

            if (_channel.IsOpen() == true) {
                _sendQueue.push_back(element);
            }

            _adminLock.Unlock();

            _channel.Trigger();
        }
        inline uint32_t Open(const uint32_t waitTime)
        {
            return (_channel.Open(waitTime));
        }
        inline uint32_t Close(const uint32_t waitTime)
        {
            return (_channel.Close(waitTime));
        }
        inline bool IsOpen() const
        {
            return (_channel.IsOpen());
        }
        inline bool IsClosed() const
        {
            return (_channel.IsClosed());
        }
        inline bool IsSuspended() const
        {
            return (_channel.IsSuspended());
        }
        inline string LocalId() const
        {
            return (_channel.LocalId());
        }
        inline void Ping()
        {
            return (_channel.Ping());
        }

    private:
        virtual bool IsIdle() const
        {
            return (_sendQueue.size() == 0);
        }
        uint16_t SendData(uint8_t* dataFrame, const uint16_t maxSendSize)
        {
            uint16_t result = 0;

            _adminLock.Lock();

            if (_offset == static_cast<uint32_t>(~0)) {
                // Seems we have to report a ready state..
                Send(_sendQueue.front());

                // We are done with this entry it has been sent!!! discard it.
                _sendQueue.pop_front();
                _offset = 0;
            }

            if (_sendQueue.size() > 0) {
                const string& sendObject = _sendQueue.front();

                // Do we still need to send data from the text..
                if (_offset < (sendObject.size() * sizeof(TCHAR))) {
                    result = (((sendObject.size() * sizeof(TCHAR)) - _offset) > maxSendSize ? maxSendSize : ((sendObject.size() * sizeof(TCHAR)) - _offset));

                    _offset += SendCharacters(dataFrame, &(sendObject.c_str()[(_offset / sizeof(TCHAR))]), (_offset % sizeof(TCHAR)), result);
                }

                // See if we can write the closing marker
                if ((maxSendSize != result) && (_offset >= (sendObject.size() * sizeof(TCHAR))) && (_offset < ((sendObject.size() + (_terminator.SizeOf())) * sizeof(TCHAR)))) {
                    uint8_t markerSize = (static_cast<uint8_t>(_terminator.SizeOf()) * sizeof(TCHAR));
                    uint8_t markerOffset = ((sendObject.size() * sizeof(TCHAR)) - _offset);
                    uint16_t size = ((markerSize - markerOffset) > (maxSendSize - result) ? (maxSendSize - result) : (markerSize - markerOffset));

                    _offset += SendCharacters(&(dataFrame[result]), &(_terminator.Marker()[(markerOffset / sizeof(TCHAR))]), (markerOffset % sizeof(TCHAR)), size);
                    result += size;

                    if ((size + markerOffset) == markerSize) {
                        // Report as completed on the next run
                        _offset = static_cast<uint32_t>(~0);
                    }
                }

                // If we went through this entry we must have processed something....
                ASSERT(result != 0);
            }

            _adminLock.Unlock();

            return (result);
        }
        inline void Convert(const uint8_t* dataFrame, wchar_t& entry) const
        {
            entry = ntohs((dataFrame[0] << 8) | dataFrame[1]);
        }
        inline void Convert(const uint8_t* dataFrame, char& entry) const
        {
            entry = (dataFrame[0]);
        }
        uint16_t ReceiveData(uint8_t* dataFrame, const uint16_t receivedSize)
        {
            for (uint16_t index = 0; index < receivedSize; index += sizeof(TCHAR)) {
                TCHAR character;
                Convert(&dataFrame[index], character);

                // 0x80 -> Termination complete. Text Terminated
                // 0x40 -> Termination sequence in Progress.
                // 0x03 -> Number of characters to delete
                uint8_t terminated = _terminator.IsTerminated(character);

                if ((terminated & 0x80) == 0) {
                    _receiver.push_back(character);
                } else {
                    uint8_t dropCharacters = terminated & 0x03;

                    while (dropCharacters != 0) {
#ifdef __WINDOWS__
                        _receiver.pop_back();
#endif
#ifdef __POSIX__
                        _receiver.erase(_receiver.end() - 1);
#endif
                        dropCharacters--;
                    }

                    // report the new string...
                    Received(_receiver);

                    _receiver.clear();
                }
            }
            return (receivedSize);
        }
        inline uint16_t SendCharacters(uint8_t* dataFrame, const TCHAR stream[], const uint8_t delta, const uint16_t total)
        {
            // TODO: Align in case we are not a multibyte character string..
            // For now we assume that this never happens, only multibyte support for now.
            ASSERT(delta == 0);

            // Copying from an aligned position..
            ::memcpy(dataFrame, stream, total);

            return (total);
        }

    private:
        HandlerType<BaseClass, SOURCE> _channel;
        Core::CriticalSection _adminLock;
        std::list<string> _sendQueue;
        uint32_t _offset;
        string _receiver;
        TEXTTERMINATOR _terminator;
    };
}
} // namespace Core

#endif // __STREAMTEXT_
