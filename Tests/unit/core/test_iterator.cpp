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

#include "../IPTestAdministrator.h"

#include <gtest/gtest.h>
#include <core/core.h>

namespace Thunder {
namespace Tests {

    class DataClass {
    public:
        DataClass() = delete;

        DataClass(uint8_t value)
            : _value(value)
        {
        }

        ~DataClass()
        {
        }

        uint8_t Value()
        {
            return _value;
        }
    private:
        uint8_t _value;
    };

    template <typename CONTAINER, typename ELEMENT>
    class IteratorClass : public Core::IteratorType<CONTAINER, ELEMENT>
    {
    private:
        typedef Core::IteratorType<CONTAINER, ELEMENT> BaseClass;
    public:
        IteratorClass() =delete;

        IteratorClass(CONTAINER& container)
            : BaseClass(container)
        {
            EXPECT_EQ(container, *(BaseClass::Container()));
        }

        ~IteratorClass()
        {
        }
    };

    template <typename CONTAINER, typename ELEMENT, typename KEY>
    class IteratorMapClass : public Core::IteratorMapType<CONTAINER, ELEMENT, KEY>
    {
    private:
        typedef Core::IteratorMapType<CONTAINER, ELEMENT, KEY> BaseClass;

    public:
        IteratorMapClass() = delete;

        IteratorMapClass(CONTAINER& container)
            : BaseClass(container)
        {
            EXPECT_EQ(container, *(BaseClass::Container()));
        }

        ~IteratorMapClass()
        {
        }
    };

    typedef IteratorClass<std::list<DataClass*>, DataClass*> DataIterator;
    typedef IteratorMapClass<std::map<uint8_t, DataClass*>, DataClass*, uint8_t> DataMapIterator;

    template <typename T>
    void Validate(T iterator, uint8_t index)
    {
        EXPECT_EQ(index, iterator.Index());
        if ((index > 0) && (index < 4)) {
            EXPECT_EQ(index, (*iterator)->Value());
            EXPECT_EQ(index, iterator.Current()->Value());
            EXPECT_EQ(index, iterator->Value());
        }
    }

    template <typename T>
    void ValidateMap(T iterator, uint8_t index)
    {
        EXPECT_EQ(index, iterator.Index());
        if ((index > 0) && (index < 4)) {
            EXPECT_EQ(index, (*iterator)->Value());
            EXPECT_EQ(index, iterator.Current()->Value());
            EXPECT_EQ(index, iterator->Value());
            EXPECT_EQ(index, iterator.Key());
        }
    }

    TEST(Core_IIterator, IteratorType)
    {
        DataClass entry1(1);
        DataClass entry2(2);
        DataClass entry3(3);
        std::list<DataClass*> list{&entry1, &entry2, &entry3};
        DataIterator iterator(list);
        EXPECT_EQ(iterator.Count(), 3u);

        for (uint8_t i = 0; i <= 4; i++) {
            iterator.Reset(i);
            Validate<DataIterator>(iterator, i);
        }

        for (uint8_t i = 3; iterator.Previous(); i--)
            Validate<DataIterator>(iterator, i);

        for (uint8_t i = 1; iterator.Next(); i++)
            Validate<DataIterator>(iterator, i);
    }

    TEST(Core_IIterator, IteratorMapType)
    {
        DataClass entry1(1);
        DataClass entry2(2);
        DataClass entry3(3);
        std::map<uint8_t, DataClass*> map{{1, &entry1}, {2, &entry2}, {3, &entry3}};    
        DataMapIterator iterator(map);
        EXPECT_EQ(iterator.Count(), 3u);

        for (uint8_t i = 0; i <= 4; i++) {
            iterator.Reset(i);
            ValidateMap<DataMapIterator>(iterator, i);
        }

        for (uint8_t i = 3; iterator.Previous(); i--)
            ValidateMap<DataMapIterator>(iterator, i);

        for (uint8_t i = 1; iterator.Next(); i++)
            ValidateMap<DataMapIterator>(iterator, i);
    }
} // Tests
} // Thunder
