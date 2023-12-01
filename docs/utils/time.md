# Time

Time as a non-constant continues to progress while existence of various events is being linked to each instance of time irrespective of the nature of such an event. Thunder has many types of events in which instances of time play a role. Their nature may be in the field of (process) communication by exchanging various messages, it may be related to the scheduling of (future) tasks or it may play some other role. The list of options is non-exhaustive.

## (Internal) representation

Thunder models time with instances described by elements of the [Gregorian calendar](https://en.wikipedia.org/wiki/Gregorian_calendar) with its epoch defined by [Unix time](https://en.wikipedia.org/wiki/Unix_time). That is, it models time as (strictly) monotomic increasing integer values. The elements year, month, day, hour, second, and millisecond allow for unique descriptions of Gregorian calendar time instances.

Set values can be individually retrieved. Time instances can be used in calculations and these instances can also be adjusted in millisecond decrements and increments.

Values are depicted with respect to the [UTC timezone](https://en.wikipedia.org/wiki/Coordinated_Universal_Time) unless otherwise specified. Alternatively, a derived convenience model incorporates local time and date offset by the timezone configured for the platform. The user conveniently can ignore any (underlying) offset.

```c++
// Local time instead of UTC
Core::Time past(/*year*/ 2023, /*month*/ 10, /*day*/ 13, /*hours*/ 18, /*minutes*/ 02, /*seconds*/ 0 ,/* milliseconds*/ 0, /*local time*/ true);
// Implicitly UTC
const Core::Time now(Core::Time::Now());

if (past.Add(1000) < now()) {
    std::cout << "More than second has elapsed since "
              << past.WeekDayName() << ", "
              << past.MonthName()   << " "
              << past.Day()         << ", "
              << past.Hours()       << ":"
              << past.Minutes()     << ":"
              << past.Seconds()     << "."
              << std::endl;
}

// Internally represent the past with our local timezone instead of UTC
TimeAsLocal local(past);
```

In the given example a time object is intialized to October 10 of the year 2023 at 18:02h and a second time object is initialized to the value of the system time. Note that the second object does not specify local time and hence it is silently set to reflect values for the UTC timezone. A final check is executed to test if past time is more than a second ago. For completeness a local time object is created from the object representing the past.

## Date and time conversion options

The external representation of time is a string formatted in a predefined pattern unless a single integer valued element is represented. Such patterns are intrinsically applied for the various conversions methods made available.  It allows users to conveniently convert 'From' one representation 'To' another by using correspondingly named methods. Among such conversions are the [RFC2311](https://www.rfc-editor.org/rfc/rfc2311) time and date specification, [RFC1036](https://www.rfc-editor.org/rfc/rfc1036) date specification, the [ISO8601]() date and time formats, and, [ANSI](https://webstore.ansi.org/standards/incits/ansiincits301997) date representation.

Users that are interested in a format other than any provided by the Gregorian calendar representations may use *clock ticks*. Ticks are merely a conversion unit given from the multiplication of a constant value and the milliseconds value. Historically, it was a measure of runtime code execution on a CPU. Thunder has defined the multiplier constant equal to 1000.

Although most people typically work with a Gregorian scheme time values can be easily converted to [Julian calendar](https://en.wikipedia.org/wiki/Julian_calendar) representation, and, back. With its corresponding [Julian days](https://en.wikipedia.org/wiki/Julian_day) being analogous to elapsed seconds since the epoch of Unix time.

Finally, NTP conversions allows for incorporation of a different epoch. NTP epoch equals to 25567 seconds offset from its Unix time equivalent.

```c++
if (past.ToISO8601() == past.ToRFC1123()) {
    std::cout << "It is very surprising to get here." << std::endl;
} else {
    std::cout << "This is more likley to happen." << std::endl;
}

past.FromString("Sun Nov 6 08:49:00 1994");

if (past.JulianDate()) == 94310) {
    std::cout << "The Julian date is equivalent to " << past.Ticks() / Core::Time::TicksPerMilliSecond / Core::Time::MilliSecondsPerSecond << " seconds from the epoch of Unix Time." << std::endl;"
}

```
The depicted example continues the previous example. Here, the different strings from the conversion methods typically have different patterns and do not equate equal. In addition, the past time object is re-inialized using the ANSI pattern to November 6 of the year 1994 at 08:49h which is equivalent to 94310 Julian days.
