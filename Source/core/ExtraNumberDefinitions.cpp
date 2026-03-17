#include "ExtraNumberDefinitions.h"

namespace Thunder {
namespace Core {

    // Out of class helpers for reordered operands of arithmetic operators
    // -------------------------------------------------------------------

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, T>::type
    operator+(T value, const SInt24& RHS)
    {
        ASSERT(!(   RHS.Overflowed() != false
                 || // Negative overflow
                    (   value < 0
                     && static_cast<T>(RHS) < 0
                     && (value < (std::numeric_limits<T>::lowest() - static_cast<T>(RHS)))
                    )
                ||  // Positive overflow
                    (   value > 0
                     && static_cast<T>(RHS) > 0
                     && (value > (std::numeric_limits<T>::max() - static_cast<T>(RHS)))
                    )
                )
        );

        return ( value + static_cast<T>(RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, T>::type
    operator+(T value, const UInt24& RHS)
    {
        ASSERT(!(   RHS.Overflowed() != false
                 ||
                    (   value > 0
                     && (value > (std::numeric_limits<T>::max() - static_cast<T>(RHS)))
                    )
                )
        );

        return ( value + static_cast<T>(RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, T>::type
    operator-(T value, const SInt24& RHS)
    {
        ASSERT(!(   RHS.Overflowed() != false
                 || // Negative overflow
                    (    value < 0
                      && static_cast<T>(RHS) > 0
                      && (value < (std::numeric_limits<T>::lowest() + static_cast<T>(RHS)))
                    )
                 || // Positive overflow
                    (    value > 0
                      && static_cast<T>(RHS) < 0
                      && (value > (std::numeric_limits<T>::max() + static_cast<T>(RHS)))
                    )
                )
        );

        return ( value - static_cast<T>(RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, T>::type
    operator-(T value, const UInt24& RHS)
    {
        ASSERT(!(   RHS.Overflowed() != false
                 ||
                    (    value < 0
                      && (value < (std::numeric_limits<T>::lowest() + static_cast<T>(RHS)))
                    )
                )
        );

        return ( value - static_cast<T>(RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, T>::type
    operator*(T value, const SInt24& RHS)
    {
        ASSERT(!(   RHS.Overflowed() != false
                 || ( // Both positive or both negative
                         (    value > 0
                           && static_cast<T>(RHS) > 0
                           && static_cast<T>(value) > (std::numeric_limits<T>::max() / static_cast<T>(RHS))
                         )
                      ||
                         (    static_cast<T>(value) < 0
                           && static_cast<T>(RHS) < 0
                           && static_cast<T>(value) < (std::numeric_limits<T>::max() / static_cast<T>(RHS))
                         )
                    )
                 || ( // One positive and one negative
                         (
                              value > 0
                           && static_cast<T>(RHS) < 0
                           && static_cast<T>(RHS) < (std::numeric_limits<T>::lowest() / static_cast<T>(value))
                         )
                      || (
                              value < 0
                           && static_cast<T>(RHS) > 0
                           && static_cast<T>(value) < (std::numeric_limits<T>::lowest() / static_cast<T>(RHS))
                         )
                      )
                    )
        );

        return ( value * static_cast<T>(RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, T>::type
    operator*(T value, const UInt24& RHS)
    {
        ASSERT(!(   RHS.Overflowed() != false
                 || (    value > 0
                      && RHS > 0
                      && static_cast<T>(value) > (std::numeric_limits<T>::max() / static_cast<T>(RHS))
                    )
                 || (   value < 0
                     && RHS > 0
                     && static_cast<T>(value) < (std::numeric_limits<T>::lowest() / static_cast<T>(RHS))
                    )
                )
        );

        return ( value * static_cast<T>(RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, T>::type
    operator/(T value, const SInt24& RHS)
    {
        ASSERT(!(   RHS.Overflowed() != false
                 || static_cast<T>(RHS) == 0
                )
        );

        return ( value / static_cast<T>(RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, T>::type
    operator/(T value, const UInt24& RHS)
    {
        ASSERT(!(   RHS.Overflowed() != false
                 || static_cast<T>(RHS) == 0
                )
        );

        return ( value / static_cast<T>(RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, T>::type
    operator%(T value, const SInt24& RHS)
    {
        ASSERT(!(   RHS.Overflowed() != false
                 || static_cast<T>(RHS) == 0
                )
        );

        return ( value % static_cast<T>(RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, T>::type
    operator%(T value, const UInt24& RHS)
    {
        ASSERT(!(   RHS.Overflowed() != false
                 || static_cast<T>(RHS) == 0
                )
        );

        return ( value % static_cast<T>(RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, T>::type
    operator&(T value, const SInt24& RHS)
    {
        ASSERT(RHS.Overflowed() != true);

        return ( value & static_cast<T>(RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, T>::type
    operator&(T value, const UInt24& RHS)
    {
        ASSERT(RHS.Overflowed() != true);

        return ( value & static_cast<T>(RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, T>::type
    operator|(T value, const SInt24& RHS)
    {
        ASSERT(RHS.Overflowed() != true);

        return ( value | static_cast<T>(RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, T>::type
    operator|(T value, const UInt24& RHS)
    {
        ASSERT(RHS.Overflowed() != true);

        return ( value | static_cast<T>(RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, T>::type
    operator^(T value, const SInt24& RHS)
    {
        ASSERT(RHS.Overflowed() != true);

        return ( value ^ static_cast<T>(RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, T>::type
    operator^(T value, const UInt24& RHS)
    {
        ASSERT(RHS.Overflowed() != true);

        return ( value ^ static_cast<T>(RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, T>::type
    operator<<(T value, const SInt24& RHS)
    {
        ASSERT(!(   RHS.Overflowed() != false
                 || static_cast<T>(RHS) < 0
                 || static_cast<std::size_t>(RHS) >= (sizeof(T) * 8)
                 || value < 0
                )
        );

        return ( value << static_cast<T>(RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, T>::type
    operator<<(T value, const UInt24& RHS)
    {
        ASSERT(!(   RHS.Overflowed() != false
                 || static_cast<std::size_t>(RHS) >= (sizeof(T) * 8)
                 || value < 0
                )
        );

        return ( value << static_cast<T>(RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, T>::type
    operator>>(T value, const SInt24& RHS)
    {
        ASSERT(!(    RHS.Overflowed() != false 
                  || static_cast<T>(RHS) < 0
                  || static_cast<std::size_t>(RHS) >= (sizeof(T) * 8)
                     // Implementation defined, here defined as overflow
                  || value < 0
                )
        );

        return ( value >> static_cast<T>(RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, T>::type
    operator>>(T value, const UInt24& RHS)
    {
        ASSERT(!(    RHS.Overflowed() != false 
                  || static_cast<std::size_t>(RHS) >= (sizeof(T) * 8)
                     // Implementation defined, here defined as overflow
                  || value < 0
                )
        );

        return ( value >> static_cast<T>(RHS) );
    }

    // Out of class helpers for reordered operands of logical operators
    // ----------------------------------------------------------------

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, bool>::type
    operator&&(T value, const SInt24& RHS)
    {
        return ( value && static_cast<T>(RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, bool>::type
    operator&&(T value, const UInt24& RHS)
    {
        return ( value && static_cast<T>(RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, bool>::type
    operator||(T value, const SInt24& RHS)
    {
        return ( value || static_cast<T>(RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, bool>::type
    operator||(T value, const UInt24& RHS)
    {
        return ( value || static_cast<T>(RHS) );
    }

    // Out of class helpers for reordered operands of assignment operators
    // -------------------------------------------------------------------

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, T>::type
    operator+=(T value, const SInt24& RHS)
    {
        return ( value = static_cast<T>(value + RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, T>::type
    operator+=(T value, const UInt24& RHS)
    {
        return ( value = static_cast<T>(value + RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, T>::type
    operator-=(T value, const SInt24& RHS)
    {
        return ( value = static_cast<T>(value - RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, T>::type
    operator-=(T value, const UInt24& RHS)
    {
        return ( value = static_cast<T>(value - RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, T>::type
    operator*=(T value, const SInt24& RHS)
    {
        return ( value = static_cast<T>(value * RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, T>::type
    operator*=(T value, const UInt24& RHS)
    {
        return ( value = static_cast<T>(value * RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, T>::type
    operator/=(T value, const SInt24& RHS)
    {
        return ( value = static_cast<T>(value / RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, T>::type
    operator/=(T value, const UInt24& RHS)
    {
        return ( value = static_cast<T>(value / RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, T>::type
    operator%=(T value, const SInt24& RHS)
    {
        return ( value = static_cast<T>(value % RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, T>::type
    operator%=(T value, const UInt24& RHS)
    {
        return ( value = static_cast<T>(value % RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, T>::type
    operator&=(T value, const SInt24& RHS)
    {
        return ( value = static_cast<T>(value & RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, T>::type
    operator&=(T value, const UInt24& RHS)
    {
        return ( value = static_cast<T>(value & RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, T>::type
    operator|=(T value, const SInt24& RHS)
    {
        return ( value = static_cast<T>(value | RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, T>::type
    operator|=(T value, const UInt24& RHS)
    {
        return ( value = static_cast<T>(value | RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, T>::type
    operator^=(T value, const SInt24& RHS)
    {
        return ( value = static_cast<T>(value ^ RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, T>::type
    operator^=(T value, const UInt24& RHS)
    {
        return ( value = static_cast<T>(value ^ RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, T>::type
    operator<<=(T value, const SInt24& RHS)
    {
        return ( value = static_cast<T>(value << RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, T>::type
    operator<<=(T value, const UInt24& RHS)
    {
        return ( value = static_cast<T>(value << RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, T>::type
    operator>>=(T value, const SInt24& RHS)
    {
        return ( value = static_cast<T>(value >> RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, T>::type
    operator>>=(T value, const UInt24& RHS)
    {
        return ( value = static_cast<T>(value >> RHS) );
    }

    // Out of class helpers for reordered operands of comparison operators
    // -------------------------------------------------------------------

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, bool>::type
    operator==(T value, const SInt24& RHS)
    {
        return ( value == static_cast<T>(RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, bool>::type
    operator==(T value, const UInt24& RHS)
    {
        return ( value == static_cast<T>(RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, bool>::type
    operator!=(T value, const SInt24& RHS)
    {
        return ( value != static_cast<T>(RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, bool>::type
    operator!=(T value, const UInt24& RHS)
    {
        return ( value != static_cast<T>(RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, bool>::type
    operator<(T value, const SInt24& RHS)
    {
        ASSERT(RHS.Overflowed() != true);

        return ( value < static_cast<T>(RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, bool>::type
    operator<(T value, const UInt24& RHS)
    {
        ASSERT(RHS.Overflowed() != true);

        return ( value < static_cast<T>(RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, bool>::type
    operator>(T value, const SInt24& RHS)
    {
        ASSERT(RHS.Overflowed() != true);

        return ( value > static_cast<T>(RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, bool>::type
    operator>(T value, const UInt24& RHS)
    {
        ASSERT(RHS.Overflowed() != true);

        return ( value > static_cast<T>(RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, bool>::type
    operator<=(T value, const SInt24& RHS)
    {
        return ( value <= static_cast<T>(RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, bool>::type
    operator<=(T value, const UInt24& RHS)
    {
        return ( value <= static_cast<T>(RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, bool>::type
    operator>=(T value, const SInt24& RHS)
    {
        return ( value >= static_cast<T>(RHS) );
    }

    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, bool>::type
    operator>=(T value, const UInt24& RHS)
    {
        return ( value >= static_cast<T>(RHS) );
    }

    // Explicit instantiation

    using SInt24InstantiationType = SInt24::InternalType;
    using UInt24InstantiationType = UInt24::InternalType;

    template SInt24InstantiationType EXTERNAL operator+(SInt24InstantiationType value, const SInt24& RHS);
    template UInt24InstantiationType EXTERNAL operator+(UInt24InstantiationType value, const UInt24& RHS);
    template SInt24InstantiationType EXTERNAL operator-(SInt24InstantiationType value, const SInt24& RHS);
    template UInt24InstantiationType EXTERNAL operator-(UInt24InstantiationType value, const UInt24& RHS);
    template SInt24InstantiationType EXTERNAL operator*(SInt24InstantiationType value, const SInt24& RHS);
    template UInt24InstantiationType EXTERNAL operator*(UInt24InstantiationType value, const UInt24& RHS);
    template SInt24InstantiationType EXTERNAL operator/(SInt24InstantiationType value, const SInt24& RHS);
    template UInt24InstantiationType EXTERNAL operator/(UInt24InstantiationType value, const UInt24& RHS);
    template SInt24InstantiationType EXTERNAL operator%(SInt24InstantiationType value, const SInt24& RHS);
    template UInt24InstantiationType EXTERNAL operator%(UInt24InstantiationType value, const UInt24& RHS);
    template SInt24InstantiationType EXTERNAL operator&(SInt24InstantiationType value, const SInt24& RHS);
    template UInt24InstantiationType EXTERNAL operator&(UInt24InstantiationType value, const UInt24& RHS);
    template SInt24InstantiationType EXTERNAL operator|(SInt24InstantiationType value, const SInt24& RHS);
    template UInt24InstantiationType EXTERNAL operator|(UInt24InstantiationType value, const UInt24& RHS);
    template SInt24InstantiationType EXTERNAL operator^(SInt24InstantiationType value, const SInt24& RHS);
    template UInt24InstantiationType EXTERNAL operator^(UInt24InstantiationType value, const UInt24& RHS);
    template SInt24InstantiationType EXTERNAL operator<<(SInt24InstantiationType value, const SInt24& RHS);
    template UInt24InstantiationType EXTERNAL operator<<(UInt24InstantiationType value, const UInt24& RHS);
    template SInt24InstantiationType EXTERNAL operator>>(SInt24InstantiationType value, const SInt24& RHS);
    template UInt24InstantiationType EXTERNAL operator>>(UInt24InstantiationType value, const UInt24& RHS);

    template bool EXTERNAL operator&&(SInt24InstantiationType, const SInt24&);
    template bool EXTERNAL operator&&(UInt24InstantiationType, const UInt24&);
    template bool EXTERNAL operator||(SInt24InstantiationType, const SInt24&);
    template bool EXTERNAL operator||(UInt24InstantiationType, const UInt24&);

    template SInt24InstantiationType EXTERNAL operator+=(SInt24InstantiationType value, const SInt24& RHS);
    template UInt24InstantiationType EXTERNAL operator+=(UInt24InstantiationType value, const UInt24& RHS);
    template SInt24InstantiationType EXTERNAL operator-=(SInt24InstantiationType value, const SInt24& RHS);
    template UInt24InstantiationType EXTERNAL operator-=(UInt24InstantiationType value, const UInt24& RHS);
    template SInt24InstantiationType EXTERNAL operator*=(SInt24InstantiationType value, const SInt24& RHS);
    template UInt24InstantiationType EXTERNAL operator*=(UInt24InstantiationType value, const UInt24& RHS);
    template SInt24InstantiationType EXTERNAL operator/=(SInt24InstantiationType value, const SInt24& RHS);
    template UInt24InstantiationType EXTERNAL operator/=(UInt24InstantiationType value, const UInt24& RHS);
    template SInt24InstantiationType EXTERNAL operator%=(SInt24InstantiationType value, const SInt24& RHS);
    template UInt24InstantiationType EXTERNAL operator%=(UInt24InstantiationType value, const UInt24& RHS);
    template SInt24InstantiationType EXTERNAL operator&=(SInt24InstantiationType value, const SInt24& RHS);
    template UInt24InstantiationType EXTERNAL operator&=(UInt24InstantiationType value, const UInt24& RHS);
    template SInt24InstantiationType EXTERNAL operator|=(SInt24InstantiationType value, const SInt24& RHS);
    template UInt24InstantiationType EXTERNAL operator|=(UInt24InstantiationType value, const UInt24& RHS);
    template SInt24InstantiationType EXTERNAL operator^=(SInt24InstantiationType value, const SInt24& RHS);
    template UInt24InstantiationType EXTERNAL operator^=(UInt24InstantiationType value, const UInt24& RHS);
    template SInt24InstantiationType EXTERNAL operator<<=(SInt24InstantiationType value, const SInt24& RHS);
    template UInt24InstantiationType EXTERNAL operator<<=(UInt24InstantiationType value, const UInt24& RHS);
    template SInt24InstantiationType EXTERNAL operator>>=(SInt24InstantiationType value, const SInt24& RHS);
    template UInt24InstantiationType EXTERNAL operator>>=(UInt24InstantiationType value, const UInt24& RHS);

    template bool EXTERNAL operator==(SInt24InstantiationType, const SInt24&);
    template bool EXTERNAL operator==(UInt24InstantiationType, const UInt24&);
    template bool EXTERNAL operator!=(SInt24InstantiationType, const SInt24&);
    template bool EXTERNAL operator!=(UInt24InstantiationType, const UInt24&);
    template bool EXTERNAL operator<(SInt24InstantiationType, const SInt24&);
    template bool EXTERNAL operator<(UInt24InstantiationType, const UInt24&);
    template bool EXTERNAL operator>(SInt24InstantiationType, const SInt24&);
    template bool EXTERNAL operator>(UInt24InstantiationType, const UInt24&);
    template bool EXTERNAL operator<=(SInt24InstantiationType, const SInt24&);
    template bool EXTERNAL operator<=(UInt24InstantiationType, const UInt24&);
    template bool EXTERNAL operator>=(SInt24InstantiationType, const SInt24&);
    template bool EXTERNAL operator>=(UInt24InstantiationType, const UInt24&);

} // namespace Core
} // namespace Thunder
