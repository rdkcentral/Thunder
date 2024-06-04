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
 
#ifndef __VALUERECORDER_H
#define __VALUERECORDER_H

// ---- Include system wide include files ----

// ---- Include local include files ----
#include "FileSystem.h"
#include "Module.h"
#include "Number.h"
#include "Time.h"
#include "TypeTraits.h"

// ---- Referenced classes and types ----

// ---- Helper types and constants ----

// ---- Helper functions ----

// ---- Class Definition ----
namespace Thunder {
namespace Core {
    // C = continuation. [1] Next byte is part of this element, [0] Last part of element
    // T = Type. [1] = Time / [0] = Value
    // S = Sign. [0] = positive / [1] = negative
    // D = Data bit
    // +---+---+---+---+---+---+---+---+  +---+---+---+---+---+---+---+---+
    // | C | T | S | D | D | D | D | D |  | C | D | D | D | D | D | D | D |
    // +---+---+---+---+---+---+---+---+  +---+---+---+---+---+---+---+---+
    //
    // Each file starts with
    // [32 bits]            ID of first measurement
    // [ STOREVALUE BITS]   First full measurment.
    // [64 bits]            Absolute time of first measurement stored.
    // [32 bits]            ID of last measurement
    // [ STOREVALUE BITS]   Last  measurment.
    // [64 bits]            Absolute time of last measurement stored.
    // ...................  Measurements according to the specification above.
    //                      Only deltas are stored of time and value. If the
    //                      delta of the time, is equal to the previous delta,
    //                      it is NOT stored !!!
    template <typename STOREVALUE, const unsigned int BLOCKSIZE>
    class RecorderType {
    private:
        class BaseRecorder {
        public:
            BaseRecorder(BaseRecorder&&) = delete;
            BaseRecorder(const BaseRecorder&) = delete;
            BaseRecorder& operator=(BaseRecorder&&) = delete;
            BaseRecorder& operator=(const BaseRecorder&) = delete;

        public:
            struct Absolute {
                uint32_t _id;
                STOREVALUE _value;
                uint64_t _time;
            };

        protected:
            void SetBuffer(uint8_t* buffer)
            {
                _storage = buffer;
            }

        public:
            BaseRecorder()
                : _currentDelta(0)
                , _currentId(0)
                , _currentTime(0)
                , _currentValue(0)
                , _index(0)
                , _storage(nullptr)
            {
                _start._time = 0;
            }
            ~BaseRecorder()
            {
            }

        public:
            static uint32_t constexpr CaptureSize = ((BLOCKSIZE * 1024) - (2 * sizeof(Absolute)));

            inline const Absolute& Start() const
            {
                return (_start);
            }
            inline uint32_t Id() const
            {
                return (_currentId);
            }
            inline uint64_t Time() const
            {
                return (_currentTime);
            }
            inline STOREVALUE Value() const
            {
                return (_currentValue);
            }

        protected:
            inline void Start(const uint32_t id)
            {
                _start._id = id;
                _start._time = 0;
                _start._value = 0;
                _currentDelta = 0;
                _currentId = id;
                _index = 0;
            }
            inline void Set(const Absolute& value)
            {
                _currentId = value._id;
                _currentValue = value._value;
                _currentTime = value._time;
            }
            uint32_t Store(const uint64_t time, const STOREVALUE value)
            {
                if (_start._time == 0) {
                    ASSERT(_currentDelta == 0);
                    ASSERT((_currentId != 0) && (_currentId != static_cast<uint32_t>(~0)));

                    // This is the first recording !!!
                    _start._time = time;
                    _start._value = value;
                    _start._id = _currentId;
                } else {
                    _currentId++;

                    // Calculate the Absolute delta in "milliSeconds"
                    uint32_t delta = static_cast<uint32_t>((time - _currentTime) / 1000);

                    // If it is a different absolute Delta, store it relatively..
                    if (_currentDelta != delta) {
                        bool negative = (_currentDelta > delta);

                        // There is a delta, so we need to store it.
                        StoreDelta(true, negative, (negative ? _currentDelta - delta : delta - _currentDelta));

                        _currentDelta = delta;
                    }

                    // Now store the actual value...
                    bool negative = (_currentValue > value);

                    StoreDelta(false, negative, (negative ? _currentValue - value : value - _currentValue));
                }

                _currentValue = value;
                _currentTime = time;

                return (_index);
            }

            void StepForward()
            {
                ASSERT(_index < CaptureSize);

                _currentId++;

                bool time, negative;
                uint64_t delta;
                ForwardElement(time, negative, delta);

                // Adjust the time
                if (time == true) {
                    ASSERT(delta <= NUMBER_MAX_UNSIGNED(uint32_t));

                    if (negative) {
                        _currentDelta -= static_cast<uint32_t>(delta);
                    } else {
                        _currentDelta += static_cast<uint32_t>(delta);
                    }

                    ForwardElement(time, negative, delta);
                }

                ASSERT(time == false);

                _currentTime = _currentTime + (_currentDelta * 1000);

                ASSERT(static_cast<STOREVALUE>(delta) <= NUMBER_MAX_UNSIGNED(STOREVALUE));

                if (negative) {
                    _currentValue -= static_cast<STOREVALUE>(delta);
                } else {
                    _currentValue += static_cast<STOREVALUE>(delta);
                }
            }

            void StepBack()
            {
                ASSERT(_index > 0);

                _currentId--;

                bool time, negative;
                uint64_t delta;

                BackwardElement(time, negative, delta);

                ASSERT(time == false);

                _currentTime = _currentTime - (_currentDelta * 1000);

                ASSERT(delta <= NUMBER_MAX_UNSIGNED(STOREVALUE));

                if (negative) {
                    _currentValue += static_cast<STOREVALUE>(delta);
                } else {
                    _currentValue -= static_cast<STOREVALUE>(delta);
                }

                if (_currentId == _start._id) {
                    // We reached the beginning..
                    _index = 0;
                    _currentDelta = 0;
                } else {
                    uint32_t handled = BackwardElement(time, negative, delta);

                    // Adjust the time
                    if (time == true) {
                        if (negative) {
                            _currentDelta += static_cast<uint32_t>(delta);
                        } else {
                            _currentDelta -= static_cast<uint32_t>(delta);
                        }
                    } else {
                        _index += handled;
                    }
                }
            }

            void SnapShot(BaseRecorder& copy, const uint32_t id) const
            {
                // Copy all recordings..
                ::memcpy(copy._storage, _storage, _index);

                // Copy all metadata
                copy._start = _start;

                if (id < _currentId) {
                    copy._currentId = _start._id;
                    copy._currentValue = _start._value;
                    copy._currentTime = _start._time;
                    copy._index = 0;
                    copy._currentDelta = 0;

                    if (id < _start._id) {
                        copy._currentId--;
                    }
                } else {
                    copy._currentId = _currentId;
                    copy._currentTime = _currentTime;
                    copy._currentValue = _currentValue;
                    copy._index = _index;
                    copy._currentDelta = _currentDelta;

                    if (id > _currentId) {
                        copy._currentId++;
                    }
                }
            }

            void Load(Core::File& file, Absolute& end, const uint32_t id)
            {
                uint32_t fileSize = static_cast<uint32_t>(file.Size());

                if (file.Size() <= (2 * sizeof(Absolute))) {
                    ClearData();
                } else {
                    LoadScalar<uint32_t>(file, _start._id);
                    LoadScalar<STOREVALUE>(file, _start._value);
                    LoadScalar<uint64_t>(file, _start._time);

                    LoadScalar<uint32_t>(file, end._id);
                    LoadScalar<STOREVALUE>(file, end._value);
                    LoadScalar<uint64_t>(file, end._time);

                    uint32_t dataSize = fileSize - file.Position();

                    if (file.Read(_storage, dataSize) != dataSize) {
                        ClearData();
                    } else if (id < end._id) {
                        _currentId = _start._id;
                        _currentValue = _start._value;
                        _currentTime = _start._time;
                        _index = 0;
                        _currentDelta = 0;

                        if (id < _start._id) {
                            _currentId--;
                        }
                    } else {
                        _currentId = end._id;
                        _currentValue = end._value;
                        _currentTime = end._time;
                        _index = dataSize;

                        // We need to read from the back. Last one should be
                        // a time delta read it, before returning
                        bool time, negative;
                        uint64_t delta;

                        BackwardElement(time, negative, delta);

                        ASSERT(time == true);
                        ASSERT(negative == true);
                        ASSERT(delta <= NUMBER_MAX_UNSIGNED(uint32_t));

                        _currentDelta = static_cast<uint32_t>(delta);

                        if (id > _currentId) {
                            _currentId++;
                        }
                    }
                }
            }

            void Save(Core::File& file)
            {
                // Save our starting point
                StoreScalar<uint32_t>(file, _start._id);
                StoreScalar<STOREVALUE>(file, _start._value);
                StoreScalar<uint64_t>(file, _start._time);

                // Save our end point
                StoreScalar<uint32_t>(file, _currentId);
                StoreScalar<STOREVALUE>(file, _currentValue);
                StoreScalar<uint64_t>(file, _currentTime);

                // If we step back from the back we start with a delta of "0" and
                // the first measurement should then, with a step back, result in
                // the current delta.
                StoreDelta(true, true, _currentDelta);

                // Save all the Data
                file.Write(_storage, _index);

                // We udated, so time to move on..
                _start._time = 0;
                _start._id = (_currentId + 1);
                _start._value = 0;
                _index = 0;
            }

            void ClearData()
            {
                _start._id = 1;
                _start._value = 0;
                _start._time = 0;

                _index = 0;
            }

        private:
            uint32_t StoreDelta(const bool time, const bool negative, const uint64_t input)
            {
                uint64_t value = input;
                uint32_t addedBytes = 1;

                // For the first byte, we can only have 5 information bits, find out if VALUE fits..
                uint8_t current = (time ? 0x40 : 0) | (negative ? 0x20 : 0) | (value > 32 ? 0x80 : 0) | (value & 0x1F);

                value >>= 5;

                _storage[_index++] = current;

                while (value > 0) {
                    current = (value > 127 ? 0x80 : 0) | (value & 0x7F);
                    value >>= 7;

                    addedBytes++;
                    _storage[_index++] = current;
                }

                return (addedBytes);
            }

            uint32_t ForwardElement(bool& time, bool& negative, uint64_t& value)
            {
                uint32_t readBytes = 1;
                uint8_t digit;
                uint8_t shiftValue = 5;

                digit = _storage[_index++];

                // In the first byte we find flag, and type and continuation.
                time = ((digit & 0x40) != 0);
                negative = ((digit & 0x20) != 0);
                value = (digit & 0x1F);

                while ((digit & 0x80) != 0) {
                    readBytes++;
                    digit = _storage[_index++];

                    value = ((digit & 0x7F) << shiftValue) | value;

                    shiftValue += 7;
                }

                return (readBytes);
            }

            uint32_t BackwardElement(bool& time, bool& negative, uint64_t& result)
            {
                uint32_t stepBack = 1;

                result = 0;

                _index--;

                // The last byte that we read prior to this sitation must close that sequence.
                ASSERT((_storage[_index] & 0x80) == 0);

                // we need to loopback for another 0..
                while ((_storage[_index - 1] & 0x80) != 0) {
                    result = (result << 7) | (_storage[_index] & 0x7F);
                    _index--;
                    stepBack++;
                }

                time = ((_storage[_index] & 0x40) != 0);
                negative = ((_storage[_index] & 0x20) != 0);
                result = (result << 5) | (_storage[_index] & 0x1F);

                return (stepBack);
            }

            template <typename SCALAR>
            uint32_t StoreScalar(File& file, const SCALAR value)
            {
                uint32_t length = 0;

                for (unsigned int index = sizeof(SCALAR); index > 0; index--) {
                    uint8_t digit = (value >> ((index - 1) << 3)) & 0xFF;
                    length += file.Write(&digit, 1);
                }

                return (length);
            }

            template <typename SCALAR>
            uint32_t LoadScalar(const Core::File& file, SCALAR& value) const
            {
                uint32_t length = 0;

                value = 0;

                for (unsigned int index = sizeof(SCALAR); index > 0; index--) {
                    uint8_t digit;
                    length += file.Read(&digit, 1);
                    value = (value << 8) | digit;
                }

                return (length);
            }

        private:
            Absolute _start;
            uint32_t _currentDelta;

            uint32_t _currentId;
            uint64_t _currentTime;
            STOREVALUE _currentValue;

            uint32_t _index;
            uint8_t* _storage;
        };

    public:
        RecorderType(RecorderType<STOREVALUE, BLOCKSIZE>&&) = delete;
        RecorderType(const RecorderType<STOREVALUE, BLOCKSIZE>&) = delete;
        RecorderType<STOREVALUE, BLOCKSIZE> operator=(RecorderType<STOREVALUE, BLOCKSIZE>&&) = delete;
        RecorderType<STOREVALUE, BLOCKSIZE> operator=(const RecorderType<STOREVALUE, BLOCKSIZE>&) = delete;

    public:
        class Writer : public BaseRecorder {
        public:
            Writer() = delete;
            Writer(Writer&& move) = delete;
            Writer(const Writer& copy) = delete;
            Writer& operator=(Writer&& move) = delete;
            Writer& operator=(const Writer& copy) = delete;

        protected:
            Writer(const string fileName)
                : BaseRecorder()
                , _lock()
                , _fileId(static_cast<uint32_t>(~0))
                , _storageName(fileName)
            {
                uint32_t startPoint = 0;

                //Find out the start ID fo the record ID and the file ID.
                NumberType<uint32_t> number(0);

                Core::File file(_storageName + "." + number.Text());

                while (file.Exists() == true) {
                    _fileId = number.Value();
                    number += 1;

                    file = _storageName + "." + number.Text();
                }

                if (_fileId == static_cast<uint32_t>(~0)) {
                    _fileId = 0;
                    startPoint = 1;
                } else {
                    typename BaseRecorder::Absolute end;
                    number = _fileId;
                    file = _storageName + "." + number.Text();

                    if (file.Open(true) == true) {
                        // Find the last possible ID
                        BaseRecorder::Load(file, end, static_cast<uint32_t>(~0));
                        startPoint = (end._id + 1);
                    }

                    _fileId++;
                }

                BaseRecorder::Start(startPoint);
            }

        public:
            ~Writer()
            {
                Save();
            }
            static ProxyType<Writer> Create(const string& filename)
            {
                ProxyType<Writer> result = ProxyType<Writer>::Create(filename);

                result->SetBuffer(result->_buffer);

                return (result);
            }

        public:
            inline const string& Source() const
            {
                return (_storageName);
            }
            void Record(const STOREVALUE value)
            {
                // First do the time tracking so it is as close as possible to the "log time"
                uint64_t time = Core::Time::Now().Ticks();

                _lock.Lock();

                uint32_t size = BaseRecorder::Store(time, value);

                _lock.Unlock();

                if (size >= (BaseRecorder::CaptureSize - ((8 * (sizeof(STOREVALUE) + sizeof(uint64_t))) / 6))) {
                    Save();
                }
            }
            void Copy(BaseRecorder& copy, const uint32_t id) const
            {
                // Lock, we need a consitent set of data, do not add a new recording
                _lock.Lock();

                BaseRecorder::SnapShot(copy, id);

                _lock.Unlock();
            }
            void Save()
            {
                _lock.Lock();

                // Save the whole shebang and move on..
                NumberType<uint32_t> number(_fileId);

                Core::File file(_storageName + "." + number.Text());

                if (file.Create()) {

                    BaseRecorder::Save(file);
                    file.Close();

                    _fileId++;
                }

                _lock.Unlock();
            }

        private:
            mutable Core::CriticalSection _lock;
            uint32_t _fileId;
            string _storageName;
            uint8_t _buffer[BaseRecorder::CaptureSize];
        };

        class Reader : public BaseRecorder {
        public:
            Reader(Reader&& move) = delete;
            Reader(const Reader& copy) = delete;
            Reader& operator=(Reader&& move) = delete;
            Reader& operator=(const Reader& copy) = delete;

        public:
            Reader(const ProxyType<Writer>& recorder, const uint32_t id = static_cast<uint32_t>(~0))
                : BaseRecorder()
                , _storageName(recorder->Source())
                , _currentSet(recorder)
            {
                BaseRecorder::SetBuffer(_buffer);
                Reset(id);
            }
            Reader(const string& fileName, const uint32_t id = 0)
                : BaseRecorder()
                , _storageName(fileName)
                , _currentSet()
            {
                BaseRecorder::SetBuffer(_buffer);
                Reset(id);
            }
            ~Reader()
            {
            }

        public:
            inline uint32_t StartId() const
            {
                return (BaseRecorder::Start()._id);
            }
            inline uint32_t EndId() const
            {
                return (_end._id);
            }
            inline const string& Source() const
            {
                return (_storageName);
            }
            inline bool IsValid() const
            {
                return ((BaseRecorder::Id() >= BaseRecorder::Start()._id) && (BaseRecorder::Id() <= _end._id));
            }
            void Reset(const uint32_t id)
            {
                _fileId = static_cast<uint32_t>(~0);

                bool correctFile = false;

                // Now create the storage space.
                NumberType<uint32_t> number(0);

                Core::File file(_storageName + '.' + number.Text());

                while ((correctFile == false) && (file.Exists() == true) && (file.Open(true) == true)) {
                    BaseRecorder::Load(file, _end, id);

                    correctFile = (id <= _end._id);

                    file.Close();

                    _fileId++;

                    if (correctFile == false) {
                        number += 1;

                        file = _storageName + '.' + number.Text();
                    }
                }

                if ((correctFile == false) && (_currentSet.IsValid() == true)) {
                    _fileId++;

                    _end._id = _currentSet->Id();
                    _end._time = _currentSet->Time();
                    _end._value = _currentSet->Value();
                    _currentSet->Copy(*this, id);
                }

                if ((id > BaseRecorder::Start()._id) && (id < _end._id)) {
                    while ((Next() == true) && (BaseRecorder::Id() < id)) /* intentionally left empty */
                        ;
                }
            }
            bool Next()
            {
                if (BaseRecorder::Id() < BaseRecorder::Start()._id) {
                    BaseRecorder::Set(BaseRecorder::Start());
                } else if (BaseRecorder::Id() < (_end._id - 1)) {
                    BaseRecorder::StepForward();
                } else if (BaseRecorder::Id() == (_end._id - 1)) {
                    BaseRecorder::Set(_end);
                } else if (BaseRecorder::Id() == _end._id) {
                    typename BaseRecorder::Absolute point = _end;
                    point._id++;

                    BaseRecorder::Set(point);

                    if ((_currentSet.IsValid() == false) || (BaseRecorder::Id() < _currentSet->Start()._id)) {
                        // See if we can load a new file
                        NumberType<uint32_t> number(++_fileId);

                        Core::File file(_storageName + '.' + number.Text());

                        if ((file.Exists() == true) && (file.Open(true) == true)) {
                            BaseRecorder::Load(file, _end, BaseRecorder::Id());
                        }
                    } else if (_currentSet.IsValid() == true) {
                        _end._id = _currentSet->Id();
                        _end._time = _currentSet->Time();
                        _end._value = _currentSet->Value();
                        _currentSet->Copy(*this, BaseRecorder::Id());
                    }
                }

                return (IsValid());
            }
            bool Previous()
            {
                if (BaseRecorder::Id() > _end._id) {
                    BaseRecorder::Set(_end);
                } else if (BaseRecorder::Id() >= (BaseRecorder::Start()._id + 1)) {
                    BaseRecorder::StepBack();
                } else if (BaseRecorder::Id() == BaseRecorder::Start()._id) {
                    typename BaseRecorder::Absolute point = BaseRecorder::Start();
                    point._id--;

                    BaseRecorder::Set(point);

                    if (_fileId > 0) {
                        // See if we can load a new file
                        NumberType<uint32_t> number(--_fileId);

                        Core::File file(_storageName + '.' + number.Text());

                        if ((file.Exists() == true) && (file.Open(true) == true)) {
                            BaseRecorder::Load(file, _end, BaseRecorder::Id());
                        }
                    }
                }

                return (IsValid());
            }

        private:
            typename BaseRecorder::Absolute _end;
            uint32_t _fileId;
            string _storageName;
            ProxyType<Writer> _currentSet;
            uint8_t _buffer[BaseRecorder::CaptureSize];
        };
    };
}

} // Namespace Core

#endif // __VALUERECORDER_H
