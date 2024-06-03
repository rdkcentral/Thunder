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

namespace Thunder {
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
    struct TestWithOutStaticMember {

        string Test1() {
            string str = "static string TestWithOutStaticMember::Test1()";
            std::cout << str << std::endl;
            return str;
        }
        string Test1(int value) {
            string str = "static string TestWithOutStaticMember::Test1(int) = " + std::to_string(value);
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
        return "method not available";
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
        return "method not available";
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
        return "method not available";
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
        return "method not available";
    }

    // Function declaration order non static and static
    struct TestWithNonStaticAndStaticMember {
        void Test2(bool& value) {
            std::cout << "TestWithNonStaticAndStaticMember: Test2(bool) = true" << std::endl;
            value = true; 
        }
        string Test2(string str) {
            str = "TestWithNonStaticAndStaticMember: Test2(str) " + str;
            std::cout << str << std::endl;
            return str;
        }
        static bool Test2() {
            string str = "static string TestWithNonStaticAndStaticMember::Test2()  = true";
            std::cout << str << std::endl;
            return true;
        }
        static int Test2(int value) {
            string str = "static string TestWithNonStaticAndStaticMember::Test2(int) = " + std::to_string(value);
            std::cout << str << std::endl;
            return value;
        }
    };
    // Function declaration order static and non static (just reverse of struct TestWithNonStaticAndStaticMember)
    struct TestWithStaticAndNonStaticMember {
        static void Test2(bool& value) {
            std::cout << "static TestWithStaticAndNonStaticMember: Test2(bool) = true" << std::endl;
            value = true;
        }
        static string Test2(string str) {
            str = "static TestWithStaticAndNonStaticMember: Test2(str) " + str;
            std::cout << str << std::endl;
            return str;
        }
        bool Test2() {
            string str = "string TestWithStaticAndNonStaticMember::Test2()";
            std::cout << str << std::endl;
            return true;
        }
        int Test2(int value) {
            string str = "string TestWithStaticAndNonStaticMember::Test2(int) = " + std::to_string(value);
            std::cout << str << std::endl;
            return value;
        }
    };

    IS_MEMBER_AVAILABLE(Test2, hasTest2ArgBool);
    template <typename TYPE>
    inline typename Core::TypeTraits::enable_if<hasTest2ArgBool<TYPE, void, bool&>::value, void>::type
    __Test2WithMemberHasArgBool(TYPE& t, bool& value)
    {
        t.Test2(value);
    }
    template < typename TYPE>
    inline typename Core::TypeTraits::enable_if<!hasTest2ArgBool<TYPE, void, bool&>::value, void>::type
    __Test2WithMemberHasArgBool(TYPE&, bool&)
    {
    }
    IS_MEMBER_AVAILABLE(Test2, hasTest2ArgString);
    template <typename TYPE>
    inline typename Core::TypeTraits::enable_if<hasTest2ArgString<TYPE, string, string&>::value, string>::type
    __Test2WithMemberHasArgString(TYPE& t, string& str)
    {
        return t.Test2(str);
    }
    template < typename TYPE>
    inline typename Core::TypeTraits::enable_if<!hasTest2ArgString<TYPE, string, string&>::value, string>::type
    __Test2WithMemberHasArgString(TYPE&, string&)
    {
    }

    IS_STATIC_MEMBER_AVAILABLE(Test2, hasTest2NoArg);
    template <typename TYPE>
    inline typename Core::TypeTraits::enable_if<hasTest2NoArg<TYPE, bool>::value, bool>::type
    __Test2WithStaticMember()
    {
        return TYPE::Test2();
    }
    template < typename TYPE>
    inline typename Core::TypeTraits::enable_if<!hasTest2NoArg<TYPE, bool>::value, bool>::type
    __Test2WithStaticMember()
    {
        return false;
    }

    IS_STATIC_MEMBER_AVAILABLE(Test2, hasTest2StaticArgBool);
    template <typename TYPE>
    inline typename Core::TypeTraits::enable_if<hasTest2StaticArgBool<TYPE, void, bool>::value, void>::type
    __Test2WithStaticMemberArgBool(bool value)
    {
        TYPE::Test2(value);
    }
    template < typename TYPE>
    inline typename Core::TypeTraits::enable_if<!hasTest2StaticArgBool<TYPE, void, bool>::value, void>::type
    __Test2WithStaticMemberArgBool(bool)
    {
    }

    IS_STATIC_MEMBER_AVAILABLE(Test2, hasTest2ArgInt);
    template <typename TYPE>
    inline typename Core::TypeTraits::enable_if<hasTest2ArgInt<TYPE, int, int>::value, int>::type
    __Test2WithStaticMemberArgInt(int value)
    {
        return TYPE::Test2(value);
    }
    template < typename TYPE>
    inline typename Core::TypeTraits::enable_if<!hasTest2ArgInt<TYPE, int, int>::value, int>::type
    __Test2WithStaticMemberArgInt(int)
    {
        return 0;
    }

    struct TestWithConstMember {

        bool Test3() const {
            std::cout << "bool TestWithConstMember::Test3() const" << std::endl;
            return true;
        }
        string Test3(string value) {
            string str = "string TestWithConstMember::Test3() = " + value;
            std::cout << str << std::endl;
            return str;
        }
        string Test3(string value) const {
            string str = "string TestWithConstMember::Test3() const = " + value;
            std::cout << str << std::endl;
            return str;
        }
        int Test3(int value) const {
            std::cout << "void TestWithConstMember::Test3() = " << value << std::endl;
            return value;
        }
    };
    struct TestWithoutConstMember {
        bool Test3() {
            std::cout << "bool TestWithoutConstMember::Test3() const" << std::endl;
            return true;
        }
        int Test3(int value) {
            std::cout << "void TestWithoutConstMember::Test3() = " << value << std::endl;
            return value;
        }
    };

    IS_MEMBER_AVAILABLE(Test3, hasTest3ConstNoArg);
    template <typename TYPE>
    inline typename Core::TypeTraits::enable_if<hasTest3ConstNoArg<TYPE, bool>::value, bool>::type
    __Test3WithConstMemberNonConstType(TYPE& t)
    {
        return t.Test3();
    }
    template < typename TYPE>
    inline typename Core::TypeTraits::enable_if<!hasTest3ConstNoArg<TYPE, bool>::value, bool>::type
    __Test3WithConstMemberNonConstType(TYPE&)
    {
        return false;
    }

    IS_MEMBER_AVAILABLE(Test3, hasTest3ConstTypeNoArg);
    template <typename TYPE>
    inline typename Core::TypeTraits::enable_if<hasTest3ConstTypeNoArg<TYPE, bool>::value, bool>::type
    __Test3WithConstMember(const TYPE& t)
    {
        return t.Test3();
    }
    template < typename TYPE>
    inline typename Core::TypeTraits::enable_if<!hasTest3ConstTypeNoArg<TYPE, bool>::value, bool>::type
    __Test3WithConstMember(const TYPE&)
    {
        return false;
    }
    IS_MEMBER_AVAILABLE(Test3, hasTest3NonConstArgString);
    template <typename TYPE>
    inline typename Core::TypeTraits::enable_if<hasTest3NonConstArgString<TYPE, string, string>::value, string>::type
    __Test3WithNonConstMember(TYPE& t, string str)
    {
        return t.Test3(str);
    }
    template < typename TYPE>
    inline typename Core::TypeTraits::enable_if<!hasTest3NonConstArgString<TYPE, string, string>::value, string>::type
    __Test3WithNonConstMember(TYPE&, string)
    {
        return "method not available";
    }

    IS_MEMBER_AVAILABLE(Test3, hasTest3ConstArgString);
    template <typename TYPE>
    inline typename Core::TypeTraits::enable_if<hasTest3ConstArgString<TYPE, string, string>::value, string>::type
    __Test3WithNonConstMember(const TYPE& t, string str)
    {
        return t.Test3(str);
    }
    template < typename TYPE>
    inline typename Core::TypeTraits::enable_if<!hasTest3ConstArgString<TYPE, string, string>::value, string>::type
    __Test3WithNonConstMember(const TYPE&, string)
    {
        return "method not available";
    }

    IS_MEMBER_AVAILABLE(Test3, hasTest3ConstArg);
    template <typename TYPE>
    inline typename Core::TypeTraits::enable_if<hasTest3ConstArg<TYPE, int, int>::value, int>::type
    __Test3WithConstMember(const TYPE& t, const int value)
    {
        return t.Test3(value);
    }
    template < typename TYPE>
    inline typename Core::TypeTraits::enable_if<!hasTest3ConstArg<TYPE, int, int>::value, int>::type
    __Test3WithConstMember(const TYPE&, const int)
    {
        return 0;
    }
    IS_MEMBER_AVAILABLE(Test3, hasTest3NoArg);
    template <typename TYPE>
    inline typename Core::TypeTraits::enable_if<hasTest3NoArg<const TYPE, bool>::value, bool>::type
    __Test3WithConstType(const TYPE& t)
    {
        return t.Test3();
    }
    template < typename TYPE>
    inline typename Core::TypeTraits::enable_if<!hasTest3NoArg<const TYPE, bool>::value, bool>::type
    __Test3WithConstType(const TYPE&)
    {
        return false;
    }
    IS_MEMBER_AVAILABLE(Test3, hasTest3Arg);
    template <typename TYPE>
    inline typename Core::TypeTraits::enable_if<hasTest3Arg<const TYPE, int, int>::value, int>::type
    __Test3WithConstTypeArgInt(const TYPE& t, int value)
    {
        return t.Test3(value);
    }
    template < typename TYPE>
    inline typename Core::TypeTraits::enable_if<!hasTest3Arg<const TYPE, int, int>::value, int>::type
    __Test3WithConstTypeArgInt(const TYPE&, int)
    {
        return 0;
    }

    struct TestBase {
    public:
        bool Test4(double value) {
            std::cout << "bool TestBase::Test4() const = " << value << std::endl;
            return true;
        }
        bool Test4(string str) const {
            std::cout << "bool TestBase::Test4() const = " << str << std::endl;
            return true;
        }
        string Test6(string value) const {
            string str = "string TestBase::Test6() const = " + value;
            std::cout << str << std::endl;
            return str;
        }
    protected:
        string Test4(int value) {
            string str = "string TestBase::Test4() = " + std::to_string(value);
            std::cout << str << std::endl;
            return str;
        }
        string Test4(int value) const {
            string str = "string TestBase::Test4() const = " + std::to_string(value);
            std::cout << str << std::endl;
            return str;
        }
    private:
        string Test4(string value, bool b) const {
            string str = "string TestBase::Test4() const = " + value + "bool = " + (b ? "true" : "false");
            std::cout << str << std::endl;
            return str;
        }
    };
    struct TestDerived : public TestBase {
        string Test5(string value) const {
            string str = "string TestDerived::Test5() const = " + value;
            std::cout << str << std::endl;
            return str;
        }
        string Test6(string value) const {
            string str = "string TestDerived::Test6() const = " + value;
            std::cout << str << std::endl;
            return str;
        }
    };

    template <typename MIX>
    struct TestInheritance : public MIX {

        IS_MEMBER_AVAILABLE_INHERITANCE_TREE(Test4, hasTest4TypeArgDouble);
        template <typename TYPE = MIX>
        inline typename Core::TypeTraits::enable_if<hasTest4TypeArgDouble<TYPE, bool, double>::value, bool>::type
        __Test4WithMemberArgDouble(double d)
        {
            return TYPE::Test4(d);
        }
        template <typename TYPE = MIX>
        inline typename Core::TypeTraits::enable_if<!hasTest4TypeArgDouble<TYPE, bool, double>::value, bool>::type
        __Test4WithMemberArgDouble(double)
        {
            return false;
        }
        IS_MEMBER_AVAILABLE_INHERITANCE_TREE(Test4, hasTest4ConstTypeArgString);
        template <typename TYPE = MIX>
        inline typename Core::TypeTraits::enable_if<hasTest4ConstTypeArgString<const TYPE, bool, string>::value, bool>::type
        __Test4WithConstMemberArgString(string str) const
        {
            return TYPE::Test4(str);
        }
        template <typename TYPE = MIX>
        inline typename Core::TypeTraits::enable_if<!hasTest4ConstTypeArgString<const TYPE, bool, string>::value, bool>::type
        __Test4WithConstMemberArgString(string) const
        {
            return false;
        }
        IS_MEMBER_AVAILABLE_INHERITANCE_TREE(Test4, hasTest4ArgInt);
        template <typename TYPE = MIX>
        inline typename Core::TypeTraits::enable_if<hasTest4ArgInt<TYPE, string, int>::value, string>::type
        __Test4WithMemberArgInt(int value)
        {
            return TYPE::Test4(value);
        }
        template <typename TYPE = MIX>
        inline typename Core::TypeTraits::enable_if<!hasTest4ArgInt<TYPE, string, int>::value, string>::type
        __Test4WithMemberArgInt(int)
        {
            return 0;
        }
        IS_MEMBER_AVAILABLE_INHERITANCE_TREE(Test4, hasTest4ConstArgInt);
        template <typename TYPE = MIX>
        inline typename Core::TypeTraits::enable_if<hasTest4ConstArgInt<const TYPE, string, int>::value, string>::type
        __Test4WithMemberArgInt(int value) const
        {
            return TYPE::Test4(value);
        }
        template <typename TYPE = MIX>
        inline typename Core::TypeTraits::enable_if<!hasTest4ConstArgInt<const TYPE, string, int>::value, string>::type
        __Test4WithMemberArgInt(int) const
        {
            return 0;
        }
        IS_MEMBER_AVAILABLE_INHERITANCE_TREE(Test4, hasTest4ConstArgString);
        template <typename TYPE = MIX>
        inline typename Core::TypeTraits::enable_if<hasTest4ConstArgString<const TYPE, string, string, bool>::value, string>::type
        __Test4WithConstMemberArgStringAndBool(string str, bool b) const
        {
            return TYPE::Test4(str, b);
        }
        template <typename TYPE = MIX>
        inline typename Core::TypeTraits::enable_if<!hasTest4ConstArgString<const TYPE, string, string, bool>::value, string>::type
        __Test4WithConstMemberArgStringAndBool(string, bool) const
        {
            return "method not available";
        }
        IS_MEMBER_AVAILABLE(Test5, hasTest5ConstArgString);
        template <typename TYPE = MIX>
        inline typename Core::TypeTraits::enable_if<hasTest5ConstArgString<TYPE, string, string>::value, string>::type
        __Test5WithConstMemberArgString(string str)
        {
            return TYPE::Test5(str);
        }
        template <typename TYPE = MIX>
        inline typename Core::TypeTraits::enable_if<!hasTest5ConstArgString<TYPE, string, string>::value, string>::type
        __Test5WithConstMemberArgString(string)
        {
            return "method not available";
        }
        IS_MEMBER_AVAILABLE_INHERITANCE_TREE(Test5, hasTest5ConstArgStringOnBase);
        template <typename TYPE = MIX>
        inline typename Core::TypeTraits::enable_if<hasTest5ConstArgStringOnBase<TYPE, string, string>::value, string>::type
        __Test5WithConstMemberArgStringOnBase(string str)
        {
            return TYPE::Test5(str);
        }
        template <typename TYPE = MIX>
        inline typename Core::TypeTraits::enable_if<!hasTest5ConstArgStringOnBase<TYPE, string, string>::value, string>::type
        __Test5WithConstMemberArgStringOnBase(string)
        {
            return "method not available";
        }
        IS_MEMBER_AVAILABLE_INHERITANCE_TREE(Test6, hasTest6ConstArgStringOnBoth);
        template <typename TYPE = MIX>
        inline typename Core::TypeTraits::enable_if<hasTest6ConstArgStringOnBoth<TYPE, string, string>::value, string>::type
        __Test6WithConstMemberArgStringOnBoth(string str)
        {
            return TYPE::Test6(str);
        }
        template <typename TYPE = MIX>
        inline typename Core::TypeTraits::enable_if<!hasTest6ConstArgStringOnBoth<TYPE, string, string>::value, string>::type
        __Test6WithConstMemberArgStringOnBoth(string)
        {
            return "method not available";
        }

    };
    struct TestTemplateMember {

        string Test7() {
            string str = "void TestTemplateMember::Test6()";
            std::cout << str << std::endl;
            return str;
        }

        template<typename P>
        string Test7() {
            string str = "void TestTemplateMember::Test6<P>()";
            std::cout << str << std::endl;
            return str;
        }
    };

    IS_TEMPLATE_MEMBER_AVAILABLE(Test7, hasTest7Template);

    template <typename TYPE, typename T>
    inline typename Core::TypeTraits::enable_if<hasTest7Template<TYPE, T, string>::value, string>::type
    __Test7Template(TYPE& t)
    {
        return t.template Test7<T>();
    }
    template <typename TYPE, typename T>
    inline typename Core::TypeTraits::enable_if<!hasTest7Template<TYPE, T, string>::value, string>::type
    __Test7Template(TYPE&)
    {
        return "method not available";
    }

    struct TestNonTemplateMember {

        string Test8() {
            string str = "void TestNonTemplateMember::Test7()";
            std::cout << str << std::endl;
            return str;
        }
    };

    IS_TEMPLATE_MEMBER_AVAILABLE(Test8, hasTest8WithoutTemplate);
    template <typename TYPE, typename T>
    inline typename Core::TypeTraits::enable_if<hasTest8WithoutTemplate<TYPE, T, string>::value, string>::type
    __Test8WithoutTemplate(TYPE& t)
    {
        return t.Test8();
    }
    template <typename TYPE, typename T>
    inline typename Core::TypeTraits::enable_if<!hasTest8WithoutTemplate<TYPE, T, string>::value, string>::type
    __Test8WithoutTemplate(TYPE&)
    {
        return "method not available";
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
        EXPECT_STREQ((__TestWithMember(withoutMember)).c_str(), "method not available");
        EXPECT_STREQ((__Test1WithMemberHasArg(withoutMember, 1)).c_str(), "method not available");
    }
    TEST(IsMemberAvailable, TestWithStaticMember)
    {
        TestWithStaticMember withStaticMember;
        EXPECT_STREQ((__TestWithMember(withStaticMember)).c_str(), "static string TestWithStaticMember::Test1()");
        EXPECT_STREQ((__Test1WithMemberHasArg(withStaticMember, 2)).c_str(), "static string TestWithStaticMember::Test1(int) = 2");
    }
    TEST(StaticMemberCheck, TestWithStaticMember)
    {
        EXPECT_STREQ((__Test1WithStaticMember<TestWithStaticMember>()).c_str(), "static string TestWithStaticMember::Test1()");
        EXPECT_STREQ((__Test1WithStaticMemberHasArg<TestWithStaticMember>(3)).c_str(), "static string TestWithStaticMember::Test1(int) = 3");
    }
    TEST(StaticMemberCheck, TestWithOutStaticMember)
    {
        EXPECT_STREQ((__Test1WithStaticMember<TestWithOutStaticMember>()).c_str(), "method not available");
        EXPECT_STREQ((__Test1WithStaticMemberHasArg<TestWithOutStaticMember>(3)).c_str(), "method not available");
    }
    TEST(NonStaticAndStaticMemberCheck, TestWithNonStaticAndStaticMember)
    {
        bool value = false;
        TestWithNonStaticAndStaticMember withNonStaticAndStaticMember;
        __Test2WithMemberHasArgBool(withNonStaticAndStaticMember, value);
        EXPECT_EQ(value,  true);
        string str = "Hello";
        EXPECT_STREQ((__Test2WithMemberHasArgString(withNonStaticAndStaticMember, str)).c_str(), "TestWithNonStaticAndStaticMember: Test2(str) Hello");

        value = false;
        __Test2WithStaticMemberArgBool<TestWithNonStaticAndStaticMember>(value);
        EXPECT_EQ(value, false);
        EXPECT_EQ((__Test2WithStaticMemberArgInt<TestWithNonStaticAndStaticMember>(5)), 5);

        value = __Test2WithStaticMember<TestWithNonStaticAndStaticMember>();
        EXPECT_EQ(value, true);
    }
    TEST(StaticAndNonStaticMemberCheck, TestWithStaticAndNonStaticMember)
    {
        bool value = false;
        TestWithStaticAndNonStaticMember withStaticAndNonStaticMember;
        __Test2WithMemberHasArgBool(withStaticAndNonStaticMember, value);
        EXPECT_EQ(value,  true);
        string str = "Hello";
        EXPECT_STREQ((__Test2WithMemberHasArgString(withStaticAndNonStaticMember, str)).c_str(), "static TestWithStaticAndNonStaticMember: Test2(str) Hello");

        value = false;
        __Test2WithStaticMemberArgBool<TestWithStaticAndNonStaticMember>(value);
        EXPECT_EQ(value, false);
        EXPECT_EQ((__Test2WithStaticMemberArgInt<TestWithStaticAndNonStaticMember>(6)), 0);

        value = __Test2WithStaticMember<TestWithStaticAndNonStaticMember>();
        EXPECT_EQ(value, false);
    }
    TEST(ConstMemberCheck, TestWithConstMemberNonConstType)
    {
        // Calling a const method for non const TYPE with non const object
        TestWithConstMember withConstMember;
        EXPECT_EQ(__Test3WithConstMemberNonConstType(withConstMember), true);
    }
    TEST(ConstMemberCheck, TestWithConstMemberConstType)
    {
        // Calling a const method for const TYPE with non const object
        TestWithConstMember withConstMember;
        EXPECT_EQ(__Test3WithConstMember(withConstMember), true);
        int value = 6;
        EXPECT_EQ(__Test3WithConstMember(withConstMember, value), 6);
    }
    TEST(ConstMemberCheck, TestWithBothConstAndNonConstOverload)
    {
        // Calling a const method for non const TYPE with const object
        const TestWithConstMember withBothConstAndNonConstOverloadedMember;
        EXPECT_STREQ(__Test3WithNonConstMember(withBothConstAndNonConstOverloadedMember, "Hai").c_str(), "string TestWithConstMember::Test3() const = Hai"); // now only const version taken into account
    }
    TEST(ConstMemberCheck, TestWithNonConstMemberOverload)
    {
        // Calling a non const method for non const TYPE with non const object
        TestWithConstMember withBothConstAndNonConstOverloadedMember;
        EXPECT_STREQ(__Test3WithNonConstMember(withBothConstAndNonConstOverloadedMember, "Hai").c_str(), "string TestWithConstMember::Test3() = Hai"); // It call non const version
    }
    TEST(ConstMemberCheck, TestWithConstClass)
    {
        // const TYPE with const methods using const object
        const TestWithConstMember withConstMember;
        EXPECT_EQ(__Test3WithConstType(withConstMember), true);
        EXPECT_EQ(__Test3WithConstTypeArgInt(withConstMember, 6), 6);
    }
    TEST(ConstMemberCheck, TestWithoutConstClassAndWithConstMember)
    {
        // const TYPE with const methods using non const object
        TestWithConstMember withConstMember;
        EXPECT_EQ(__Test3WithConstType(withConstMember), true);
        EXPECT_EQ(__Test3WithConstTypeArgInt(withConstMember, 7), 7);
    }
    TEST(ConstMemberCheck, TestWithoutAnyConstMember)
    {
        // const TYPE will ignore non const member methods for const object
        const TestWithoutConstMember withoutConstMember;
        EXPECT_EQ(__Test3WithConstType(withoutConstMember), false);
        EXPECT_EQ(__Test3WithConstTypeArgInt(withoutConstMember, 8), 0);
    }
    TEST(ConstMemberCheck, TestWithoutConstClassAndWithoutConstMember)
    {
        // const TYPE will ignore non const member methods for non const object
        TestWithoutConstMember withoutConstMember;
        EXPECT_EQ(__Test3WithConstType(withoutConstMember), false);
        EXPECT_EQ(__Test3WithConstTypeArgInt(withoutConstMember, 9), 0);
    }
    TEST(InheritedMemberCheck, TestPublicMethod)
    {
        // Call non const public method using base class object
        TestInheritance<TestBase> base;
        EXPECT_EQ(base.__Test4WithMemberArgDouble(1.1), true);
        // Call non const public method using derived class object
        TestInheritance<TestDerived> derived;
        EXPECT_EQ(derived.__Test4WithMemberArgDouble(2.2), true);
    }
    TEST(InheritedMemberCheck, TestPublicConstMethod)
    {
        // Call const public method using base class object
        TestInheritance<TestBase> base;
        EXPECT_EQ(base.__Test4WithConstMemberArgString("Base"), true);
        // Call const public method using derived class object
        TestInheritance<TestDerived> derived;
        EXPECT_EQ(derived.__Test4WithConstMemberArgString("Derived"), true);
    }
    TEST(InheritedMemberCheck, TestPublicConstMethodWithConstObject)
    {
        // Call const public method using base class const object
        const TestInheritance<TestBase> base;
        EXPECT_EQ(base.__Test4WithConstMemberArgString(string("Base")), true);
        // Call const public method using derived class const object
        const TestInheritance<TestDerived> derived;
        EXPECT_EQ(derived.__Test4WithConstMemberArgString("Derived"), true);
    }
    TEST(InheritedMemberCheck, TestProtectedMethod)
    {
        // Call non const protected method using base class object
        TestInheritance<TestBase> base;
        EXPECT_STREQ(base.__Test4WithMemberArgInt(10).c_str(), "string TestBase::Test4() = 10");
        // Call non const protected method using derived class object
        TestInheritance<TestDerived> derived;
        EXPECT_STREQ(derived.__Test4WithMemberArgInt(15).c_str(), "string TestBase::Test4() = 15");
    }
    TEST(InheritedMemberCheck, TestProtectedConstMethod)
    {
        // Call const protected method using base class const object
        const TestInheritance<TestBase> base;
        EXPECT_STREQ(base.__Test4WithMemberArgInt(20).c_str(), "string TestBase::Test4() const = 20");
        // Call const protected method using derived class const object
        const TestInheritance<TestDerived> derived;
        EXPECT_STREQ(derived.__Test4WithMemberArgInt(25).c_str(), "string TestBase::Test4() const = 25");
    }
    TEST(InheritedMemberCheck, TestPrivateMethodOnBase)
    {
        TestInheritance<TestBase> base;
        // Private method is not accessible for base class object
        EXPECT_STREQ((base.__Test4WithConstMemberArgStringAndBool("Hello", true)).c_str(), "method not available");
        TestInheritance<TestDerived> derived;
        // Private method is not accessible for derived class object as well
        EXPECT_STREQ((derived.__Test4WithConstMemberArgStringAndBool("Hello", true)).c_str(), "method not available");
    }
    TEST(InheritedMemberCheck, TestPrivateMethodOnBaseWithConstObject)
    {
        const TestInheritance<TestBase> base;
        // Private method is not accessible for const base class object case as well
        EXPECT_STREQ((base.__Test4WithConstMemberArgStringAndBool("Hello", true)).c_str(), "method not available");
        TestInheritance<TestDerived> derived;
        // Private method is not accessible for const derived class object case as well
        EXPECT_STREQ((derived.__Test4WithConstMemberArgStringAndBool("Hello", true)).c_str(), "method not available");
    }
    TEST(InheritedMemberCheck, TestPrivateMethodForDerived)
    {
        TestInheritance<TestDerived> derived;
        // Private method is not accessible for derived object case also, since it is not available for derived class
        EXPECT_STREQ((derived.__Test4WithConstMemberArgStringAndBool("Hai", true)).c_str(), "method not available"); 
        // Try public method here to validate derived functions are available
        EXPECT_STREQ((derived.__Test5WithConstMemberArgString("Hai")).c_str(), "string TestDerived::Test5() const = Hai");
    }
    TEST(InheritedMemberCheck, TestPublicConstMethodOnBoth)
    {
        // Use non const object to call Test6 const method
        // And check which method get invoked if base and derived has has method with same name.
        // Derived class method should get invoked as per C++ rule
        TestInheritance<TestDerived> derived;
        EXPECT_STREQ((derived.__Test6WithConstMemberArgStringOnBoth(" invoked derived version as per C++ rule")).c_str(), "string TestDerived::Test6() const =  invoked derived version as per C++ rule");
    }
    TEST(TemplateCheck, TestTemplateMethod)
    {
        TestTemplateMember templateMember;
        EXPECT_STREQ((__Test7Template<TestTemplateMember, int>(templateMember)).c_str(), "void TestTemplateMember::Test6<P>()");
    }
    TEST(TemplateCheck, TestNonTemplateMethod)
    {
        TestNonTemplateMember nonTemplateMember;
        EXPECT_STREQ((__Test8WithoutTemplate<TestNonTemplateMember, int>(nonTemplateMember)).c_str(), "method not available");
    }
}
}
