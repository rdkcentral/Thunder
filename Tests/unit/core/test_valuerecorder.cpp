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

#include "gtest/gtest.h"

#ifndef MODULE_NAME
#include "../Module.h"
#endif

#include "core/core.h"

namespace Thunder {
namespace Tests {
namespace Core {

    constexpr unsigned int BLOCKSIZE = 20;

    class WriterClass
    {
    public:
        WriterClass() = delete;

        WriterClass(const string& filename)
            : _file{filename}
        {
            _writer = ::Thunder::Core::RecorderType<uint32_t, BLOCKSIZE>::Writer::Create(filename);
        }

        ~WriterClass()
        {
            _writer.Release();
        }

    public:
        void WriterJob(const uint32_t& value)
        {
            ASSERT_TRUE(_writer.IsValid());

            _writer->Record(value);

            EXPECT_STREQ(_writer->Source().c_str(), _file.c_str());
            EXPECT_EQ(_writer->Value(), value);
        }

        void Save()
        {
            _writer->Save();
        }

    private:
        const string _file;
        ::Thunder::Core::ProxyType<::Thunder::Core::RecorderType<uint32_t, BLOCKSIZE>::Writer> _writer;
    };

    class ReaderClass : public ::Thunder::Core::RecorderType<uint32_t, BLOCKSIZE>::Reader
    {
    public:
        ReaderClass() = delete;

        ReaderClass(const string& filename)
            : _file{filename}
            , Reader(filename)
        {
        }

        ~ReaderClass() = default;

    public:
        void ReaderJob(const uint32_t value)
        {
            static_assert(std::is_same<::Thunder::Core::Time::microsecondsfromepoch, uint64_t>::value);

            const ::Thunder::Core::Time::microsecondsfromepoch readTime = ::Thunder::Core::Time::Now().Ticks();

            // Get a valid position
            Reset(StartId());
 
            ASSERT_TRUE(IsValid());

            EXPECT_EQ(Id(), StartId());

            EXPECT_STREQ(Source().c_str(), _file.c_str());
            EXPECT_EQ(value, Value());

            ASSERT_EQ(Id(), EndId());

            // Load next file if it exist if no additional data exist
            EXPECT_FALSE(Next());

            // The previous failed so no new files has been loaded and the current file is still used
            EXPECT_TRUE(Previous());

            EXPECT_LE(Time(), readTime);
        }

    private:
        const string _file;
    };

    TEST(test_valuerecorder, test_writer)
    {
        const string filename = "baseRecorder.txt";

        constexpr uint32_t value = 10;

        WriterClass writer(filename);
        writer.WriterJob(value);
        writer.Save();

        ReaderClass reader(filename);
        reader.ReaderJob(value);
    }

} // Core
} // Tests
} // Thunder
