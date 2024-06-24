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
#include <functional>

namespace Thunder {

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
        struct func_traits {
            typedef std::nullptr_t result_type;

            typedef std::nullptr_t classtype;

            template <unsigned Idx>
            struct argument {
                typedef std::nullptr_t type;
            };

            enum { Arguments = -1 };
        };

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

        template <typename R, typename... TArgs>
        struct func_traits<std::function<R(TArgs...)>> {
            typedef R result_type;

            typedef void classtype;
            template <unsigned Idx>
            struct argument {
                typedef typename pick<Idx, TArgs...>::result type;
            };

            enum { Arguments = sizeof...(TArgs) };
        };

        template <typename R, typename... TArgs>
        struct func_traits<const std::function<R(TArgs...)>> {
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

// func: name of function that should be present
// name: name of the struct that later can be used to override using SFINEA
// T:    type on which it should be chekced if func is available. If only const versions of func need to be taken into account specify it as const T
// R:    expected return type for func
// Args: arguments that func should have, note it will also work if overrides of Func are available on T. Note: passing Args&&... itself is also allowed here to allow for variable parameters
#define IS_MEMBER_AVAILABLE(func, name)                                                         \
    template <typename T, typename R, typename... Args>                                         \
    struct name {                                                                               \
        typedef char yes[1];                                                                    \
        typedef char no[2];                                                                     \
        template <typename U,                                                                   \
                  typename RR = decltype(std::declval<U>().func(std::declval<Args>()...)),      \
                  typename Z = typename std::enable_if<std::is_same<R, RR>::value>::type>       \
        static yes& chk(int);                                                                   \
        template <typename U>                                                                   \
        static no& chk(...);                                                                    \
        static bool const value = sizeof(chk<T>(0)) == sizeof(yes);                             \
    }

// func: name of function that should be present
// name: name of the struct that later can be used to override using SFINEA
// T:    type on which it should be chekced if func is available. If only const versions of func need to be taken into account specify it as const T
// R:    expected return type for func
// Args: arguments that func should have, note it will also work if overrides of Func are available on T. Note: passing Args&&... itself is also allowed here to allow for variable parameters
// use this macro instead of IS_MEMBER_AVAILABLE when the return type of the function does not have to be exactly the same but instead it can be convertible to the expected one
#define IS_MEMBER_AVAILABLE_CONVERTIBLE(func, name)                                             \
    template <typename T, typename R, typename... Args>                                         \
    struct name {                                                                               \
        typedef char yes[1];                                                                    \
        typedef char no[2];                                                                     \
        template <typename U,                                                                   \
                  typename RR = decltype(std::declval<U>().func(std::declval<Args>()...)),      \
                  typename Z = typename std::enable_if<std::is_convertible<RR, R>::value>::type>\
        static yes& chk(int);                                                                   \
        template <typename U>                                                                   \
        static no& chk(...);                                                                    \
        static bool const value = sizeof(chk<T>(0)) == sizeof(yes);                             \
    }

// func: name of function that should be present in the inheritance tree and accessable (public or protected)
// name: name of the struct that later can be used to override using SFINEA
// T:    type on which it should be chekced if func is available in the Inheritance tree. If only const versions of func need to be taken into account specify it as const T
// R:    expected return type for func
// Args: arguments that func should have, note it will also work if overrides of Func are available on T. Note: passing Args&&... itself is also allowed here to allow for variable parameters
#define IS_MEMBER_AVAILABLE_INHERITANCE_TREE(func, name)                                                                                             \
    template <bool, typename TT>                                                                                                                     \
    struct name##_IsMemberAvailableCheck : public TT {                                                                                               \
      using type = TT;                                                                                                                               \
      template <typename TTT, typename... Args2>                                                                                                     \
      auto Verify() -> decltype( (TTT::func(std::declval<Args2>()...)));                                                                             \
    };                                                                                                                                               \
    template <typename TT>                                                                                                                           \
    struct name##_IsMemberAvailableCheck<true, TT> : public TT {                                                                                     \
      using type = const TT;                                                                                                                         \
      template <typename TTT, typename... Args2>                                                                                                     \
      auto Verify() const -> decltype( (TTT::func(std::declval<Args2>()...)));                                                                       \
    };                                                                                                                                               \
    template <typename T, typename R, typename... Args>                                                                                              \
    struct name {                                                                                                                                    \
        typedef char yes[1];                                                                                                                         \
        typedef char no[2];                                                                                                                          \
        template <typename U,                                                                                                                        \
                  typename RR = decltype(std::declval<name##_IsMemberAvailableCheck<std::is_const<U>::value, U>>().template Verify<U, Args...>()),   \
                  typename Z = typename std::enable_if<std::is_same<R, RR>::value>::type>                                                            \
        static yes& chk(int);                                                                                                                        \
        template <typename U>                                                                                                                        \
        static no& chk(...);                                                                                                                         \
        static bool const value = sizeof(chk<T>(0)) == sizeof(yes);                                                                                  \
    }


// func: name of static function that should be present
// name: name of the struct that later can be used to override using SFINEA
// T:    type on which it should be chekced if func is available
// R:    expected return type for func
// Args: arguments that func should have, note it will also work if overrides of Func are available on T. Note: passing Args&&... itself is also allowed here to allow for variable parameters
// (Note: theoritcally also IS_MEMBER_AVAILABLE could be used for statics (and IS_MEMBER_AVAILABLE will actually find statics a match) but we made the differentiation so it is possibe to have non 
//        none statics not taken into account when checking if func is available
#define IS_STATIC_MEMBER_AVAILABLE(func, name)                                       \
    template <typename T, typename R, typename... Args>                              \
    struct name {                                                                    \
        typedef char yes[1];                                                         \
        typedef char no[2];                                                          \
        template <typename U,                                                        \
            typename RR = decltype(U::func(std::declval<Args>()...)),                \
            typename Z = typename std::enable_if<std::is_same<R, RR>::value>::type>  \
        static yes& chk(int);                                                        \
        template <typename U>                                                        \
        static no& chk(...);                                                         \
        static bool const value = sizeof(chk<T>(0)) == sizeof(yes);                  \
    }

// func: name of static function that should be present
// name: name of the struct that later can be used to override using SFINEA
// T:    type on which it should be chekced if func is available. If only const versions of func need to be taken into account specify it as const T
// P:    desired template type for func 
// R:    expected return type for func
// Args: arguments that func should have, note it will also work if overrides of Func are available on T. Note: passing Args&&... itself is also allowed here to allow for variable parameters
#define IS_TEMPLATE_MEMBER_AVAILABLE(func, name)                                        \
    template <typename T, typename P, typename R, typename... Args>                     \
    struct name {                                                                       \
        typedef char yes[1];                                                            \
        typedef char no[2];                                                             \
        template <typename U,                                                           \
            typename RR = decltype(std::declval<U>().template func<P>(std::declval<Args>()...)), \
            typename Z = typename std::enable_if<std::is_same<R, RR>::value>::type>     \
        static yes& chk(int);                                                           \
        template <typename U>                                                           \
        static no& chk(...);                                                            \
        static bool const value = sizeof(chk<T>(0)) == sizeof(yes);                     \
    }

    }
}
} // namespace Core::TypeTraits

#endif // __TYPETRAITS_H
