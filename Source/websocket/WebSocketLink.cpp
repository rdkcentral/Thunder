#include "WebSocketLink.h"

namespace WPEFramework {
namespace Web {
    namespace WebSocket {
        static RequestAllocator _requestAllocator;
        static ResponseAllocator _responseAllocator;

        /* static */ RequestAllocator& RequestAllocator::Instance()
        {
            return (_requestAllocator);
        }

        /* static */ ResponseAllocator& ResponseAllocator::Instance()
        {
            return (_responseAllocator);
        }

        static const uint8_t CONTINUATION_FRAME = 0x00;
        static const uint8_t FINISHING_FRAME = 0x80;
        static const uint8_t TYPE_FRAME = 0x0F;
        static const uint8_t MASKING_FRAME = 0x80;
        static const uint8_t CONTROL_FRAME = 0x08;
        static const uint8_t HandShakeKey[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

        std::string Protocol::RequestKey() const
        {
            string baseEncodedKey;
            uint16_t randomBytes[8];

            for (uint8_t teller = 0; teller < 8; teller++) {
                Crypto::Random(randomBytes[teller]);
            }
            Core::ToString(reinterpret_cast<const uint8_t*>(&randomBytes[0]), sizeof(randomBytes), true, baseEncodedKey);

            return (baseEncodedKey);
        }

        std::string Protocol::ResponseKey(const std::string& requestKey) const
        {
            string baseEncodedKey;

            Crypto::SHA1 shaCalculator(reinterpret_cast<const uint8_t*>(requestKey.c_str()), static_cast<uint16_t>(requestKey.length()));

            shaCalculator.Input(HandShakeKey, sizeof(HandShakeKey) - 1);

            Core::ToString(shaCalculator.Result(), 20, true, baseEncodedKey);

            return (baseEncodedKey);
        }

/*  %x0 denotes a continuation frame
 *  %x1 denotes a text frame
 *  %x2 denotes a binary frame
 *  %x3-7 are reserved for further non-control frames
 *  %x8 denotes a connection close
 *  %x9 denotes a ping
 *  %xA denotes a pong
 *  %xB-F are reserved for further control frames
 */
        uint16_t Protocol::Encoder(uint8_t* dataFrame, const uint16_t maxSendSize, const uint16_t usedSize)
        {
            uint32_t result = 0;

            if ((usedSize != 0) || (SendInProgress() == true)) {
                result = (usedSize <= 125 ? 2 : 4);

                if ((_setFlags & MASKING_FRAME) == 0) {
                    // Only if this is a smallFrame, we need to "flush" 2 bytes..
                    if ((result == 2) && (usedSize != 0)) {
                        // No masking, so just move and insert the header up front..
                        ::memcpy(&dataFrame[2], &(dataFrame[4]), usedSize);
                    }
                }
                else {
		    uint32_t value;
                    // Generate a new mask value
                    uint8_t maskKey[4];
                    Crypto::Random(value);
		    maskKey[0] = value & 0xFF;
		    maskKey[1] = (value >> 8) & 0xFF;
		    maskKey[2] = (value >> 16) & 0xFF;
		    maskKey[3] = (value >> 24) & 0xFF;

                    // Mask and insert the bytes on the right spots
                    uint8_t* source = &dataFrame[usedSize - 1 + 4];
                    uint8_t* destination = &dataFrame[usedSize - 1 + 4 + result];
                    uint32_t bytesToMove = usedSize;

                    while (bytesToMove != 0) {
                        bytesToMove--;
                        *destination-- = (*source ^ maskKey[(bytesToMove & 0x3)]);
                        source--;
                    }

                    // Now there is space again, write down the encryption key.
                    ::memcpy(&dataFrame[result], &maskKey, 4);
                    result += 4;
                }

                if (usedSize <= 125) {
                    dataFrame[1] = ((_setFlags & MASKING_FRAME) | usedSize);
                }
                else {
                    // We only allows uin1t 16 so no need to go beyond two bytes..
                    dataFrame[1] = ((_setFlags & MASKING_FRAME) | 126);
                    dataFrame[2] = (usedSize >> 8);
                    dataFrame[3] = (usedSize & 0xFF);
                }

                if (usedSize < maxSendSize) {
                    // Seems like not all available space is used, so I guess we are ready..
                    dataFrame[0] = FINISHING_FRAME | (SendInProgress() == true ? CONTINUATION_FRAME : TYPE_FRAME & _setFlags);
                    _progressInfo &= (~0x40);
                }
                else {
                    // There is more to come, this is just part of a bigger picture
                    dataFrame[0] = (SendInProgress() == true ? CONTINUATION_FRAME : TYPE_FRAME & _setFlags);
                    _progressInfo |= (0x40);
                }

                result += usedSize;
            }

            if (((_controlStatus & (REQUEST_CLOSE | REQUEST_PING | REQUEST_PONG)) != 0) && ((result + 1) < maxSendSize)) {
                if ((_controlStatus & REQUEST_CLOSE) != 0) {
                    dataFrame[result++] = FINISHING_FRAME | Protocol::CLOSE;
                    _controlStatus &= (~REQUEST_CLOSE);
                    dataFrame[result++] = 0;
                }
                if (((_controlStatus & REQUEST_PING) != 0) && ((result + 1) < maxSendSize)) {
                    dataFrame[result++] = FINISHING_FRAME | Protocol::PING;
                    _controlStatus &= (~REQUEST_PING);
                    dataFrame[result++] = 0;
                }
                if (((_controlStatus & REQUEST_PONG) != 0) && ((result + 1) < maxSendSize)) {
                    dataFrame[result++] = FINISHING_FRAME | Protocol::PONG;
                    _controlStatus &= (~REQUEST_PONG);
                    dataFrame[result++] = 0;
                }
            }

            return (result);
        }

        uint16_t Protocol::Decoder(uint8_t* dataFrame, uint16_t& receivedSize)
        {
            uint16_t actualHeader = 0;

            ASSERT(receivedSize > 0);

            if (_pendingReceiveBytes > 0) {
                // Just unscramble, what is left...
                if ((_progressInfo & 0x20) == 0x20) {
                    // looks like we need to unscramble..
                    uint8_t* source = dataFrame;

                    if (_pendingReceiveBytes < receivedSize) {
                        receivedSize = _pendingReceiveBytes;
                    }

                    while (_pendingReceiveBytes != 0) {
                        *source = (*source ^ _scrambleKey[(_progressInfo & 0x3)]);
                        source++;
			_progressInfo = ((_progressInfo + 1) & 0x03) | (_progressInfo & 0xFC);
                        _pendingReceiveBytes--;
                    }
                }
                else {
                    if (_pendingReceiveBytes > receivedSize) {
                        _pendingReceiveBytes -= receivedSize;
                    }
                    else {
                        receivedSize = _pendingReceiveBytes;
                        _pendingReceiveBytes = 0;
                    }
                }
            }
            else if (receivedSize < 2) {
                // This is a way too small frame..
                receivedSize = 0;
            }
            else {
                // This seems to be a new frame, check it out !!!
                uint32_t bytesToMove = (dataFrame[1] & 0x7F);

                // check if the full header is present..
                actualHeader = 2 + (bytesToMove == 127 ? 4 : (bytesToMove == 126 ? 2 : 0)) + ((dataFrame[1] & MASKING_FRAME) ? 4 : 0);

                if (actualHeader > receivedSize) {
                    // Frame too small to identify the content yet !!
                    receivedSize = 0;
                    actualHeader = 0;
                }
                else {
                    _frameType = static_cast<frameType>(dataFrame[0] & TYPE_FRAME);

                    // Continuation frame is only allowed if a receive is in progress...
                    if (ReceiveInProgress() == true) {
                        if (_frameType == 0) {
                            _frameType = static_cast<frameType>(TYPE_FRAME & _setFlags);

                            // If this is a continuation, check if this is maybe the last frame...
                            _progressInfo &= ((dataFrame[0] & FINISHING_FRAME) == 0 ? 0xFF : 0x7F);
                        }
                    }
                    else if ((dataFrame[0] & FINISHING_FRAME) == 0) {
                        // Seems we will be in progess from now on.
                        _progressInfo |= 0x80;
                    }

                    // Is this a continuing frame...
                    if ((dataFrame[0] & FINISHING_FRAME) == 0) {
                        // Only if this is a Textframe or a BinaryFrame, The Finished flag is allowed to be '0'.
                        if ((_frameType & TYPE_FRAME) != (_setFlags & TYPE_FRAME)) {
                            _frameType = ((_frameType & CONTROL_FRAME) == 0 ? INCONSISTENT : VIOLATION);
                        }
                    }
                    // Make sure that the package type (if text or Binary) is according to what we expect..
                    else if (((_frameType & CONTROL_FRAME) == 0) && ((_frameType & TYPE_FRAME) != (_setFlags & TYPE_FRAME))) {
                        _frameType = INCONSISTENT;
                    }

                    // If the frame is not an error, unpack/move what is required..
                    if ((_frameType & 0xF8) == 0) {
                        if (bytesToMove == 126) {
                            bytesToMove = ((dataFrame[2] << 8) + dataFrame[3]);
                        }
                        else if (bytesToMove == 127) {
                            bytesToMove = ((dataFrame[2] << 24) + (dataFrame[3] << 16) + (dataFrame[4] << 8) + dataFrame[5]);
                        }

                        // We might not have the full body yet...
                        if ((actualHeader + bytesToMove) > receivedSize) {
                            _pendingReceiveBytes = (actualHeader + bytesToMove - receivedSize);
                            bytesToMove = receivedSize - actualHeader;
                            _progressInfo &= (~0x20);
                        }

                        receivedSize = bytesToMove;

                        // If it is masked, we need to onvert..
                        if ((dataFrame[1] & MASKING_FRAME) != 0) {
                            _scrambleKey[0] = dataFrame[actualHeader - 4];
                            _scrambleKey[1] = dataFrame[actualHeader - 3];
                            _scrambleKey[2] = dataFrame[actualHeader - 2];
                            _scrambleKey[3] = dataFrame[actualHeader - 1];

                            // The last two bits in the progressInfo are used to select the proper scrambling key.
                            // We need to clear them if we start., the 0x20 indicates scrambling required
                            _progressInfo |= 0x20;
                            _progressInfo &= (~0x03);

                            uint8_t* source = &dataFrame[actualHeader];

                            while (bytesToMove != 0) {
                                *source = (*source ^ _scrambleKey[(_progressInfo & 0x3)]);
                                source++;
                                bytesToMove--;
                                _progressInfo = ((_progressInfo + 1) & 0xF3);
                            }
                        }
                    }
                }
            }

            return (actualHeader);
        }
    }
}
}
