/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2021 Metrological
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
#include <thread>

namespace WPEFramework {
namespace Tests {

    struct TestWithMember {

        string Test1() {
            string str = "string TestWithMember::Test1()";
            std::cout << str << std::endl;
            return str;
        }
        string Test1(int value) {
            string str = "string TestWithMember::Test1(int) = " + std::to_string(value);
            std::cout << str << std::endl;
            return str;
        }
    };
    struct TestWithoutMember {
        string Test1(char*) {
            string str = "string TestWithoutMember::Test1(uint32_t)";
            std::cout << str << std::endl;
            return str;
        }
    };
    struct TestWithStaticMember {

        static string Test1() {
            string str = "static string TestWithStaticMember::Test1()";
            std::cout << str << std::endl;
            return str;
        }
        static string Test1(int value) {
            string str = "static string TestWithStaticMember::Test1(int) = " + std::to_string(value); 
            std::cout << str << std::endl;
            return str;
        }
    };

    IS_MEMBER_AVAILABLE(Test1, hasTest1);
    template <typename TYPE>
    inline typename Core::TypeTraits::enable_if<hasTest1<TYPE, string>::value, string>::type
    __TestWithMember(TYPE& t)
    {
        return t.Test1();
    }
    template < typename TYPE>
    inline typename Core::TypeTraits::enable_if<!hasTest1<TYPE, string>::value, string>::type
    __TestWithMember(TYPE&)
    {
        return "function not available";
    }

    IS_MEMBER_AVAILABLE(Test1, hasTest1Args);
    template <typename TYPE>
    inline typename Core::TypeTraits::enable_if<hasTest1Args<TYPE, string, int>::value, string>::type
    __Test1WithMemberHasArg(TYPE& t, int value)
    {
        return t.Test1(value);
    }
    template < typename TYPE>
    inline typename Core::TypeTraits::enable_if<!hasTest1Args<TYPE, string, int>::value, string>::type
    __Test1WithMemberHasArg(TYPE&, int)
    {
        return "function not available";
    }

    IS_STATIC_MEMBER_AVAILABLE(Test1, hasStaticTest1);
    template <typename TYPE>
    inline typename Core::TypeTraits::enable_if<hasStaticTest1<TYPE, string>::value, string>::type
    __Test1WithStaticMember()
    {
         return TYPE::Test1();
    }
    template < typename TYPE>
    inline typename Core::TypeTraits::enable_if<!hasStaticTest1<TYPE, string>::value, string>::type
    __Test1WithStaticMember()
    {
        return "function not available";
    }
    IS_STATIC_MEMBER_AVAILABLE(Test1, hasStaticTest1Args);
    template <typename TYPE>
    inline typename Core::TypeTraits::enable_if<hasStaticTest1Args<TYPE, string, int>::value, string>::type
    __Test1WithStaticMemberHasArg(int value)
    {
        return TYPE::Test1(value);
    }
    template < typename TYPE>
    inline typename Core::TypeTraits::enable_if<!hasStaticTest1Args<TYPE, string, int>::value, string>::type
    __Test1WithStaticMemberHasArg(int)
    {
        return "function not available";
    }

    struct TestWithNonStaticAndStaticMember {
        void Test2(bool& value) {
            std::cout << "TestWithNonStaticAndStaticMember: true" << std::endl;
            value = true; 
        }
        string Test2(string str) {
            str = "TestWithNonStaticAndStaticMember: " + str;
            std::cout << str << std::endl;
            return str;
        }
        static bool Test2() {
            string str = "static string TestWithStaticMember::Test1()";
            std::cout << str << std::endl;
            return true;
        }
        static int Test2(int value) {
            string str = "static string TestWithStaticMember::Test1(int) = " + std::to_string(value);
            std::cout << str << std::endl;
            return value;
        }
    };
    struct TestWithStaticAndNonStaticMember {
        static void Test2(bool& value) {
            std::cout << "static TestWithStaticAndNonStaticMember: true" << std::endl;
            value = true;
        }
        static string Test2(string str) {
            str = "static TestWithStaticAndNonStaticMember: " + str;
            std::cout << str << std::endl;
            return str;
        }
        bool Test2() {
            string str = "string TestWithStaticMember::Test1()";
            std::cout << str << std::endl;
            return true;
        }
        int Test2(int value) {
            string str = "string TestWithStaticMember::Test1(int) = " + std::to_string(value);
            std::cout << str << std::endl;
            return value;
        }
    };

    IS_MEMBER_AVAILABLE(Test2, hasTest2ArgBool);
    template <typename TYPE>
    inline typename Core::TypeTraits::enable_if<hasTest2ArgBool<TYPE, void, bool&>::value, void>::type
    __Test2WithMemberHasArg(TYPE& t, bool& value)
    {
        t.Test2(value);
    }
    template < typename TYPE>
    inline typename Core::TypeTraits::enable_if<!hasTest2ArgBool<TYPE, void, bool&>::value, void>::type
    __Test2WithMemberHasArg(TYPE&, bool&)
    {
    }
    IS_MEMBER_AVAILABLE(Test2, hasTest2ArgString);
    template <typename TYPE>
    inline typename Core::TypeTraits::enable_if<hasTest2ArgString<TYPE, string, string&>::value, string>::type
    __Test2WithMemberHasArg(TYPE& t, string& str)
    {
        return t.Test2(str);
    }
    template < typename TYPE>
    inline typename Core::TypeTraits::enable_if<!hasTest2ArgString<TYPE, string, string&>::value, string>::type
    __Test2WithMemberHasArg(TYPE&, string&)
    {
    }

    IS_STATIC_MEMBER_AVAILABLE(Test2, hasTest2NoArg);
    template <typename TYPE>
    inline typename Core::TypeTraits::enable_if<hasTest2NoArg<TYPE, bool>::value, bool>::type
    __Test2WithStaticMemberHasArg()
    {
        return TYPE::Test2();
    }
    template < typename TYPE>
    inline typename Core::TypeTraits::enable_if<!hasTest2NoArg<TYPE, bool>::value, bool>::type
    __Test2WithStaticMemberHasArg()
    {
        return false;
    }
    IS_STATIC_MEMBER_AVAILABLE(Test2, hasTest2ArgInt);
    template <typename TYPE>
    inline typename Core::TypeTraits::enable_if<hasTest2ArgInt<TYPE, int, int>::value, int>::type
    __Test2WithStaticMemberHasArg(int value)
    {
        return TYPE::Test2(value);
    }
    template < typename TYPE>
    inline typename Core::TypeTraits::enable_if<!hasTest2ArgInt<TYPE, int, int>::value, int>::type
    __Test2WithStaticMemberHasArg(int)
    {
        return 0;
    }

    TEST(IsMemberAvailable, TestWithMember)
    {
        TestWithMember withMember;
        EXPECT_STREQ((__TestWithMember(withMember)).c_str(), "string TestWithMember::Test1()");
        EXPECT_STREQ((__Test1WithMemberHasArg(withMember, 1)).c_str(), "string TestWithMember::Test1(int) = 1");
    }
    TEST(IsMemberAvailable, TestWithoutMember)
    {
        TestWithoutMember withoutMember;
        EXPECT_STREQ((__TestWithMember(withoutMember)).c_str(), "function not available");
        EXPECT_STREQ((__Test1WithMemberHasArg(withoutMember, 1)).c_str(), "function not available");
    }
    TEST(IsMemberAvailable, TestWithStaticMember)
    {
        TestWithStaticMember withStaticMember;
        EXPECT_STREQ((__TestWithMember(withStaticMember)).c_str(), "static string TestWithStaticMember::Test1()");
        EXPECT_STREQ((__Test1WithMemberHasArg(withStaticMember, 2)).c_str(), "static string TestWithStaticMember::Test1(int) = 2");
    }
    TEST(IsStaticMemberAvailable, TestWithStaticMember)
    {
        EXPECT_STREQ((__Test1WithStaticMember<TestWithStaticMember>()).c_str(), "static string TestWithStaticMember::Test1()");
        EXPECT_STREQ((__Test1WithStaticMemberHasArg<TestWithStaticMember>(3)).c_str(), "static string TestWithStaticMember::Test1(int) = 3");
    }
    TEST(IsStaticMemberAvailable, TestWithMember)
    {
        EXPECT_STREQ((__Test1WithStaticMember<TestWithMember>()).c_str(), "function not available");
        EXPECT_STREQ((__Test1WithStaticMemberHasArg<TestWithMember>(1)).c_str(), "function not available");
    }
    TEST(IsNonStaticAndStaticMemberAvailable, TestWithNonStaticAndStaticMember)
    {
        bool value = false;
        TestWithNonStaticAndStaticMember withNonStaticAndStaticMember;
        __Test2WithMemberHasArg(withNonStaticAndStaticMember, value);
        EXPECT_EQ(value,  true);
        string str = "Hello";
        EXPECT_STREQ((__Test2WithMemberHasArg(withNonStaticAndStaticMember, str)).c_str(), "TestWithNonStaticAndStaticMember: Hello");

        EXPECT_EQ(__Test2WithStaticMemberHasArg<TestWithNonStaticAndStaticMember>(value), true);
        EXPECT_EQ((__Test2WithStaticMemberHasArg<TestWithNonStaticAndStaticMember>(5)), 5);

	value = __Test2WithStaticMemberHasArg<TestWithNonStaticAndStaticMember>();
        EXPECT_EQ(value, true);
    }
    TEST(IsStaticAndNonStaticMemberAvailable, TestWithStaticAndNonStaticMember)
    {
        bool value = false;
        TestWithStaticAndNonStaticMember withStaticAndNonStaticMember;
        __Test2WithMemberHasArg(withStaticAndNonStaticMember, value);
        EXPECT_EQ(value,  true);
        string str = "Hello";
        EXPECT_STREQ((__Test2WithMemberHasArg(withStaticAndNonStaticMember, str)).c_str(), "static TestWithStaticAndNonStaticMember: Hello");

        EXPECT_EQ(__Test2WithStaticMemberHasArg<TestWithStaticAndNonStaticMember>(value), false);
        EXPECT_EQ((__Test2WithStaticMemberHasArg<TestWithStaticAndNonStaticMember>(6)), 0);

	value = __Test2WithStaticMemberHasArg<TestWithStaticAndNonStaticMember>();
        EXPECT_EQ(value, false);
    }
}
}
