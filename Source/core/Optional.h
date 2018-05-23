#ifndef __OPTIONAL_H
#define __OPTIONAL_H

#include "Portability.h"

namespace WPEFramework {
namespace Core {
    template <typename TYPE>
    class OptionalType {
    public:
        OptionalType()
            : m_Value()
            , m_Set(false)
        {
        }

        OptionalType(const TYPE& value)
            : m_Value(value)
            , m_Set(true)
        {
        }

        OptionalType(const OptionalType<TYPE>& value)
            : m_Value(value.m_Value)
            , m_Set(value.m_Set)
        {
        }

        ~OptionalType()
        {
        }

        inline OptionalType<TYPE>& operator=(const OptionalType<TYPE>& value)
        {
            m_Value = value.m_Value;
            m_Set = value.m_Set;

            return (*this);
        }

        inline OptionalType<TYPE>& operator=(const TYPE& value)
        {
            m_Value = value;
            m_Set = true;

            return (*this);
        }

        inline bool operator==(const TYPE& value) const
        {
            return (value == m_Value);
        }

        inline bool operator==(const OptionalType<TYPE>& value) const
        {
            return ((value.m_Set == m_Set)&& operator==(value.m_Value));
        }

        inline bool operator!=(const TYPE& value) const
        {
            return (!operator==(value));
        }

        inline bool operator!=(const OptionalType<TYPE>& value) const
        {
            return (!operator==(value));
        }

    public:
        bool IsSet() const
        {
            return (m_Set);
        }
        inline void Clear()
        {
            m_Set = false;
        }

        operator TYPE&()
        {
            return (m_Value);
        }

        operator const TYPE&() const
        {
            return (m_Value);
        }

        TYPE& Value()
        {
            return (m_Value);
        }

        const TYPE& Value() const
        {
            return (m_Value);
        }

    private:
        TYPE m_Value;
        bool m_Set;
    };
}
} // namespace Core

#endif // __OPTIONAL_H
