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
 
#ifndef __PORTABILITY_H
#define __PORTABILITY_H

#ifdef __APPLE__
#if !defined(SOL_IP)
#define SOL_IP IPPROTO_IP /* SOL_IP is not defined on OSX Lion */
#endif /* TARGET_DARWIN && !SOL_IP */
#define CBR_110 110
#define CBR_300 300
#define CBR_600 600
#define CBR_1200 1200
#define CBR_2400 2400
#define CBR_4800 4800
#define CBR_9600 9600
#define CBR_14400 14400
#define CBR_19200 19200
#define CBR_38400 38400
#define CBR_56000 56000
#define CBR_57600 57600
#define CBR_115200 115200
#define CBR_128000 128000
#define CBR_256000 256000

#define B0 0
#define B110 110
#define B300 300
#define B600 600
#define B1200 1200
#define B2400 2400
#define B4800 4800
#define B9600 9600
#define B19200 19200
#define B38400 38400
#define B57600 57600
#define B115200 115200
#define B500000 500000
#define B1000000 1000000
#define B1152000 1152000
#define B1500000 1500000
#define B2000000 2000000
#define B2500000 2500000
#define B3000000 3000000
#define B3500000 3500000
#define B4000000 4000000
#endif

#if defined(WIN32) || defined(_WINDOWS) || defined (__CYGWIN__) || defined(_WIN64)
    #ifdef __GNUC__
        #define EXTERNAL        __attribute__ ((dllimport))
        #define EXTERNAL_EXPORT __attribute__ ((dllexport))
    #else
        #define EXTERNAL        __declspec(dllimport) 
        #define EXTERNAL_EXPORT __declspec(dllexport)
    #endif

    #if defined(CORE_EXPORTS)
    #undef EXTERNAL
    #define EXTERNAL EXTERNAL_EXPORT
    #endif

    #define EXTERNAL_HIDDEN
    #define __WINDOWS__
#else
  #if __GNUC__ >= 4 && !defined(__mips__)
    #define EXTERNAL_HIDDEN __attribute__ ((visibility ("hidden")))
    #define EXTERNAL_EXPORT __attribute__ ((visibility ("default")))
    #define EXTERNAL EXTERNAL_EXPORT        
#else
    #define EXTERNAL
    #define EXTERNAL_HIDDEN
    #define EXTERNAL_EXPORT 
  #endif
#endif

#ifdef __WINDOWS__
    #define DO_PRAGMA(x) __pragma(x)

    #define PUSH_WARNING_ DO_PRAGMA(warning(push))
    #define PUSH_WARNING_ARG_(WARNING) DO_PRAGMA(warning(disable: WARNING))
    #define POP_WARNING_ DO_PRAGMA(warning(pop))

#else
    #define DO_PRAGMA(x) _Pragma(#x)

    #if defined(__clang__)
        #define PUSH_WARNING_ _Pragma("clang diagnostic push")
        #define PUSH_WARNING_ARG_(WARNING) DO_PRAGMA(clang diagnostic ignored #WARNING)
        #define POP_WARNING_ _Pragma("clang diagnostic pop")

    #elif (__GNUC__ >= 4)
        #define PUSH_WARNING_ _Pragma("GCC diagnostic push")
        #define PUSH_WARNING_ARG_(WARNING) DO_PRAGMA(GCC diagnostic ignored WARNING)
        #define POP_WARNING_ _Pragma("GCC diagnostic pop")

    #else
        #define PUSH_WARNING_(WARNING)
        #define PUSH_WARNING_ARG_
        #define POP_WARNING_
    #endif
#endif

#define DEFINE_STRING_1(parameter) #parameter
#define DEFINE_STRING(parameter) DEFINE_STRING_1(parameter)
#define CONCAT_STRINGS_1(ARG1, ARG2)  ARG1##ARG2
#define CONCAT_STRINGS(ARG1, ARG2)  CONCAT_STRINGS_1(ARG1,ARG2)
#define PUSH_WARNING_ROLL_1(WARNING, ...) WARNING
#define PUSH_WARNING_ROLL_2(WARNING, ...) WARNING PUSH_WARNING_ROLL_1(__VA_ARGS__)
#define PUSH_WARNING_ROLL_3(WARNING, ...) WARNING PUSH_WARNING_ROLL_2(__VA_ARGS__)
#define PUSH_WARNING_ROLL_4(WARNING, ...) WARNING PUSH_WARNING_ROLL_3(__VA_ARGS__)
#define PUSH_WARNING_ROLL_5(WARNING, ...) WARNING PUSH_WARNING_ROLL_4(__VA_ARGS__)

// Seems to be a MSVC issue, see: https://stackoverflow.com/questions/9183993/msvc-variadic-macro-expansion
#define PUSH_RETURN_ARG_COUNT(_1_, _2_, _3_, _4_, _5_, _6_, _7_, _8_, count, ...) count
#define PUSH_EXPAND_ARGS(args) PUSH_RETURN_ARG_COUNT args
#define PUSH_COUNT_ARGS(...) PUSH_EXPAND_ARGS((__VA_ARGS__, 8, 7, 6, 5, 4, 3, 2, 1, 0))

#define PUSH_WARNING_ARG(N, ...) CONCAT_STRINGS(PUSH_WARNING_ROLL_, N)(__VA_ARGS__)
#define PUSH_WARNING(...) \
    PUSH_WARNING_ PUSH_WARNING_ARG(PUSH_COUNT_ARGS(__VA_ARGS__), __VA_ARGS__)

#define POP_WARNING() POP_WARNING_

#ifdef __WINDOWS__
// W4 -- Make sure the "conditional expression is constant"
#define DISABLE_WARNING_CONDITIONAL_EXPRESSION_IS_CONSTANT PUSH_WARNING_ARG_(4127)
// W4 -- Nonstandard extension used : zero-sized array in struct/union
#define DISABLE_WARNING_NON_STANDARD_EXTENSION_USED PUSH_WARNING_ARG_(4200)
// W3 -- Make sure the "clas 'xxxx' needs to have dll-interface to be used by clients of class 'xxxx'"
#define DISABLE_WARNING_DLL_INTERFACE_UNAVAILABLE PUSH_WARNING_ARG_(4251)
// W3 -- C++ exception specification ignored except to indicate a function is not __declspec(nothrow)
#define DISABLE_WARNING_EXCEPTION_SPECIFICATION PUSH_WARNING_ARG_(4290)
// W3 -- No matching operator delete found; memory will not be freed if initialization throws an exception
#define DISABLE_WARNING_NO_MATCHING_OPERATOR_DELETE PUSH_WARNING_ARG_(4291)
// W2 -- Conversion : truncation from 'type 1' to 'type 2'
#define DISABLE_WARNING_CONVERSION_TRUNCATION PUSH_WARNING_ARG_(4302)
// W1 -- Variable : pointer truncation from 'type' to 'type'
#define DISABLE_WARNING_POINTER_TRUNCATION PUSH_WARNING_ARG_(4311)
// W1 -- Operation : conversion from 'type1' to 'type2' of greater size
#define DISABLE_WARNING_CONVERSION_TO_GREATERSIZE PUSH_WARNING_ARG_(4312)
// W1 & 4 -- The 'this' pointer : used in base member initializer list
#define DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST PUSH_WARNING_ARG_(4355)
// W3 -- Make sure the "C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc"
#define DISABLE_WARNING_UNWIND_SEMANTICS_NOT_ENABLED PUSH_WARNING_ARG_(4530)
// W1 -- 'class1' : base-class 'class2' is already a base-class of 'class3'.
#define DISABLE_WARNING_MULTPILE_INHERITENCE_OF_BASE_CLASS PUSH_WARNING_ARG_(4584)
// W1 -- Discarding return value of function with 'nodiscard' attribute
#define DISABLE_WARNING_DISCARD_RETURN_VALUE_FOR_NONDISCARD_FUNCTION PUSH_WARNING_ARG_(4834)
// W3 -- Code uses a function, class member, variable, or typedef that's marked deprecated
#define DISABLE_WARNING_DEPRECATED_USE PUSH_WARNING_ARG_(4996)
#define DISABLE_WARNING_MISSING_FIELD_INITIALIZERS
// W3 - 'identifier': unreferenced local variable
#define DISABLE_WARNING_UNUSED_VARIABLES PUSH_WARNING_ARG_(4101)
// W4 - 'identifier': unreferenced formal parameter
#define DISABLE_WARNING_UNUSED_PARAMETERS PUSH_WARNING_ARG_(4100)
// W4 - 'function': unreferenced function with internal linkage has been removed
#define DISABLE_WARNING_UNUSED_FUNCTIONS PUSH_WARNING_ARG_(5242)
#define DISABLE_WARNING_DEPRECATED_COPY
#define DISABLE_WARNING_NON_VIRTUAL_DESTRUCTOR
#define DISABLE_WARNING_UNUSED_RESULT
#define DISABLE_WARNING_TYPE_LIMITS

#else
#define DISABLE_WARNING_CONDITIONAL_EXPRESSION_IS_CONSTANT
#define DISABLE_WARNING_NON_STANDARD_EXTENSION_USED
#define DISABLE_WARNING_DLL_INTERFACE_UNAVAILABLE
#define DISABLE_WARNING_EXCEPTION_SPECIFICATION
#define DISABLE_WARNING_NO_MATCHING_OPERATOR_DELETE
#define DISABLE_WARNING_CONVERSION_TRUNCATION
#define DISABLE_WARNING_POINTER_TRUNCATION
#define DISABLE_WARNING_CONVERSION_TO_GREATERSIZE
#define DISABLE_WARNING_THIS_IN_MEMBER_INITIALIZER_LIST
#define DISABLE_WARNING_MULTPILE_INHERITENCE_OF_BASE_CLASS
#define DISABLE_WARNING_DISCARD_RETURN_VALUE_FOR_NONDISCARD_FUNCTION

#if defined(__clang__) || (__GNUC__ >= 4)
#define DISABLE_WARNING_MISSING_FIELD_INITIALIZERS PUSH_WARNING_ARG_("-Wmissing-field-initializers")
#define DISABLE_WARNING_UNUSED_VARIABLES PUSH_WARNING_ARG_("-Wunused-variable")
#define DISABLE_WARNING_UNUSED_PARAMTERS PUSH_WARNING_ARG_("-Wunused-parameter")
#define DISABLE_WARNING_UNUSED_FUNCTIONS PUSH_WARNING_ARG_("-Wunused-function")
#define DISABLE_WARNING_UNUSED_RESULT PUSH_WARNING_ARG_("-Wunused-result")
#define DISABLE_WARNING_DEPRECATED_USE PUSH_WARNING_ARG_("-Wdeprecated-declarations")
#define DISABLE_WARNING_DEPRECATED_COPY PUSH_WARNING_ARG_("-Wdeprecated-copy")
#define DISABLE_WARNING_NON_VIRTUAL_DESTRUCTOR PUSH_WARNING_ARG_("-Wnon-virtual-dtor")
#define DISABLE_WARNING_TYPE_LIMITS PUSH_WARNING_ARG_("-Wtype-limits")
#endif
#endif

#if defined WIN32 || defined _WINDOWS

PUSH_WARNING( \
    DISABLE_WARNING_EXCEPTION_SPECIFICATION, \
    DISABLE_WARNING_DLL_INTERFACE_UNAVAILABLE, \
    DISABLE_WARNING_UNWIND_SEMANTICS_NOT_ENABLED, \
    DISABLE_WARNING_CONDITIONAL_EXPRESSION_IS_CONSTANT, \
    DISABLE_WARNING_NO_MATCHING_OPERATOR_DELETE)

#ifdef _WIN64
#define __SIZEOF_POINTER__ 8
#else
#ifdef WIN32
#define __SIZEOF_POINTER__ 4
#endif
#endif

#define _WINSOCKAPI_ /* Prevent inclusion of winsock.h in windows.h */
#define WIN32_LEAN_AND_MEAN
#define NOWINRES
//#define NOGDI
#define NOSERVICE
#define NOMCX
#define NOIME
//#include <SDKDDKVer.h>
#include <TCHAR.h>
#include <WinSock2.h>
#include <algorithm>
#include <assert.h>
#include <list>
#include <map>
#include <memory.h>
#include <string>
#include <windows.h>
#include <unordered_map>
#include <atomic>
#include <array>
#include <thread>
#include <stdarg.h> /* va_list, va_start, va_arg, va_end */

#define AF_NETLINK 16
#define AF_PACKET  17

// template class __declspec(dllexport) std::basic_string<char>;

inline void SleepS(const uint32_t time)
{
    ::Sleep(time * 1000);
}
inline void SleepMs(const uint32_t time)
{
    ::Sleep(time);
}

EXTERNAL void SleepUs(const uint32_t time);

#ifdef _UNICODE
typedef std::wstring string;
#endif

#ifndef _UNICODE
typedef std::string string;
#endif

using uint24_t = uint32_t;

#define CBR_110 110
#define CBR_300 300
#define CBR_600 600
#define CBR_1200 1200
#define CBR_2400 2400
#define CBR_4800 4800
#define CBR_9600 9600
#define CBR_14400 14400
#define CBR_19200 19200
#define CBR_38400 38400
#define CBR_56000 56000
#define CBR_57600 57600
#define CBR_115200 115200
#define CBR_128000 128000
#define CBR_256000 256000

#define B0 0
#define B110 CBR_110
#define B300 CBR_300
#define B600 CBR_600
#define B1200 CBR_1200
#define B2400 CBR_2400
#define B4800 CBR_4800
#define B9600 CBR_9600
#define B19200 CBR_19200
#define B38400 CBR_38400
#define B57600 CBR_57600
#define B115200 CBR_115200
#define B500000 500000
#define B1000000 1000000
#define B1152000 1152000
#define B1500000 1500000
#define B2000000 2000000
#define B2500000 2500000
#define B3000000 3000000
#define B3500000 3500000
#define B4000000 4000000

#define CS5 5
#define CS6 6
#define CS7 7
#define CS8 8

#define ALLOCA _malloca

#define KEY_LEFTSHIFT VK_LSHIFT
#define KEY_RIGHTSHIFT VK_RSHIFT
#define KEY_LEFTALT VK_LMENU
#define KEY_RIGHTALT VK_RMENU
#define KEY_LEFTCTRL VK_LCONTROL
#define KEY_RIGHTCTRL VK_RCONTROL
#undef INPUT_MOUSE

// This is an HTTP keyword (VERB) Let's undefine it from windows headers..
#define _CRT_SECURE_NO_WARNINGS 1
#define SOCK_CLOEXEC 0
#undef DELETE
#undef min
#undef max
#undef ERROR_NOT_SUPPORTED
#undef ERROR_HIBERNATED

//#if _MSC_VER >= 1600
//const std::basic_string<char>::size_type std::basic_string<char>::npos = (std::basic_string<char>::size_type) - 1;
//#endif

#define LITTLE_ENDIAN_PLATFORM 1
#undef ERROR
#define __WINDOWS__
#else
#ifndef __LINUX__
#define __LINUX__
#endif
#endif

#ifdef __LINUX__

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <string>
#include <algorithm>
#include <atomic>
#include <array>
#include <map>
#include <unordered_map>
#include <list>
#include <typeinfo>
#include <cmath>
#include <thread>

#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <stdarg.h> /* va_list, va_start, va_arg, va_end */
#include <alloca.h>
#include <cxxabi.h>
#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <getopt.h>
#include <math.h>
#include <poll.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>

#include <sys/ioctl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <arpa/inet.h>

#ifdef __APPLE__
#include <pthread_impl.h>
#define SCHED_BATCH 99
#define MREMAP_MAYMOVE 1
#define KEY_LEFTSHIFT 2
#define KEY_RIGHTSHIFT 3
#define KEY_LEFTALT 4
#define KEY_RIGHTALT 5
#define KEY_LEFTCTRL 6
#define KEY_RIGHTCTRL 7
extern "C" EXTERNAL void* mremap(void* old_address, size_t old_size, size_t new_size, int flags);
int clock_gettime(int, struct timespec*);
#else
#include <linux/input.h>
#include <linux/types.h>
#include <linux/uinput.h>
#include <sys/signalfd.h>
#endif

#define ONESTOPBIT 0
#define TWOSTOPBITS CSTOPB
#define ONE5STOPBITS 3
#define NOPARITY 0
#define EVENPARITY PARENB
#define ODDPARITY (PARENB | PARODD)
#define MARKPARITY  8
#define SPACEPARITY 9

#define INVALID_HANDLE_VALUE -1

#define ESUCCESS 0
#define _Geterrno() errno
#define BYTE unsigned char
#define __POSIX__ 1
#define __UNIX__ 1

#ifdef _UNICODE
#define _T(x) L##x
#define TCHAR wchar_t
#define _tcslen wcslen
#define _tcscmp wcscmp
#define _tcsncmp wcsncmp
#define _tcsicmp wcscasecmp
#define _tcsnicmp wcsncasecmp
#define _tcschr wcschr
#define _tcsrchr wcsrchr
#define _tcsftime wcsftime
#define _stprintf swprintf
#define _tcscpy wcscpy
#define _tcsncpy wcsncpy

#define _tiscntrl iswcntrl
#define _tisprint iswprint
#define _tisspace iswspace
#define _tisblank iswblank
#define _tisgraph iswgraph
#define _tispunct iswpunct
#define _tisalnum iswalnum
#define _tisalpha iswalpha
#define _tisupper iswupper
#define _tislower iswlower
#define _tisdigit iswdigit
#define _tisxdigit iswxdigit
#endif

#ifndef _UNICODE
#define _T(x) x
#define TCHAR char
#define _tcslen strlen
#define _tcscmp strcmp
#define _tcsncmp strncmp
#define _tcsicmp strcasecmp
#define _tcsnicmp strncasecmp
#define _tcschr strchr
#define _tcsrchr strrchr
#define _tcsftime strftime
#define _stprintf sprintf
#define _tcscpy strcpy
#define _tcsncpy strncpy

#define _tiscntrl iscntrl
#define _tisprint isprint
#define _tisspace isspace
#define _tisblank isblank
#define _tisgraph isgraph
#define _tispunct ispunct
#define _tisalnum isalnum
#define _tisalpha isalpha
#define _tisupper isupper
#define _tislower islower
#define _tisdigit isdigit
#define _tisXdigit isxdigit
#endif

#define ALLOCA alloca

extern void EXTERNAL SleepMs(const unsigned int a_Time);
extern void EXTERNAL SleepUs(const unsigned int a_Time);
inline void EXTERNAL SleepS(unsigned int a_Time)
{
    ::SleepMs(a_Time * 1000);
}

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define LITTLE_ENDIAN_PLATFORM 1
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define BIG_ENDIAN_PLATFORM 1
#else
#error "Unknown endianess: please set __BYTE_ORDER__ to proper endianess"
#endif

#endif


#ifdef __GNUC__
#define DEPRECATED __attribute__((deprecated))
#define VARIABLE_IS_NOT_USED __attribute__((unused))
#define WARNING_RESULT_NOT_USED __attribute__((warn_unused_result))
#elif defined(_MSC_VER)
#define DEPRECATED __declspec(deprecated)
#define VARIABLE_IS_NOT_USED
#define WARNING_RESULT_NOT_USED
#else
#define DEPRECATED
#define VARIABLE_IS_NOT_USED
#define WARNING_RESULT_NOT_USED
#endif

#if !defined(NDEBUG)
#if defined(_THUNDER_DEBUG) || !defined(_THUNDER_NDEBUG)
#define __DEBUG__
#ifdef _THUNDER_PRODUCTION
#error "Production and Debug is not a good match. Select Production or Debug, not both !!"
#endif
#endif
#endif

#ifdef __LINUX__
#if !defined(OS_ANDROID) && !defined(OS_NACL) && defined(__GLIBC__) && defined(_THUNDER_CALLSTACK_INFO)
#define THUNDER_BACKTRACE 1
#include <execinfo.h>
#endif
#endif

#include "Config.h"

typedef DEPRECATED unsigned char uint8;
typedef DEPRECATED unsigned short uint16;
typedef DEPRECATED unsigned int uint32;
typedef DEPRECATED unsigned long long uint64;

typedef DEPRECATED signed char sint8;
typedef DEPRECATED signed short sint16;
typedef DEPRECATED signed int sint32;
typedef DEPRECATED signed long long sint64;

#include <cstdint>

#ifndef FALSE
#define FALSE (0)
#endif

#ifndef TRUE
#define TRUE (!FALSE)
#endif

#ifdef __WINDOWS__
#define SYSTEM_SYNC_HANDLE HANDLE
#else
#define SYSTEM_SYNC_HANDLE void*
#endif

#ifndef __WINDOWS__
#define HANDLE int
#endif

template <unsigned int TYPE>
struct TemplateIntToType {
    enum { value = TYPE };
};

extern "C" {

DEPRECATED inline EXTERNAL void* memrcpy(void* _Dst, const void* _Src, size_t _MaxCount)
{
    return (::memmove(_Dst, _Src, _MaxCount));
}

#if defined(__LINUX__)
uint64_t htonll(const uint64_t& value);
uint64_t ntohll(const uint64_t& value);
#endif
}

// ---- Helper types and constants ----
#define _TXT(THETEXT) \
    _T(THETEXT)       \
    , (sizeof(THETEXT) / sizeof(TCHAR)) - 1

#define NUMBER_MAX_BITS(TYPE) (sizeof(TYPE) << 3)
#define NUMBER_MIN_UNSIGNED(TYPE) (static_cast<TYPE>(0))
#define NUMBER_MAX_UNSIGNED(TYPE) (static_cast<TYPE>(~0))
#define NUMBER_MAX_SIGNED(TYPE) (static_cast<TYPE>((((static_cast<TYPE>(1) << (NUMBER_MAX_BITS(TYPE) - 2)) - 1) << 1) + 1))
#define NUMBER_MIN_SIGNED(TYPE) (static_cast<TYPE>(-1 - NUMBER_MAX_SIGNED(TYPE)))

typedef enum {
    BASE_UNKNOWN = 0,
    BASE_OCTAL = 8,
    BASE_DECIMAL = 10,
    BASE_HEXADECIMAL = 16

} NumberBase;

#ifdef __WINDOWS__

#include <TCHAR.h>
#define VARIABLE_IS_NOT_USED

#endif 

#ifdef __LINUX__

#ifdef _UNICODE
#define _T(x) L##x
#define TCHAR wchar_t
#endif

#ifndef _UNICODE
#define _T(x) x
#define TCHAR char
#endif


#endif // __LINUX__

#ifdef _UNICODE
typedef std::wstring string;
#endif

#ifndef _UNICODE
typedef std::string string;
#endif

using uint24_t = uint32_t;

#define STRLEN(STATIC_TEXT) ((sizeof(STATIC_TEXT) / sizeof(TCHAR)) - 1)
#define EMPTY_STRING _T("")

#ifdef __LINUX__
typedef pthread_t ThreadId;
#else
typedef HANDLE ThreadId;
#endif

#define QUOTE(str) #str
#define EXPAND_AND_QUOTE(str) QUOTE(str)

#if !defined(__DEBUG)
#define DEBUG_VARIABLE(X) (void)(X)
#else
#define DEBUG_VARIABLE(x)
#endif

namespace WPEFramework {

namespace Core {

    #if defined(__CORE_INSTANCE_BITS__) && (__CORE_INSTANCE_BITS__ != 0)
    #if __CORE_INSTANCE_BITS__ <= 8
    typedef uint8_t instance_id;
    #elif __CORE_INSTANCE_BITS__ <= 16
    typedef uint16_t instance_id;
    #elif __CORE_INSTANCE_BITS__ <= 32 
    typedef uint32_t instance_id;
    #elif __CORE_INSTANCE_BITS__ <= 64
    typedef uint64_t instance_id;
    #endif
    #else
    #if defined(__SIZEOF_POINTER__) && (__SIZEOF_POINTER__ == 8) 
    typedef uint64_t instance_id;
    #else
    typedef uint32_t instance_id;
    #endif
    #endif

    typedef uint32_t hresult;

    struct callstack_info {
        void*    address;
        string   module;
        string   function;
        uint32_t line;
    };


    inline void* Alignment(size_t alignment, void* incoming)
    {
        const auto basePtr = reinterpret_cast<uintptr_t>(incoming);
        return reinterpret_cast<void*>((basePtr - 1u + alignment) & ~(alignment - 1));
    }

    inline uint8_t* PointerAlign(uint8_t* pointer)
    {
        uintptr_t addr = reinterpret_cast<uintptr_t>(pointer);
        addr = (addr + (sizeof(void*) - 1)) & ~(sizeof(void*) - 1); // Round up to align-byte boundary
        return reinterpret_cast<uint8_t*>(addr);
    }

    inline const uint8_t* PointerAlign(const uint8_t* pointer)
    {
        uintptr_t addr = reinterpret_cast<uintptr_t>(pointer);
        addr = (addr + (sizeof(void*) - 1)) & ~(sizeof(void*) - 1); // Round up to align-byte boundary
        return reinterpret_cast<const uint8_t*>(addr);
    }

#ifdef _UNICODE
    typedef std::wstring string;
#endif

#ifndef _UNICODE
    typedef std::string string;
#endif

    inline void ToUpper(const string& input, string& output)
    {
        // Copy string to output, so we know memory is allocated.
        output = input;

        std::transform(input.begin(), input.end(), output.begin(), ::toupper);
    }

    inline void ToUpper(string& inplace)
    {
        std::transform(inplace.begin(), inplace.end(), inplace.begin(), ::toupper);
    }

    inline void ToLower(const string& input, string& output)
    {
        output = input;

        std::transform(input.begin(), input.end(), output.begin(), ::tolower);
    }

    inline void ToLower(string& inplace)
    {
        std::transform(inplace.begin(), inplace.end(), inplace.begin(), ::tolower);
    }

    string EXTERNAL Format(const TCHAR formatter[], ...);
    void EXTERNAL Format(string& dst, const TCHAR format[], ...);
    void EXTERNAL Format(string& dst, const TCHAR format[], va_list ap);

    const uint32_t infinite = -1;
    static const string emptyString;

    class Void {
    public:
        template <typename... Args>
        inline Void(Args&&...) {}
        inline Void(const Void&) = default;
        inline Void(Void&&) = default;
        inline ~Void() = default;

        inline Void& operator=(const Void&) = default;
    };

    struct EXTERNAL IReferenceCounted {
        virtual ~IReferenceCounted() = default;
        virtual void AddRef() const = 0;
        virtual uint32_t Release() const = 0;
    };

    struct EXTERNAL IUnknown : public IReferenceCounted  {

        enum {
            ID_OFFSET_INTERNAL  = 0x00000000,
            ID_OFFSET_PUBLIC    = 0x00000040,
            ID_OFFSET_CUSTOM    = 0x80000000
        };

        enum { ID = (ID_OFFSET_INTERNAL + 0x0000) };

        ~IUnknown() override = default;

        virtual void* QueryInterface(const uint32_t interfaceNumber) = 0;

        template <typename REQUESTEDINTERFACE>
        REQUESTEDINTERFACE* QueryInterface()
        {
            void* baseInterface(QueryInterface(REQUESTEDINTERFACE::ID));

            if (baseInterface != nullptr) {
                return (reinterpret_cast<REQUESTEDINTERFACE*>(baseInterface));
            }

            return (nullptr);
        }

        template <typename REQUESTEDINTERFACE>
        const REQUESTEDINTERFACE* QueryInterface() const
        {
            const void* baseInterface(const_cast<IUnknown*>(this)->QueryInterface(REQUESTEDINTERFACE::ID));

            if (baseInterface != nullptr) {
                return (reinterpret_cast<const REQUESTEDINTERFACE*>(baseInterface));
            }

            return (nullptr);
        }
    };

    namespace memory_order {
    #ifdef __WINDOWS__
        static constexpr std::memory_order memory_order_relaxed = std::memory_order::memory_order_relaxed;
        static constexpr std::memory_order memory_order_consume = std::memory_order::memory_order_seq_cst;
        static constexpr std::memory_order memory_order_acquire = std::memory_order::memory_order_seq_cst;
        static constexpr std::memory_order memory_order_release = std::memory_order::memory_order_release;
        static constexpr std::memory_order memory_order_acq_rel = std::memory_order::memory_order_seq_cst;
        static constexpr std::memory_order memory_order_seq_cst = std::memory_order::memory_order_seq_cst;
    #else
        static constexpr std::memory_order memory_order_relaxed = std::memory_order::memory_order_relaxed;
        static constexpr std::memory_order memory_order_consume = std::memory_order::memory_order_consume;
        static constexpr std::memory_order memory_order_acquire = std::memory_order::memory_order_acquire;
        static constexpr std::memory_order memory_order_release = std::memory_order::memory_order_release;
        static constexpr std::memory_order memory_order_acq_rel = std::memory_order::memory_order_acq_rel;
        static constexpr std::memory_order memory_order_seq_cst = std::memory_order::memory_order_seq_cst;
    #endif
    }

    #define COM_ERROR (0x80000000)

    #define ERROR_CODES \
        ERROR_CODE(ERROR_NONE, 0) \
        ERROR_CODE(ERROR_GENERAL, 1) \
        ERROR_CODE(ERROR_UNAVAILABLE, 2) \
        ERROR_CODE(ERROR_ASYNC_FAILED, 3) \
        ERROR_CODE(ERROR_ASYNC_ABORTED, 4) \
        ERROR_CODE(ERROR_ILLEGAL_STATE, 5) \
        ERROR_CODE(ERROR_OPENING_FAILED, 6) \
        ERROR_CODE(ERROR_ACCEPT_FAILED, 7) \
        ERROR_CODE(ERROR_PENDING_SHUTDOWN, 8) \
        ERROR_CODE(ERROR_ALREADY_CONNECTED, 9) \
        ERROR_CODE(ERROR_CONNECTION_CLOSED, 10) \
        ERROR_CODE(ERROR_TIMEDOUT, 11) \
        ERROR_CODE(ERROR_INPROGRESS, 12) \
        ERROR_CODE(ERROR_COULD_NOT_SET_ADDRESS, 13) \
        ERROR_CODE(ERROR_INCORRECT_HASH, 14) \
        ERROR_CODE(ERROR_INCORRECT_URL, 15) \
        ERROR_CODE(ERROR_INVALID_INPUT_LENGTH, 16) \
        ERROR_CODE(ERROR_DESTRUCTION_SUCCEEDED, 17) \
        ERROR_CODE(ERROR_DESTRUCTION_FAILED, 18) \
        ERROR_CODE(ERROR_CLOSING_FAILED, 19) \
        ERROR_CODE(ERROR_PROCESS_TERMINATED, 20) \
        ERROR_CODE(ERROR_PROCESS_KILLED, 21) \
        ERROR_CODE(ERROR_UNKNOWN_KEY, 22) \
        ERROR_CODE(ERROR_INCOMPLETE_CONFIG, 23) \
        ERROR_CODE(ERROR_PRIVILIGED_REQUEST, 24) \
        ERROR_CODE(ERROR_RPC_CALL_FAILED, 25) \
        ERROR_CODE(ERROR_UNREACHABLE_NETWORK, 26) \
        ERROR_CODE(ERROR_REQUEST_SUBMITTED, 27) \
        ERROR_CODE(ERROR_UNKNOWN_TABLE, 28) \
        ERROR_CODE(ERROR_DUPLICATE_KEY, 29) \
        ERROR_CODE(ERROR_BAD_REQUEST, 30) \
        ERROR_CODE(ERROR_PENDING_CONDITIONS, 31) \
        ERROR_CODE(ERROR_SURFACE_UNAVAILABLE, 32) \
        ERROR_CODE(ERROR_PLAYER_UNAVAILABLE, 33) \
        ERROR_CODE(ERROR_FIRST_RESOURCE_NOT_FOUND, 34) \
        ERROR_CODE(ERROR_SECOND_RESOURCE_NOT_FOUND, 35) \
        ERROR_CODE(ERROR_ALREADY_RELEASED, 36) \
        ERROR_CODE(ERROR_NEGATIVE_ACKNOWLEDGE, 37) \
        ERROR_CODE(ERROR_INVALID_SIGNATURE, 38) \
        ERROR_CODE(ERROR_READ_ERROR, 39) \
        ERROR_CODE(ERROR_WRITE_ERROR, 40) \
        ERROR_CODE(ERROR_INVALID_DESIGNATOR, 41) \
        ERROR_CODE(ERROR_UNAUTHENTICATED, 42) \
        ERROR_CODE(ERROR_NOT_EXIST, 43) \
        ERROR_CODE(ERROR_NOT_SUPPORTED, 44) \
        ERROR_CODE(ERROR_INVALID_RANGE, 45) \
        ERROR_CODE(ERROR_HIBERNATED, 46) \
        ERROR_CODE(ERROR_INPROC, 47) \
        ERROR_CODE(ERROR_FAILED_REGISTERED, 48) \
        ERROR_CODE(ERROR_FAILED_UNREGISTERED, 49) 

    #define ERROR_CODE(CODE, VALUE) CODE = VALUE,

    enum ErrorCodes {
        ERROR_CODES
        ERROR_COUNT
    };

    #undef ERROR_CODE

    // Convert error enumerations to string

    template<uint32_t N>
    inline const TCHAR* _Err2Str()
    {
        return _T("");
    };

    #define ERROR_CODE(CODE, VALUE) \
        template<> inline const TCHAR* _Err2Str<VALUE>() { return _T(#CODE); }

    ERROR_CODES;

    template<uint32_t N = (ERROR_COUNT - 1)>
    inline const TCHAR* _bogus_ErrorToString(uint32_t code)
    {
        return (code == N? _Err2Str<N>() : _bogus_ErrorToString<N-1>(code));
    };

    template<>
    inline const TCHAR* _bogus_ErrorToString<0u>(uint32_t code)
    {
        return (code == 0? _Err2Str<0u>() : _Err2Str<~0u>());
    };

    inline const TCHAR* ErrorToString(uint32_t code)
    {
        return _bogus_ErrorToString<>(code);
    }

    #undef ERROR_CODE
}
}

extern "C" {

#ifdef __WINDOWS__
extern int EXTERNAL inet_aton(const char* cp, struct in_addr* inp);
extern void EXTERNAL usleep(const uint32_t value);
#endif

void EXTERNAL DumpCallStack(const ThreadId threadId, std::list<WPEFramework::Core::callstack_info>& stack);
uint32_t EXTERNAL GetCallStack(const ThreadId threadId, void* addresses[], const uint32_t bufferSize);

}


#ifndef BUILD_REFERENCE
#define BUILD_REFERENCE engineering_build_for_debug_purpose_only
#endif

#ifdef __GNUC__
#if __GNUC__ < 4 || (__GNUC__ == 4 && (__GNUC_MINOR__ < 9 || (__GNUC_MINOR__ == 9 && __GNUC_PATCHLEVEL__ < 3)))
//defining atomic_init: see https://gcc.gnu.org/bugzilla/show_bug.cgi?id=64658"
namespace std {
  template<typename _ITp>
    inline void
    atomic_init(atomic<_ITp>* __a, _ITp __i) noexcept
    {
       __a->store(__i, memory_order_relaxed);
    }
}
#endif
#endif

#define THUNDER_VERSION 4

#endif // __PORTABILITY_H
