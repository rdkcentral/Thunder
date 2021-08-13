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

        template< bool B, class T = void>
        using enable_if = std::enable_if<B, T>;

        template<class T1, class T2>
        using is_same = std::is_same<T1,T2>;

        template <class T1, class T2>
        using inherits = std::is_base_of<T1, T2>;

        template <class T1, class T2>
        struct same_or_inherits {
            constexpr static bool value = is_same<T1, T2>::value || inherits<T1, T2>::value;
        };

        template <class T>
        struct sign {
            constexpr static bool Signed = std::is_signed<T>::value;
        };


// HPL todo this one can be used with overloads, could not get that to work with the HAS_MEMBER
#define HAS_MEMBER_NAME(func, name)                                 \
    template <typename T, typename... Args>                                           \
    struct name {                                                   \
        typedef char yes[1];                                        \
        typedef char no[2];                                         \
         template <typename U,                                      \
              typename = decltype( std::declval<U>().func(std::declval<Args>()...) )> \
        static yes& chk(int);                                       \
        template <typename U>                                       \
        static no& chk(...);                                        \
        static bool const value = sizeof(chk<T>(0)) == sizeof(yes); \
    }

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
    template <typename T>                                           \
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

    }
}
} // namespace Core::TypeTraits

#endif // __TYPETRAITS_H
