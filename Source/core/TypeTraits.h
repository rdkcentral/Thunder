 /*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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
 
#ifndef __DERIVEDCHECK_H
#define __DERIVEDCHECK_H

#include "Portability.h"

namespace WPEFramework {

template <bool PARAMETER>
using EnableIfParameter = typename std::enable_if<PARAMETER, int>::type;

namespace Core {
    namespace TypeTraits {
        template <unsigned Idx, typename... T>
        struct pick {
            typedef void result;
        };

        template <typename T, typename... TRest>
        struct pick<0U, T, TRest...> {
            typedef T result;
        };

        template <unsigned Idx, typename T, typename... TRest>
        struct pick<Idx, T, TRest...> {
            typedef typename pick<Idx - 1, TRest...>::result result;
        };

        template <typename Func>
        struct func_traits;

        template <typename TObj, typename R, typename... TArgs>
        struct func_traits<R (TObj::*)(TArgs...)> {
            typedef R result_type;

            typedef TObj classtype;

            template <unsigned Idx>
            struct argument {
                typedef typename pick<Idx, TArgs...>::result type;
            };

			enum { Arguments = sizeof...(TArgs) };
        };

        template <typename TObj, typename R, typename... TArgs>
        struct func_traits<R (TObj::*)(TArgs...) const> {
            typedef R result_type;

            typedef TObj classtype;

            template <unsigned Idx>
            struct argument {
                typedef typename pick<Idx, TArgs...>::result type;
            };

            enum { Arguments = sizeof...(TArgs) };
        };

        template <typename R, typename... TArgs>
        struct func_traits<R (*)(TArgs...)> {
            typedef R result_type;

            typedef void classtype;
            template <unsigned Idx>
            struct argument {
                typedef typename pick<Idx, TArgs...>::result type;
            };

			enum { Arguments = sizeof...(TArgs) };
        };

        template <typename SCALAR>
        struct sign {
            enum { Signed = 1 };
        };
        template <>
        struct sign<unsigned char> {
            enum { Signed = 0 };
        };
        template <>
        struct sign<unsigned short> {
            enum { Signed = 0 };
        };
        template <>
        struct sign<unsigned int> {
            enum { Signed = 0 };
        };
        template <>
        struct sign<unsigned long> {
            enum { Signed = 0 };
        };
        template <>
        struct sign<unsigned long long> {
            enum { Signed = 0 };
        };

        typedef struct {
            char c[2];
        } tester_t;

        template <class T>
        struct inherits_checker {
            static tester_t f(...);
            static char f(T*);
        };

        template <class T1, class T2>
        struct is_same {
            enum {
                value = 0
            };
        };

        template <class T>
        struct is_same<T, T> {
            enum {
                value = 1
            };
        };

        template <class T1, class T2>
        struct inherits {
            enum {
                value = sizeof(char) == sizeof(inherits_checker<T1>::f((T2*)0)) && !is_same<T1, T2>::value
            };
        };

        template <class T1, class T2>
        struct same_or_inherits {
            enum {
                value = sizeof(char) == sizeof(inherits_checker<T1>::f((T2*)0))
            };
        };

#define HAS_MEMBER(func, name)                                      \
    template <typename T, typename Sign>                            \
    struct name {                                                   \
        typedef char yes[1];                                        \
        typedef char no[2];                                         \
        template <typename U, U>                                    \
        struct type_check;                                          \
        template <typename _1>                                      \
        static yes& chk(type_check<Sign, &_1::func>*);              \
        template <typename>                                         \
        static no& chk(...);                                        \
        static bool const value = sizeof(chk<T>(0)) == sizeof(yes); \
    }

#define HAS_STATIC_MEMBER(func, name)                               \
    template <typename T, typename Sign>                            \
    struct name {                                                   \
        typedef char yes[1];                                        \
        typedef char no[2];                                         \
        template <typename U, U>                                    \
        struct type_check;                                          \
        template <typename _1>                                      \
        static yes& chk(decltype(&_1::func));                       \
        template <typename>                                         \
        static no& chk(...);                                        \
        static bool const value = sizeof(chk<T>(0)) == sizeof(yes); \
    }

#define HAS_TEMPLATE_MEMBER(func, parameter, name)                  \
    template <typename T, typename Sign>                            \
    struct name {                                                   \
        typedef char yes[1];                                        \
        typedef char no[2];                                         \
        template <typename U, U>                                    \
        struct type_check;                                          \
        template <typename _1>                                      \
        static yes& chk(type_check<Sign, &_1::func<parameter>>*);   \
        template <typename>                                         \
        static no& chk(...);                                        \
        static bool const value = sizeof(chk<T>(0)) == sizeof(yes); \
    };

        template <bool C, typename T = void>
        struct enable_if {
            typedef T type;
        };

        template <typename T>
        struct enable_if<false, T> {
        };
    }
}
} // namespace Core::TypeTraits

#endif // __TYPETRAITS_H
